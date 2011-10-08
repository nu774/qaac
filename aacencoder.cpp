#include "aacencoder.h"
#include "aacconfig.h"

void AACEncoder::setEncoderParameter(const wchar_t *key, int value)
{
    aac::SetParameter(this, key, value);
    getBasicDescription(&m_output_desc);
}

int AACEncoder::getParameterRange(const wchar_t *key,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    return aac::GetParameterRange(this, key, result, limits);
}

void AACEncoder::getGaplessInfo(GaplessInfo *info) const
{
    info->delay = 0x840;
    info->samples = m_samples_read;
    if (m_input_desc.mSampleRate != m_output_desc.mSampleRate)
	info->samples = static_cast<uint64_t>(m_samples_read *
	    (m_output_desc.mSampleRate / m_input_desc.mSampleRate));
    uint32_t frame_length = m_output_desc.mFramesPerPacket;
    if (m_output_desc.mFormatID == 'aach') {
	info->samples /= 2;
	frame_length /= 2;
    }
    info->padding = static_cast<uint32_t>(
	    m_frames_written * frame_length - info->delay - info->samples);
}

/*
 * Workaround for QT >=7.6.9 channel mapping problem.
 * QT doesn't automatically re-arrange channels now, therefore
 * we have to manually map to AAC order.
 * Basically we re-arrange the channels at the source level,
 * an tell QT that it's layout is already in AAC order.
 */
void AACEncoder::forceAACChannelMapping()
{
    AudioChannelLayoutX layout;
    getInputChannelLayout(&layout);
    std::vector<uint32_t> chanmap;
    uint32_t newtag = GetAACChannelMap(layout, &chanmap);
    if (!newtag) return;

    AudioChannelLayoutX newLayout;
    newLayout->mChannelLayoutTag = newtag;
    setInputChannelLayout(newLayout);
    x::shared_ptr<ISource> newsrc(new ChannelMapper(m_src, chanmap));
    m_src = newsrc;
}
