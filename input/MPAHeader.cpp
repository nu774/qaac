#include <stdexcept>
#include "MPAHeader.h"

void MPAHeader::fill(const uint8_t *data)
{
    uint32_t hd = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
    syncword           = hd >> 21;
    IDex               = (hd >> 20) & 0x1;
    ID                 = (hd >> 19) & 0x1;
    layer              = (hd >> 17) & 0x3;
    protection_bit     = (hd >> 16) & 0x1;
    bitrate_index      = (hd >> 12) & 0xf;
    sampling_frequency = (hd >> 10) & 0x3;
    padding_bit        = (hd >>  9) & 0x1;
    private_bit        = (hd >>  8) & 0x1;
    mode               = (hd >>  6) & 0x3;
    mode_extension     = (hd >>  4) & 0x3;
    copywrite          = (hd >>  3) & 0x1;
    original           = (hd >>  2) & 0x1;
    emphasis           = hd & 0x3;

    if (syncword != 0x7ff
     || (IDex == 0 && ID == 1)
     || layer == 0
     || sampling_frequency == 3
     || bitrate_index == 15)
        throw std::runtime_error("Invalid MPEG Frame Header");
}

void MPAHeader::render(uint8_t *data) const
{
    uint32_t hd = 0;
    hd |= syncword           << 21;
    hd |= IDex               << 20;
    hd |= ID                 << 19;
    hd |= layer              << 17;
    hd |= protection_bit     << 16;
    hd |= bitrate_index      << 12;
    hd |= sampling_frequency << 10;
    hd |= padding_bit        <<  9;
    hd |= private_bit        <<  8;
    hd |= mode               <<  6;
    hd |= mode_extension     <<  4;
    hd |= copywrite          <<  3;
    hd |= original           <<  2;
    hd |= emphasis;
    data[0] = hd >> 24;
    data[1] = hd >> 16;
    data[2] = hd >>  8;
    data[3] = hd;
}
