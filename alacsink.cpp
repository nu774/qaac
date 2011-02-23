#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "alacsink.h"

ALACSink::ALACSink(const std::wstring &path, EncoderBase &encoder)
	: m_filename(path),
	  m_closed(false)
{
    static const char * const compatibleBrands[] = { "M4A ", "mp42" };

    const AudioStreamBasicDescription &format
	= encoder.getOutputBasicDescription();
    uint32_t sample_rate = static_cast<uint32_t>(format.mSampleRate);
    try {
	m_mp4file.Create(
		w2m(path, utf8_codecvt_facet()).c_str(),
		0, // flags
		1, // add_ftypes
		0, // add_iods
		"M4A ", // majorBrand
		0, // minorVersion
		const_cast<char**>(compatibleBrands), 
		array_size(compatibleBrands));

	m_mp4file.SetTimeScale(sample_rate);
	std::vector<char> cookie;
	encoder.getMagicCookie(&cookie);
	m_track_id = m_mp4file.AddAlacAudioTrack(sample_rate,
		reinterpret_cast<uint8_t*>(&cookie[20]),
		cookie.size() - 28);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

void ALACSink::close()
{
    if (!m_closed) {
	m_closed = true;
	try {
	    m_mp4file.Close();
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
}
