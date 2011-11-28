#include <stdint.h>
#include <ALACBitUtilities.h>
#include "strcnv.h"
#include "utf8_codecvt_facet.hpp"
#include "alacsrc.h"
#include "itunetags.h"
#include "CoreAudioHelper.h"

ALACSource::ALACSource(const std::wstring &path)
    : m_position(0)
{
    try {
	m_file.Read(w2m(path, utf8_codecvt_facet()).c_str(), 0);
	m_track_id = m_file.FindTrackId(0, MP4_AUDIO_TRACK_TYPE);
	const char *type = m_file.GetTrackMediaDataName(m_track_id);
	if (std::strcmp(type, "alac"))
	    throw std::runtime_error("Not an ALAC file");

	const char *alacprop, *chanprop;
	const char *brand = m_file.GetStringProperty("ftyp.majorBrand");
	if (!std::strcmp(brand, "qt  ")) {
	    alacprop = "mdia.minf.stbl.stsd.alac.wave.alac.decoderConfig";
	    chanprop = "mdia.minf.stbl.stsd.alac.wave.chan.data";
	} else {
	    alacprop = "mdia.minf.stbl.stsd.alac.alac.decoderConfig";
	    chanprop = "mdia.minf.stbl.stsd.alac.chan.data";
	}

	std::vector<uint8_t> alac, chan;
	uint8_t *value;
	uint32_t size;
	m_file.GetTrackBytesProperty(m_track_id, alacprop, &value, &size);
	std::copy(value + 4, value + size, std::back_inserter(alac));
	MP4Free(value);
	value = 0;
	try {
	    m_file.GetTrackBytesProperty(m_track_id, chanprop, &value, &size);
	    std::copy(value + 4, value + size, std::back_inserter(chan));
	    MP4Free(value);
	} catch (...) {}
	if (alac.size() != 24 || (chan.size() && chan.size() != 12))
	    throw std::runtime_error("ALACSource: invalid magic cookie");

	std::memset(&m_format, 0, sizeof m_format);
	m_format.m_type = SampleFormat::kIsSignedInteger;
	m_format.m_endian = SampleFormat::kIsLittleEndian;
	m_format.m_nchannels = alac[9];
	m_format.m_bitsPerSample = alac[5];
	if (m_format.m_bitsPerSample == 20)
	    m_format.m_bitsPerSample = 24;
	uint32_t timeScale;
	std::memcpy(&timeScale, &alac[20], 4);
	timeScale = b2host32(timeScale);
	m_format.m_rate = timeScale;

	m_decoder = x::shared_ptr<ALACDecoder>(new ALACDecoder());
	CHECKCA(m_decoder->Init(&alac[0], alac.size()));
	setRange(0, m_file.GetTrackDuration(m_track_id));
	fetchTags();
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

size_t ALACSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    try {
	uint32_t bpf = m_format.bytesPerFrame();
	uint8_t *bufp = static_cast<uint8_t*>(buffer);
	uint32_t nread = 0;

	uint32_t size = std::min(m_buffer.rest(), nsamples);
	if (size) {
	    std::memcpy(bufp, &m_buffer.v[m_buffer.done * bpf], size * bpf);
	    m_buffer.advance(size);
	    nread += size;
	    bufp += size * bpf;
	}
	while (nread < nsamples) {
	    MP4SampleId sid =
		m_file.GetSampleIdFromTime(m_track_id, m_position);
	    uint32_t size = m_file.GetSampleSize(m_track_id, sid);
	    MP4Timestamp start;
	    MP4Duration duration;
	    std::vector<uint8_t> ivec(size);
	    uint8_t *vp = &ivec[0];

	    m_file.ReadSample(m_track_id, sid, &vp, &size, &start, &duration);
	    m_buffer.done = m_position - start;
	    m_position = start + duration;

	    BitBuffer bits;
	    BitBufferInit(&bits, &ivec[0], size);
	    m_buffer.v.resize(bpf * duration);
	    uint32_t ncount;
	    CHECKCA(m_decoder->Decode(&bits, &m_buffer.v[0], duration,
			m_format.m_nchannels, &ncount));
	    m_buffer.nsamples = ncount;

	    duration = std::min(ncount - m_buffer.done, nsamples - nread);
	    std::memcpy(bufp, &m_buffer.v[m_buffer.done * bpf], duration * bpf);
	    m_buffer.advance(duration);
	    nread += duration;
	    bufp += duration * bpf;
	}
	addSamplesRead(nread);
	return nread;
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static
std::wstring parseValue(uint32_t fcc, const MP4ItmfData &data,
	std::codecvt<wchar_t, char, std::mbstate_t> &codec)
{
    uint8_t *value = data.value;

    if (fcc == Tag::kGenreID3) {
	int v = (value[0] << 8) | value[1];
	return widen(format("%d", v));
    } else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
	int index = (value[2] << 8) | value[3];
	int total = (value[4] << 8) | value[5];
	return widen(format("%d/%d", index, total));
    } else if (data.typeCode == MP4_ITMF_BT_INTEGER) {
	int v;
	if (data.valueSize == 1)
	    v = value[0];
	else if (data.valueSize == 2)
	    v = (value[0] << 8) | value[1];
	else if (data.valueSize == 4)
	    v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
	else
	    return L"";
	return widen(format("%d", v));
    } else if (data.typeCode == MP4_ITMF_BT_UTF8) {
	char *vp = reinterpret_cast<char*>(value);
	std::string s(vp, vp + data.valueSize);
	return m2w(s, codec);
    }
    return L"";
}

void ALACSource::fetchTags()
{
    utf8_codecvt_facet u8codec;
    try {
	MP4ItmfItemList *itemlist = mp4v2::impl::itmf::genericGetItems(m_file);
	if (!itemlist)
	    return;
	x::shared_ptr<MP4ItmfItemList> __delete_later__(
		itemlist, mp4v2::impl::itmf::genericItemListFree);
	for (size_t i = 0; i < itemlist->size; ++i) {
	    MP4ItmfItem &item = itemlist->elements[i];
	    uint32_t fcc = fourcc(item.code);
	    MP4ItmfData &data = item.dataList.elements[0];
	    if (!data.value)
		continue;
	    if (fcc != '----') {
		std::wstring v = parseValue(fcc, data, u8codec);
		if (!v.empty()) m_tags[fcc] = v;
	    }
	}
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}
