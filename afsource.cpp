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
#include "itunetags.h"

typedef std::shared_ptr<const __CFDictionary> CFDictionaryPtr;

namespace caf {
    uint64_t next_chunk(int fd, char *name)
    {
	uint64_t size;
	if (util::nread(fd, name, 4) != 4 || util::nread(fd, &size, 8) != 8)
	    return 0;
	return util::b2host64(size);
    }
    bool get_info(int fd, std::vector<char> *info)
    {
	util::FilePositionSaver _(fd);
	if (_lseeki64(fd, 8, SEEK_SET) != 8)
	    return false;
	uint64_t chunk_size;
	char chunk_name[4];
	while ((chunk_size = next_chunk(fd, chunk_name)) > 0) {
	    if (std::memcmp(chunk_name, "info", 4)) {
		if (_lseeki64(fd, chunk_size, SEEK_CUR) < 0)
		    break;
	    } else {
		std::vector<char> buf(chunk_size);
		if (util::nread(fd, &buf[0], buf.size()) != buf.size())
		    break;
		info->swap(buf);
		return true;
	    }
	}
	return false;
    }
    bool get_info_dictionary(int fd, CFDictionaryPtr *dict)
    {
	std::vector<char> info;
	if (!get_info(fd, &info) || info.size() < 4)
	    return false;
	// inside of info tag is delimited with NUL char.
	std::vector<std::string> tokens;
	{
	    const char *infop = &info[0] + 4;
	    const char *endp = &info[0] + info.size();
	    do {
		tokens.push_back(std::string(infop));
		infop += tokens.back().size() + 1;
	    } while (infop < endp);
	}
	CFMutableDictionaryRef dictref =
	    cautil::CreateDictionary(tokens.size() >> 1);
	CFDictionaryPtr dictptr(dictref, CFRelease);
	for (size_t i = 0; i < tokens.size() >> 1; ++i) {
	    CFStringPtr key = cautil::W2CF(strutil::us2w(tokens[2 * i]));
	    CFStringPtr value = cautil::W2CF(strutil::us2w(tokens[2 * i + 1]));
	    CFDictionarySetValue(dictref, key.get(), value.get());
	}
	dict->swap(dictptr);
	return true;
    }
}

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

    const Tag::NameIDMap tagNameMap[] = {
	{ "title", Tag::kTitle },
	{ "artist", Tag::kArtist },
	{ "album", Tag::kAlbum },
	{ "composer", Tag::kComposer },
	{ "genre", Tag::kGenre },
	{ "year", Tag::kDate },
	{ "track number", Tag::kTrack },
	{ "comments", Tag::kComment },
	{ "subtitle", Tag::kSubTitle },
	{ "tempo", Tag::kTempo },
	{ 0, 0 }
    };
    uint32_t getIDFromTagName(const char *name)
    {
	const Tag::NameIDMap *map = tagNameMap;
	for (; map->name; ++map)
	    if (!strcasecmp(map->name, name))
		return map->id;
	return 0;
    }

    typedef std::map<uint32_t, std::wstring> tag_t;

    void fetchTagDictCallback(const void *key, const void *value, void *ctx)
    {
	if (CFGetTypeID(key) != CFStringGetTypeID() ||
	    CFGetTypeID(value) != CFStringGetTypeID())
	    return;
	tag_t *tagp = static_cast<tag_t*>(ctx);
	std::wstring wskey = cautil::CF2W(static_cast<CFStringRef>(key));
	std::string utf8key = strutil::w2us(wskey);
	uint32_t id = getIDFromTagName(utf8key.c_str());
	if (id)
	    (*tagp)[id] = cautil::CF2W(static_cast<CFStringRef>(value));
    }

    void fetchTags(AudioFileX &af, FILE *fp, tag_t *result)
    {
	CFDictionaryPtr dict;
	if (af.getFileFormat() == 'caff')
	    caf::get_info_dictionary(fileno(fp), &dict);
	else
	    af.getInfoDictionary(&dict);
	tag_t tags;
	if (dict.get()) {
	    CFDictionaryApplyFunction(dict.get(), fetchTagDictCallback, &tags);
	    result->swap(tags);
	}
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

    AudioStreamBasicDescription asbd;
    m_af.getDataFormat(&asbd);
    if (asbd.mFormatID != 'lpcm' && asbd.mFormatID != 'alac' &&
	asbd.mFormatID != '.mp3')
	throw std::runtime_error("Not supported input format");

    uint32_t fcc = m_af.getFileFormat();

    if (asbd.mFormatID == 'lpcm') {
	bool isfloat = asbd.mFormatFlags & kAudioFormatFlagIsFloat;
	uint32_t packbits = asbd.mBytesPerFrame / asbd.mChannelsPerFrame * 8;
	m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
					  asbd.mChannelsPerFrame,
					  asbd.mBitsPerChannel,
					  isfloat ? packbits : 32,
					  isfloat ? kAudioFormatFlagIsFloat
					    : kAudioFormatFlagIsSignedInteger);
    } else if (asbd.mFormatID == 'alac') {
	unsigned bits_per_channel;
	{
	    unsigned tab[] = { 16, 20, 24, 32 };
	    unsigned index = (asbd.mFormatFlags - 1) & 0x3;
	    bits_per_channel = tab[index];
	}
	m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
					  asbd.mChannelsPerFrame,
					  bits_per_channel, 32,
					  kAudioFormatFlagIsSignedInteger);
    } else {
	m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
					  asbd.mChannelsPerFrame, 32, 32,
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

    int64_t length = m_af.getAudioDataPacketCount() * asbd.mFramesPerPacket;

    if (fcc == kAudioFileAIFFType || fcc == kAudioFileAIFCType)
	ID3::fetchAiffID3Tags(fileno(m_fp.get()), &m_tags);
    else if (fcc == kAudioFileMP3Type)
	ID3::fetchMPEGID3Tags(fileno(m_fp.get()), &m_tags);
    else if (fcc == 'm4af' || fcc == 'm4bf' || fcc == 'mp4f') {
	try {
	    int fd = fileno(m_fp.get());
	    util::FilePositionSaver _(fd);
	    static MP4FDReadProvider provider;
	    MP4FileX file;
	    std::string name = strutil::format("%d", fd);
	    file.Read(name.c_str(), &provider);
	    mp4a::fetchTags(file, &m_tags);
	} catch (...) {}
    } else {
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
	if (total == length) {
	    length = ptinfo.mNumberValidFrames;
	} else if (total == length / 2) {
	    length = ptinfo.mNumberValidFrames * 2;
	} else if (!ptinfo.mNumberValidFrames && ptinfo.mPrimingFrames)
	    length = std::max(0LL, length - ptinfo.mPrimingFrames
			                  - ptinfo.mRemainderFrames);
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
    int preroll_packets = 0;
    switch (m_asbd.mFormatID) {
    case kAudioFormatMPEGLayer1: preroll_packets = 1; break;
    case kAudioFormatMPEGLayer2: preroll_packets = 1; break;
    case kAudioFormatMPEGLayer3: preroll_packets = 2; break;
    }
    int64_t off
	= std::max(0LL, count - m_asbd.mFramesPerPacket * preroll_packets);
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
