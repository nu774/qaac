#ifndef MPA_H
#define MPA_H

#include <cstdint>
#include <cstring>
#include <string>

struct MPAHeader {
    unsigned syncword: 11;
    unsigned IDex: 1;
    unsigned ID: 1;
    unsigned layer: 2;
    unsigned protection_bit: 1;
    unsigned bitrate_index: 4;
    unsigned sampling_frequency: 2;
    unsigned padding_bit: 1;
    unsigned private_bit: 1;
    unsigned mode: 2;
    unsigned mode_extension: 2;
    unsigned copywrite: 1;
    unsigned original: 1;
    unsigned emphasis: 2;

    MPAHeader() { std::memset(this, 0, sizeof(MPAHeader)); }
    MPAHeader(const uint8_t *data)
    {
        fill(data);
    }
    void fill(const uint8_t *data);
    void render(uint8_t *data) const;

    int is_mono() const
    {
        return mode == 3;
    }
    uint16_t bitrate() const
    {
        static const uint16_t tab[][3][15] = {
            {
                {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
                {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
                {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,}
            },
            {
                {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,},
                {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
                {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,}
            },
        };
        return tab[ID][layer - 1][bitrate_index];
    }
    uint16_t sample_rate() const
    {
        static const uint16_t tab[][2][3] = {
            {
                { 11025, 12000,  8000 },
                {     0,     0,     0 }
            },
            {
                { 22050, 24000, 16000 },
                { 44100, 48000, 32000 }
            }
        };
        return tab[IDex][ID][sampling_frequency];
    }
    uint16_t samples_per_frame() const
    {
        static const uint32_t tab[][3] = {
            {  576, 1152, 384 },
            { 1152, 1152, 384 }
        };
        return tab[ID][layer - 1];
    }
    uint16_t frame_size() const
    {
        static const uint32_t spfdiv8[][3] = {
            {  72, 144, 12 },
            { 144, 144, 12 }
        };
        static const uint32_t slot_size[3] = { 1, 1, 4 };
        return (spfdiv8[ID][layer - 1] * bitrate() * 1000 / sample_rate()
                + padding_bit) * slot_size[layer - 1];
    }
    bool has_crc() const
    {
        return protection_bit ? false : true;
    }
    uint8_t side_info_start() const
    {
        return has_crc() ? 6 : 4;
    }
    uint8_t side_info_size() const
    {
        static const uint32_t tab[][2] = { { 17, 9 }, { 32, 17 } };
        return tab[ID][is_mono()];
    }
    uint8_t side_info_end() const
    {
        return side_info_start() + side_info_size();
    }
};

#endif
