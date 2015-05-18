#include <io.h>
#include "afsource.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "cautil.h"
#include "AudioConverterX.h"
#include "chanmap.h"
#include "mp4v2wrapper.h"
#include "strutil.h"
#include "metadata.h"

typedef std::shared_ptr<const __CFDictionary> CFDictionaryPtr;

namespace audiofile {
    const int ioErr = -36;

    OSStatus read(void *cookie, SInt64 pos, UInt32 count, void *data,
            UInt32 *nread)
    {
        int fd = reinterpret_cast<int>(cookie);
        if (_lseeki64(fd, pos, SEEK_SET) != pos)
            return ioErr;
        ssize_t n = util::nread(fd, data, count);
        if (n < 0)
            return ioErr;
        *nread = n;
        return 0;
    }
    SInt64 size(void *cookie)
    {
        return _filelengthi64(reinterpret_cast<int>(cookie));
    }

    void fetchTagDictCallback(const void *key, const void *value, void *ctx)
    {
        if (CFGetTypeID(key) != CFStringGetTypeID() ||
            CFGetTypeID(value) != CFStringGetTypeID())
            return;
        std::map<std::string, std::string> *tag =
            static_cast<std::map<std::string, std::string>*>(ctx);
        std::wstring wskey = cautil::CF2W(static_cast<CFStringRef>(key));
        std::wstring wsval = cautil::CF2W(static_cast<CFStringRef>(value));
        (*tag)[strutil::w2us(wskey)] = strutil::w2us(wsval);
    }

    void fetchTags(AudioFileX &af, FILE *fp,
                   std::map<std::string, std::string> *result)
    {
        std::map<std::string, std::string> tags;

        if (af.getFileFormat() == 'caff') {
            std::vector<uint8_t> data;
            af.getUserData('info', 0, &data);
            CAF::fetchTags(data, &tags);
        } else {
            CFDictionaryPtr dict;
            af.getInfoDictionary(&dict);
            if (dict.get())
                CFDictionaryApplyFunction(dict.get(), fetchTagDictCallback,
                                          &tags);
        }
        TextBasedTag::normalizeTags(tags, result);
    }
}

ExtAFSource::ExtAFSource(const std::shared_ptr<FILE> &fp)
    : m_fp(fp)
{
    AudioFileID afid;
    void *ctx = reinterpret_cast<void*>(fileno(fp.get()));
    CHECKCA(AudioFileOpenWithCallbacks(ctx, audiofile::read, 0,
                                       audiofile::size, 0, 0, &afid));
    m_af.attach(afid, true);

    ExtAudioFileRef eaf;
    CHECKCA(ExtAudioFileWrapAudioFileID(m_af, false, &eaf));
    m_eaf.attach(eaf, true);

    std::vector<AudioFormatListItem> aflist;
    m_af.getFormatList(&aflist);
    m_iasbd = aflist[0].mASBD;
    if (m_iasbd.mFormatID != 'lpcm' && m_iasbd.mFormatID != 'alac' &&
        m_iasbd.mFormatID != '.mp3')
        throw std::runtime_error("Not supported input format");

    uint32_t fcc = m_af.getFileFormat();

    if (m_iasbd.mFormatID == 'lpcm') {
        bool isfloat = m_iasbd.mFormatFlags & kAudioFormatFlagIsFloat;
        uint32_t bits = m_iasbd.mBytesPerFrame / m_iasbd.mChannelsPerFrame * 8;
        m_asbd = cautil::buildASBDForPCM2(m_iasbd.mSampleRate,
                                          m_iasbd.mChannelsPerFrame,
                                          m_iasbd.mBitsPerChannel,
                                          isfloat ? bits : 32,
                                          isfloat ? kAudioFormatFlagIsFloat
                                            : kAudioFormatFlagIsSignedInteger);
    } else if (m_iasbd.mFormatID == 'alac') {
        unsigned bits_per_channel;
        {
            unsigned tab[] = { 16, 20, 24, 32 };
            unsigned index = (m_iasbd.mFormatFlags - 1) & 0x3;
            bits_per_channel = tab[index];
        }
        m_asbd = cautil::buildASBDForPCM2(m_iasbd.mSampleRate,
                                          m_iasbd.mChannelsPerFrame,
                                          bits_per_channel, 32,
                                          kAudioFormatFlagIsSignedInteger);
    } else {
        m_asbd = cautil::buildASBDForPCM2(m_iasbd.mSampleRate,
                                          m_iasbd.mChannelsPerFrame, 32, 32,
                                          kAudioFormatFlagIsFloat);
    }
    m_eaf.setClientDataFormat(m_asbd);

    std::shared_ptr<AudioChannelLayout> acl;
    try {
        m_af.getChannelLayout(&acl);
        chanmap::getChannels(acl.get(), &m_chanmap);
    } catch (...) {}

    /* Let AudioFile scan the file and generate index in case of MP3. 
     * Take up about 1s for 35min MP3 in my environment, but this is 
     * mandatory to obtain sample accurate information.
     */
    m_af.getMaximumPacketSize();

    int64_t length = m_af.getAudioDataPacketCount() * m_iasbd.mFramesPerPacket;

    if (fcc == kAudioFileAIFFType || fcc == kAudioFileAIFCType)
        ID3::fetchAiffID3Tags(fileno(m_fp.get()), &m_tags);
    else if (fcc == kAudioFileMP3Type)
        ID3::fetchMPEGID3Tags(fileno(m_fp.get()), &m_tags);
    else {
        try {
            audiofile::fetchTags(m_af, m_fp.get(), &m_tags);
        } catch (...) {}
    }
    try {
        AudioFilePacketTableInfo ptinfo = { 0 };
        m_af.getPacketTableInfo(&ptinfo);
        int64_t total =
            ptinfo.mNumberValidFrames + ptinfo.mPrimingFrames +
            ptinfo.mRemainderFrames;
        length = (total == length / 2) ? ptinfo.mNumberValidFrames * 2
                                       : ptinfo.mNumberValidFrames;
    } catch (CoreAudioException &e) {
        if (!e.isNotSupportedError())
            throw;
    }
    m_length = length;
}

size_t ExtAFSource::readSamples(void *buffer, size_t nsamples)
{
    UInt32 ns = nsamples;
    UInt32 nb = ns * m_asbd.mBytesPerFrame;
    AudioBufferList abl = { 0 };
    abl.mNumberBuffers = 1;
    abl.mBuffers[0].mNumberChannels = m_asbd.mChannelsPerFrame;
    abl.mBuffers[0].mData = buffer;
    abl.mBuffers[0].mDataByteSize = nb;
    CHECKCA(ExtAudioFileRead(m_eaf, &ns, &abl));
    return ns;
}

void ExtAFSource::seekTo(int64_t count)
{
    int npreroll = 0;
    
    switch (m_iasbd.mFormatID) {
    case kAudioFormatMPEGLayer1: npreroll = 1; break;
    case kAudioFormatMPEGLayer2: npreroll = 1; break;
    case kAudioFormatMPEGLayer3: npreroll = 10; break;
    }
    int64_t off
        = std::max(0LL, count - m_iasbd.mFramesPerPacket * npreroll);
    CHECKCA(ExtAudioFileSeek(m_eaf, off));
    int32_t distance = count - off;
    while (distance > 0) {
        size_t nbytes = distance * m_asbd.mBytesPerFrame;
        if (nbytes > m_buffer.size())
            m_buffer.resize(nbytes);
        size_t n = readSamples(&m_buffer[0], distance);
        if (n <= 0) break;
        distance -= n;
    }
}

int64_t ExtAFSource::getPosition()
{
   int64_t pos;
   CHECKCA(ExtAudioFileTell(m_eaf, &pos));
   return pos;
}
