#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "alacsink.h"

ALACSink::ALACSink(const std::wstring &path, EncoderBase &encoder, bool temp)
	: MP4SinkBase(path, temp)
{
    const AudioStreamBasicDescription &format
	= encoder.getOutputBasicDescription();
    uint32_t sample_rate = static_cast<uint32_t>(format.mSampleRate);
    try {
	m_mp4file.SetTimeScale(sample_rate);
	std::vector<char> cookie;
	encoder.getMagicCookie(&cookie);
	/* XXX
	   OutputBasicDescription.mBitsPerChannel is always zero for ALAC.
	   Therefore, we use InputBasicDescription instead.
         */
	uint32_t bps = encoder.getInputBasicDescription().mBitsPerChannel;
	m_track_id = m_mp4file.AddAlacAudioTrack(sample_rate,
		bps > 16 ? 24 : 16,
		reinterpret_cast<uint8_t*>(&cookie[20]),
		cookie.size() - 28);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}
