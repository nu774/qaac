#include "libsndfilesrc.h"
#include "win32util.h"
#include "itunetags.h"
#include "cautil.h"

static
uint32_t convert_chanmap(uint32_t value)
{
    switch (value) {
	case SF_CHANNEL_MAP_MONO:
	    return 3;
	case SF_CHANNEL_MAP_LEFT: case SF_CHANNEL_MAP_FRONT_LEFT:
	    return 1;
	case SF_CHANNEL_MAP_RIGHT: case SF_CHANNEL_MAP_FRONT_RIGHT:
	    return 2;
	case SF_CHANNEL_MAP_CENTER: case SF_CHANNEL_MAP_FRONT_CENTER:
	    return 3;
	case SF_CHANNEL_MAP_LFE: return 4;
	case SF_CHANNEL_MAP_REAR_LEFT: return 5;
	case SF_CHANNEL_MAP_REAR_RIGHT: return 6;
	case SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER: return 7;
	case SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER: return 8;
	case SF_CHANNEL_MAP_REAR_CENTER: return 9;
	case SF_CHANNEL_MAP_SIDE_LEFT: return 10;
	case SF_CHANNEL_MAP_SIDE_RIGHT: return 11;
	case SF_CHANNEL_MAP_TOP_CENTER: return 12;
	case SF_CHANNEL_MAP_TOP_FRONT_LEFT: return 13;
	case SF_CHANNEL_MAP_TOP_FRONT_CENTER: return 15;
	case SF_CHANNEL_MAP_TOP_FRONT_RIGHT: return 15;
	case SF_CHANNEL_MAP_TOP_REAR_LEFT: return 16;
	case SF_CHANNEL_MAP_TOP_REAR_CENTER: return 17;
	case SF_CHANNEL_MAP_TOP_REAR_RIGHT: return 18;
	default:
	    throw std::runtime_error(strutil::format("Unknown channel: %u", value));
    }
}

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

LibSndfileModule::LibSndfileModule(const std::wstring &path)
    : m_dl(path)
{
    if (!m_dl.loaded())
	return;
    try {
	CHECK(version_string = m_dl.fetch("sf_version_string"));
	CHECK(open_virtual = m_dl.fetch("sf_open_virtual"));
	CHECK(close = m_dl.fetch("sf_close"));
	CHECK(strerror = m_dl.fetch("sf_strerror"));
	CHECK(command = m_dl.fetch("sf_command"));
	CHECK(seek = m_dl.fetch("sf_seek"));
	CHECK(read_short = m_dl.fetch("sf_read_short"));
	CHECK(read_int = m_dl.fetch("sf_read_int"));
	CHECK(read_float = m_dl.fetch("sf_read_float"));
	CHECK(read_double = m_dl.fetch("sf_read_double"));
	CHECK(close = m_dl.fetch("sf_close"));
    } catch (...) {
	m_dl.reset();
    }
}


struct SFVirtualIOImpl: public SF_VIRTUAL_IO
{
    SFVirtualIOImpl()
    {
	static SF_VIRTUAL_IO t = { size, seek, read, 0/*write*/, tell };
	std::memcpy(this, &t, sizeof t);
    }
    static sf_count_t size(void *cookie)
    {
	int fd = reinterpret_cast<int>(cookie);
	return _filelengthi64(fd);
    }
    static sf_count_t seek(sf_count_t off, int whence, void *cookie)
    {
	int fd = reinterpret_cast<int>(cookie);
	return _lseeki64(fd, off, whence);
    }
    static sf_count_t read(void *data, sf_count_t count, void *cookie)
    {
	int fd = reinterpret_cast<int>(cookie);
	return ::read(fd, data, count);
    }
    static sf_count_t tell(void *cookie)
    {
	int fd = reinterpret_cast<int>(cookie);
	return _lseeki64(fd, 0, SEEK_CUR);
    }
};

LibSndfileSource::LibSndfileSource(
	const LibSndfileModule &module, const std::shared_ptr<FILE> &fp)
    : m_module(module), m_fp(fp)
{
    static SFVirtualIOImpl vio;
    SF_INFO info = { 0 };
    SNDFILE *sf =
	m_module.open_virtual(&vio, SFM_READ, &info,
			      reinterpret_cast<void*>(fileno(m_fp.get())));
    if (!sf)
	throw std::runtime_error(m_module.strerror(0));
    m_handle.reset(sf, m_module.close);
    setRange(0, info.frames);

    SF_FORMAT_INFO finfo = { 0 };
    int count;
    m_module.command(sf, SFC_GET_FORMAT_MAJOR_COUNT, &count, sizeof count);
    for (int i = 0; i < count; ++i) {
	finfo.format = i;
	m_module.command(sf, SFC_GET_FORMAT_MAJOR, &finfo, sizeof finfo);
	if (finfo.format == (info.format & SF_FORMAT_TYPEMASK)) {
	    m_format_name = finfo.extension;
	    break;
	}
    }
    uint32_t subformat = info.format & SF_FORMAT_SUBMASK;
    struct format_table_t {
	int bits;
	int type;
    } mapping[] = {
	 { 0,  0                               },
	 { 8,  kAudioFormatFlagIsSignedInteger },
	 { 16, kAudioFormatFlagIsSignedInteger },
	 { 24, kAudioFormatFlagIsSignedInteger },
	 { 32, kAudioFormatFlagIsSignedInteger },
	 { 8,  0 /* unsigned integer */        },
	 { 32, kAudioFormatFlagIsFloat         },
	 { 64, kAudioFormatFlagIsFloat         }
    };
    if (subformat < 1 || subformat > 7)
	throw std::runtime_error("Unsupported input subformat");
    m_asbd = cautil::buildASBDForPCM(info.samplerate, info.channels,
				mapping[subformat].bits,
				mapping[subformat].type);

    m_chanmap.resize(info.channels);
    if (m_module.command(sf, SFC_GET_CHANNEL_MAP_INFO, &m_chanmap[0],
	    m_chanmap.size() * sizeof(uint32_t)) == SF_FALSE)
	m_chanmap.clear();
    else
	std::transform(m_chanmap.begin(), m_chanmap.end(),
		m_chanmap.begin(), convert_chanmap);
    if (m_format_name == "aiff") {
	try {
	    ID3::fetchAiffID3Tags(fileno(m_fp.get()), &m_tags);
	} catch (...) {}
    }
}

void LibSndfileSource::skipSamples(int64_t count)
{
    if (m_module.seek(m_handle.get(), count, SEEK_CUR) == -1)
	throw std::runtime_error("sf_seek failed");
}

#define SF_READ(type, handle, buffer, nsamples) \
    m_module.read_##type(handle, reinterpret_cast<type*>(buffer), nsamples)

size_t LibSndfileSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    nsamples *= m_asbd.mChannelsPerFrame;
    sf_count_t rc;
    uint32_t bpc = m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame;
    if (bpc == 1)
	rc = readSamples8(buffer, nsamples);
    else if (bpc == 2)
	rc = SF_READ(short, m_handle.get(), buffer, nsamples);
    else if (bpc == 3)
	rc = readSamples24(buffer, nsamples);
    else if (bpc == 8)
	rc = SF_READ(double, m_handle.get(), buffer, nsamples);
    else if (m_asbd.mFormatFlags & kAudioFormatFlagIsFloat)
	rc = SF_READ(float, m_handle.get(), buffer, nsamples);
    else
	rc = SF_READ(int, m_handle.get(), buffer, nsamples);
    nsamples = static_cast<size_t>(rc / m_asbd.mChannelsPerFrame);
    addSamplesRead(nsamples);
    return nsamples;
}

size_t LibSndfileSource::readSamples8(void *buffer, size_t nsamples)
{
    std::vector<short> v(m_asbd.mChannelsPerFrame * nsamples);
    sf_count_t rc = SF_READ(short, m_handle.get(), &v[0], nsamples);
    char *bp = reinterpret_cast<char*>(buffer);
    for (size_t i = 0; i < rc; ++i)
	*bp++ = static_cast<char>(v[i] / 256);
    return static_cast<size_t>(rc);
}

size_t LibSndfileSource::readSamples24(void *buffer, size_t nsamples)
{
    std::vector<int32_t> v(m_asbd.mChannelsPerFrame * nsamples);
    sf_count_t rc = SF_READ(int, m_handle.get(), &v[0], nsamples);
    util::MemorySink24LE sink(buffer);
    for (size_t i = 0; i < rc; ++i)
	sink.put(v[i] / 256);
    return static_cast<size_t>(rc);
}
