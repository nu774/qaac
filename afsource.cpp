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
    uint64_t next_chunk(InputStream *stream, char *name)
    {
	uint64_t size;
	if (stream->read(name, 4) != 4 || stream->read(&size, 8) != 8)
	    return 0;
	return util::b2host64(size);
    }
    bool get_info(InputStream *stream, std::vector<char> *info)
    {
	int64_t pos = stream->tell();
	stream->seek(8, SEEK_SET);
	uint64_t chunk_size;
	char chunk_name[4];
	bool found = false;
	while ((chunk_size = next_chunk(stream, chunk_name)) > 0) {
	    if (std::memcmp(chunk_name, "info", 4)) {
		if (stream->seek(chunk_size, SEEK_CUR) < 0)
		    break;
	    } else {
		std::vector<char> buf(chunk_size);
		if (stream->read(&buf[0], buf.size()) != buf.size())
		    break;
		info->swap(buf);
		found = true;
		break;
	    }
	}
	stream->seek(pos, SEEK_SET);
	return found;
    }
    bool get_info_dictionary(InputStream *stream, CFDictionaryPtr *dict)
    {
	std::vector<char> info;
	if (!get_info(stream, &info) || info.size() < 4)
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
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	if (pT->seek(pos, ISeekable::kBegin) != pos)
	    return ioErr;
	*nread = pT->read(data, count);
	return 0;
    }
    SInt64 size(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->size();
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

    void fetchTags(AudioFileX &af, InputStream *stream, tag_t *result)
    {
	CFDictionaryPtr dict;
	if (af.getFileFormat() == 'caff')
	    caf::get_info_dictionary(stream, &dict);
	else
	    af.getInfoDictionary(&dict);
	tag_t tags;
	if (dict.get()) {
	    CFDictionaryApplyFunction(dict.get(), fetchTagDictCallback, &tags);
	    result->swap(tags);
	}
    }
}

std::shared_ptr<ISource>
AudioFileOpenFactory(InputStream &stream, const std::wstring &path)
{
    AudioFileID afid;
    std::shared_ptr<InputStream> streamPtr(new InputStream(stream));

    CHECKCA(AudioFileOpenWithCallbacks(streamPtr.get(), audiofile::read, 0,
				       audiofile::size, 0, 0, &afid));
    AudioFileX af(afid, true);
    AudioStreamBasicDescription asbd;
    af.getDataFormat(&asbd);

    if (asbd.mFormatID == 'lpcm')
	return std::shared_ptr<ISource>(new AFSource(af, streamPtr));
    else if (asbd.mFormatID == 'alac')
	return std::shared_ptr<ISource>(new ExtAFSource(af, streamPtr, path));
    else
	throw std::runtime_error("Not supported format");
}

AFSource::AFSource(AudioFileX &af, std::shared_ptr<InputStream> &stream)
    : m_af(af), m_offset(0), m_stream(stream)
{
    util::fourcc fcc(m_af.getFileFormat());
    m_af.getDataFormat(&m_asbd);
    setRange(0, m_af.getAudioDataPacketCount());
    std::shared_ptr<AudioChannelLayout> acl;
    try {
	m_af.getChannelLayout(&acl);
	chanmap::getChannels(acl.get(), &m_chanmap);
    } catch (...) {}
    try {
	if (fcc == 'AIFF')
	    ID3::fetchAiffID3Tags(m_stream->name(), &m_tags);
	else
	    audiofile::fetchTags(m_af, m_stream.get(), &m_tags);
    } catch (...) {}
}

size_t AFSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    UInt32 ns = nsamples;
    UInt32 nb = ns * m_asbd.mBytesPerFrame;
    CHECKCA(AudioFileReadPackets(m_af, false, &nb, 0,
		m_offset + getSamplesRead(), &ns, buffer));
    addSamplesRead(ns);
    return ns;
}


ExtAFSource::ExtAFSource(AudioFileX &af, std::shared_ptr<InputStream> &stream,
			 const std::wstring &path)
    : m_af(af), m_stream(stream)
{
    std::vector<uint8_t> cookie;
    ExtAudioFileRef eaf;
    CHECKCA(ExtAudioFileWrapAudioFileID(m_af, false, &eaf));
    m_eaf.attach(eaf, true);
    AudioStreamBasicDescription asbd;
    m_af.getDataFormat(&asbd);
    if (asbd.mFormatID != 'alac')
	throw std::runtime_error("Not supported format");

    unsigned bits_per_channel;
    {
	unsigned tab[] = { 16, 20, 24, 32 };
	unsigned index = (asbd.mFormatFlags - 1) & 0x3;
	bits_per_channel = tab[index];
    }
    m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate, asbd.mChannelsPerFrame,
				bits_per_channel,
				kAudioFormatFlagIsSignedInteger,
				kAudioFormatFlagIsAlignedHigh);
    m_eaf.setClientDataFormat(m_asbd);
    setRange(0, m_eaf.getFileLengthFrames());
    std::shared_ptr<AudioChannelLayout> acl;
    try {
	m_af.getChannelLayout(&acl);
	chanmap::getChannels(acl.get(), &m_chanmap);
    } catch (...) {}
    if (m_af.getFileFormat() == 'm4af') {
	MP4FileX mp4file;
	mp4file.Read(strutil::w2us(path).c_str(), 0);
	mp4a::fetchTags(mp4file, &m_tags);
    } else {
	try {
	    audiofile::fetchTags(m_af, m_stream.get(), &m_tags);
	} catch (...) {}
    }
}

size_t ExtAFSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    size_t processed = 0;
    uint8_t *bp = static_cast<uint8_t*>(buffer);
    while (processed < nsamples) {
	UInt32 ns = nsamples - processed;
	UInt32 nb = ns * m_asbd.mBytesPerFrame;
	AudioBufferList abl = { 0 };
	abl.mNumberBuffers = 1;
	abl.mBuffers[0].mData = bp;
	abl.mBuffers[0].mDataByteSize = nb;
	CHECKCA(ExtAudioFileRead(m_eaf, &ns, &abl));
	if (ns == 0) break;
	processed += ns;
	bp += ns * m_asbd.mBytesPerFrame;
	addSamplesRead(ns);
    }
    return processed;
}

void ExtAFSource::skipSamples(int64_t count)
{
    SInt64 pos;
    CHECKCA(ExtAudioFileTell(m_eaf, &pos));
    CHECKCA(ExtAudioFileSeek(m_eaf, pos + count));
}

