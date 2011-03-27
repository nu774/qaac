#include <cstdio>
#include "iointer.h"

static
void die() { throw std::runtime_error("Invalid sample format"); }

SampleFormat::SampleFormat(const char *spec, unsigned nchannels, unsigned rate)
    : m_nchannels(nchannels), m_rate(rate)
{
    char c_type, c_endian;
    if (m_nchannels == 0 || m_nchannels > 8 || m_rate == 0)
	die();
    if (std::sscanf(spec, "%c%d%c",
		&c_type, &m_bitsPerSample, &c_endian) != 3)
	die();
    if ((m_type = strindex("SUF", toupper(c_type & 0xff))) == -1)
	die();
    if ((m_endian = strindex("LB", toupper(c_endian & 0xff))) == -1)
	die();
    if (!m_bitsPerSample || (m_bitsPerSample & 0x7)) die();
    if (m_type == kIsFloat && (m_bitsPerSample & 0x1f)) die();
}
