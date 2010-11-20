#include "wavsink.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

void BuildWaveFormat(const SampleFormat &format,
	const std::vector<uint32_t> *chanmap, WAVEFORMATEXTENSIBLE *result)
{
    if (format.m_nchannels > 8)
	throw std::runtime_error("Sorry, can't handle more than 8 channels");
    if (format.m_bitsPerSample == 8 &&
	    format.m_type == SampleFormat::kIsSignedInteger)
	throw std::runtime_error("Sorry, can't handle 8bit signed LPCM");
    if (format.m_endian == SampleFormat::kIsBigEndian)
	throw std::runtime_error("Sorry, can't handle big endian LPCM");

    WAVEFORMATEXTENSIBLE wfx = { 0 };
    WAVEFORMATEX &wf = wfx.Format;
    if (format.m_nchannels <= 2) {
	if (format.m_type != SampleFormat::kIsFloat)
	    wf.wFormatTag = WAVE_FORMAT_PCM;
	else
	    wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    }
    wf.nChannels = format.m_nchannels;
    wf.nSamplesPerSec = format.m_rate;
    wf.wBitsPerSample = format.m_bitsPerSample;
    wf.nBlockAlign = format.bytesPerFrame();
    wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
    if (format.m_nchannels > 2) {
	wf.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wf.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	if (chanmap)
	    for (size_t i = 0; i < chanmap->size(); ++i)
		wfx.dwChannelMask |= (1 << (chanmap->at(i) - 1));
	else {
	    static uint32_t default_map[] = {
		1, 2, 7, 0x107, 0x37, 0x3f, 0x70f, 0x63f
	    };
	    wfx.dwChannelMask = default_map[format.m_nchannels - 1];
	}
	if (format.m_type == SampleFormat::kIsFloat)
	    wfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	else
	    wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    *result = wfx;
}

WavSink::WavSink(FILE *fp,
		 uint64_t duration,
		 const SampleFormat &format,
		 const std::vector<uint32_t> *chanmap)
	: m_file(fp)
{
    WAVEFORMATEXTENSIBLE wfx;
    BuildWaveFormat(format, chanmap, &wfx);

    uint32_t hdrsize = wfx.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE
	? sizeof wfx : offsetof(WAVEFORMATEX, cbSize);
    uint64_t datasize_ = duration * format.bytesPerFrame();
    uint64_t riffsize_ = hdrsize + datasize_ + 20; 
    uint32_t datasize = static_cast<uint32_t>(datasize_); // XXX
    uint32_t riffsize = static_cast<uint32_t>(riffsize_); // XXX

    write("RIFF", 4);
    write(&riffsize, 4);
    write("WAVEfmt ", 8);
    write(&hdrsize, 4);
    write(&wfx, hdrsize);
    write("data", 4);
    write(&datasize, 4);
}
