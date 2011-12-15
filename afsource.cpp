#include "afsource.h"
#include "win32util.h"
#include "CoreAudioHelper.h"
#include "AudioConverterX.h"
#include "mp4v2wrapper.h"
#include "strcnv.h"
#include "utf8_codecvt_facet.hpp"
#include "itunetags.h"

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

    void SampleFormat_FromASBD(const AudioStreamBasicDescription &asbd,
			       SampleFormat *format)
    {
	format->m_nchannels = asbd.mChannelsPerFrame;
	format->m_bitsPerSample = asbd.mBitsPerChannel;
	format->m_rate = asbd.mSampleRate;
	if (asbd.mFormatFlags & kAudioFormatFlagIsFloat)
	    format->m_type = SampleFormat::kIsFloat;
	else if (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)
	    format->m_type = SampleFormat::kIsSignedInteger;
	else
	    format->m_type = SampleFormat::kIsUnsignedInteger;
	if (asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
	    format->m_endian = SampleFormat::kIsBigEndian;
	else
	    format->m_endian = SampleFormat::kIsLittleEndian;
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

    void fetchTags(AudioFileX &af, std::map<uint32_t, std::wstring> *result)
    {
	CFDictionaryRef dict = af.getInfoDictionary();
	x::shared_ptr<const __CFDictionary> dictPtr(dict, CFRelease);
	CFIndex elts = CFDictionaryGetCount(dict);
	if (!elts) return;
	std::vector<const void*> keys(elts);
	std::vector<const void*> values(elts);
	CFDictionaryGetKeysAndValues(dict, &keys[0], &values[0]);
	utf8_codecvt_facet u8codec;
	std::map<uint32_t, std::wstring> tags;

	for (CFIndex i = 0; i < elts; ++i) {
	    if (CFGetTypeID(keys[i]) != CFStringGetTypeID() ||
		CFGetTypeID(values[i]) != CFStringGetTypeID())
		continue;
	    CFStringRef key = static_cast<CFStringRef>(keys[i]);
	    CFStringRef value = static_cast<CFStringRef>(values[i]);
	    std::string utf8key = w2m(CF2W(key), u8codec);
	    uint32_t id = getIDFromTagName(utf8key.c_str());
	    if (id) tags[id] = CF2W(value);
	}
	result->swap(tags);
    }
}

x::shared_ptr<ISource>
    AudioFileOpenFactory(InputStream &stream, const std::wstring &path)
{
    AudioFileID afid;
    x::shared_ptr<InputStream> streamPtr(new InputStream(stream));

    CHECKCA(AudioFileOpenWithCallbacks(
		streamPtr.get(), audiofile::read, 0,
		audiofile::size, 0, 0, &afid));
    AudioFileX af(afid, true);
    fourcc fmt(af.getFileFormat());
    AudioStreamBasicDescription asbd;
    af.getDataFormat(&asbd);
    if (asbd.mFormatID == 'lpcm')
	return x::shared_ptr<ISource>(new AFSource(af, streamPtr));
    else if (asbd.mFormatID == 'alac') {
	x::shared_ptr<ISource> src(new ExtAFSource(af, streamPtr, path));
	const SampleFormat &sf = src->getSampleFormat();
	std::vector<uint32_t> chanmap;
	uint32_t tag = GetALACLayoutTag(sf.m_nchannels);
	uint32_t bitmap = GetAACReversedChannelMap(tag, &chanmap);
	return (chanmap.size())
	    ? x::shared_ptr<ISource>(new ChannelMapper(src, chanmap, bitmap))
	    : src;
    }
    else
	throw std::runtime_error("Not supported format");
}

AFSource::AFSource(AudioFileX &af, x::shared_ptr<InputStream> &stream)
    : m_af(af), m_offset(0), m_stream(stream)
{
    AudioStreamBasicDescription asbd;
    m_af.getDataFormat(&asbd);
    audiofile::SampleFormat_FromASBD(asbd, &m_format);
    setRange(0, m_af.getAudioDataPacketCount());
    x::shared_ptr<AudioChannelLayout> acl;
    try {
	m_af.getChannelLayout(&acl);
	uint32_t bitmap = GetBitmapFromAudioChannelLayout(acl.get());
	if (bitmap && bitcount(bitmap) >= m_format.m_nchannels) {
	    for (size_t i = 0;
		 m_chanmap.size() < m_format.m_nchannels && i < 32;
		 ++i, bitmap >>= 1)
		if (bitmap & 1) m_chanmap.push_back(i + 1);
	}
    } catch (std::exception &) {}
    audiofile::fetchTags(m_af, &m_tags);
}

size_t AFSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    UInt32 ns = nsamples;
    UInt32 nb = ns * m_format.bytesPerFrame();
    CHECKCA(AudioFileReadPackets(m_af, false, &nb, 0,
		m_offset + getSamplesRead(), &ns, buffer));
    addSamplesRead(ns);
    return ns;
}


ExtAFSource::ExtAFSource(AudioFileX &af, x::shared_ptr<InputStream> &stream,
			 const std::wstring &path)
    : m_af(af), m_stream(stream)
{
    std::vector<uint8_t> cookie;
    ExtAudioFileRef eaf;
    CHECKCA(ExtAudioFileWrapAudioFileID(m_af, false, &eaf));
    m_eaf.attach(eaf, true);
    AudioStreamBasicDescription asbd, oasbd;
    m_af.getDataFormat(&asbd);
    unsigned bytesPerSample =
	asbd.mFormatFlags == 1 ? 2 : asbd.mFormatFlags == 4 ? 4 : 3;
    oasbd = asbd;
    oasbd.mFormatID = 'lpcm';
    oasbd.mFormatFlags =
	kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    oasbd.mBitsPerChannel = bytesPerSample << 3;
    oasbd.mFramesPerPacket = 1;
    oasbd.mBytesPerPacket = oasbd.mBytesPerFrame =
	oasbd.mChannelsPerFrame * bytesPerSample; 
    m_eaf.setClientDataFormat(oasbd);
    audiofile::SampleFormat_FromASBD(oasbd, &m_format);
    setRange(0, m_eaf.getFileLengthFrames());
    if (m_af.getFileFormat() == 'm4af') {
	MP4FileX mp4file;
	mp4file.Read(w2m(path, utf8_codecvt_facet()).c_str(), 0);
	mp4a::fetchTags(mp4file, &m_tags);
    } else
	audiofile::fetchTags(m_af, &m_tags);
}

size_t ExtAFSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    size_t processed = 0;
    uint8_t *bp = static_cast<uint8_t*>(buffer);
    while (processed < nsamples) {
	UInt32 ns = nsamples - processed;
	UInt32 nb = ns * m_format.bytesPerFrame();
	AudioBufferList abl = { 0 };
	abl.mNumberBuffers = 1;
	abl.mBuffers[0].mData = bp;
	abl.mBuffers[0].mDataByteSize = nb;
	CHECKCA(ExtAudioFileRead(m_eaf, &ns, &abl));
	if (ns == 0) break;
	processed += ns;
	bp += ns * m_format.bytesPerFrame();
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

