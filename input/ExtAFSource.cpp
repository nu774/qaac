#include "ExtAFSource.h"
#include "cautil.h"
#include "chanmap.h"
#include "strutil.h"
#include "metadata.h"

typedef std::shared_ptr<const __CFDictionary> CFDictionaryPtr;

namespace audiofile {

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

    std::map<std::string, std::string> fetchTags(AudioFileX &af)
    {
        std::map<std::string, std::string> tags;

        if (af.getFileFormat() == 'caff') {
            auto data = af.getUserData('info', 0);
            tags = CAF::fetchTags(data);
        } else {
            auto dict = af.getInfoDictionary();
            if (dict.get())
                CFDictionaryApplyFunction(dict.get(), fetchTagDictCallback,
                                          &tags);
        }
        return TextBasedTag::normalizeTags(tags);
    }
}

ExtAFSource::ExtAFSource(std::shared_ptr<IInputStream> stream)
    : m_stream(stream)
{
    AudioFileID afid;
    CHECKCA(AudioFileOpenWithCallbacks(this,
                                       &ExtAFSource::staticReadCallback,
                                       nullptr,
                                       &ExtAFSource::staticSizeCallback,
                                       nullptr,
                                       0,
                                       &afid));
    m_af.attach(afid, true);

    ExtAudioFileRef eaf;
    CHECKCA(ExtAudioFileWrapAudioFileID(m_af, false, &eaf));
    m_eaf.attach(eaf, true);

    auto aflist = m_af.getFormatList();
    m_iasbd = aflist[0].mASBD;
    if (m_iasbd.mFormatID != 'lpcm' && m_iasbd.mFormatID != 'alac' &&
        m_iasbd.mFormatID != '.mp3' && m_iasbd.mFormatID != 'aac ' &&
        m_iasbd.mFormatID != 'aach' && m_iasbd.mFormatID != 'aacp')
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

    try {
        auto acl = m_af.getChannelLayout();
        m_chanmap = chanmap::getChannels(acl.get());
    } catch (...) {
        // getChannelLayout() will fail if chan chunk not present in CAF 
        if (m_iasbd.mFormatID == 'aac ' ||
            m_iasbd.mFormatID == 'aach' ||
            m_iasbd.mFormatID == 'aacp')
        {
            auto kuki = m_af.getMagicCookieData();
            auto asc = cautil::parseMagicCookieAAC(kuki);
            AudioStreamBasicDescription tmp;
            cautil::parseASC(asc, &tmp, &m_chanmap);
        } else if (m_iasbd.mFormatID == 'alac') {
            AudioChannelLayout acl = { 0 };
            acl.mChannelLayoutTag = chanmap::getALACChannelLayoutTag(m_iasbd.mChannelsPerFrame);
            m_chanmap = chanmap::getChannels(&acl);
        }
    }

    /* Let AudioFile scan the file and generate index in case of MP3. 
     * Take up about 1s for 35min MP3 in my environment, but this is 
     * mandatory to obtain sample accurate information.
     */
    m_af.getMaximumPacketSize();

    int64_t length = m_af.getAudioDataPacketCount() * m_iasbd.mFramesPerPacket;

    if (fcc == kAudioFileAIFFType || fcc == kAudioFileAIFCType)
        m_tags = ID3::fetchAiffID3Tags(m_stream);
    else if (fcc == kAudioFileMP3Type)
        m_tags = ID3::fetchMPEGID3Tags(m_stream);
    else if (fcc == kAudioFileCAFType) {
        try {
            m_tags = audiofile::fetchTags(m_af);
        } catch (...) {}
    }
    try {
        auto ptinfo = m_af.getPacketTableInfo();
        if (ptinfo.mPrimingFrames && !ptinfo.mNumberValidFrames) {
            ptinfo.mNumberValidFrames = length - (ptinfo.mPrimingFrames + ptinfo.mRemainderFrames);
        }
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
    if (m_length != ~0ULL) {
        nsamples = static_cast<size_t>(std::min(static_cast<uint64_t>(nsamples),
                                                m_length - m_position));
    }
    if (!nsamples) return 0;
    UInt32 ns = nsamples;
    UInt32 nb = ns * m_asbd.mBytesPerFrame;
    AudioBufferList abl = { 0 };
    abl.mNumberBuffers = 1;
    abl.mBuffers[0].mNumberChannels = m_asbd.mChannelsPerFrame;
    abl.mBuffers[0].mData = buffer;
    abl.mBuffers[0].mDataByteSize = nb;
    CHECKCA(ExtAudioFileRead(m_eaf, &ns, &abl));
    m_position += ns;
    return ns;
}

void ExtAFSource::seekTo(int64_t count)
{
    int npreroll = 0;
    
    switch (m_iasbd.mFormatID) {
    case kAudioFormatMPEGLayer1:     npreroll = 1; break;
    case kAudioFormatMPEGLayer2:     npreroll = 1; break;
    case kAudioFormatMPEGLayer3:     npreroll = 10; break;
    case kAudioFormatMPEG4AAC:       npreroll = 1; break;
    case kAudioFormatMPEG4AAC_HE:    npreroll = m_iasbd.mSampleRate / m_iasbd.mFramesPerPacket / 2; break;
    case kAudioFormatMPEG4AAC_HE_V2: npreroll = m_iasbd.mSampleRate / m_iasbd.mFramesPerPacket / 2; break;
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
    CHECKCA(ExtAudioFileTell(m_eaf, &m_position));
}

OSStatus ExtAFSource::readCallback(SInt64 pos, UInt32 count, void *data, UInt32 *nread)
{
    const int ioErr = -36;
    if (m_stream->seek(pos, SEEK_SET) != pos)
        return ioErr;
    int n = m_stream->read(data, count);
    if (n < 0)
        return ioErr;
    *nread = n;
    return 0;
}

SInt64 ExtAFSource::sizeCallback()
{
    return m_stream->size();
}
