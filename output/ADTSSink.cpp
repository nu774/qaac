#include <algorithm>
#include "ADTSSink.h"
#include "bitstream.h"

static
size_t parseDecSpecificConfig(const std::vector<uint8_t> &config,
                              unsigned *sampling_rate_index,
                              unsigned *sampling_rate, unsigned *channel_config)
{
    static const unsigned tab[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
        16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };
    BitStream bs(const_cast<uint8_t*>(&config[0]), config.size());
    bs.get(5); // objtype
    *sampling_rate_index = bs.get(4);
    if (*sampling_rate_index == 15)
        *sampling_rate = bs.get(24);
    else
        *sampling_rate = tab[*sampling_rate_index];
    *channel_config = bs.get(4);
    return bs.position();
}

ADTSSink::ADTSSink(const std::string &path,
                   const std::vector<uint8_t> &cookie,
                   bool append)
    : m_fp(platform::openFileOrStd(path, append ? "ab" : "wb"))
{
    init(cookie);
}

ADTSSink::ADTSSink(const std::shared_ptr<FILE> &fp,
                   const std::vector<uint8_t> &cookie)
    : m_fp(fp)
{
    init(cookie);
}

void ADTSSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    BitStream bs;
    bs.put(0xfff, 12); // syncword
    bs.put(0, 1);  // ID(MPEG identifier). 0 for MPEG4, 1 for MPEG2
    bs.put(0, 2);  // layer. always 0
    bs.put(1, 1);  // protection absent. 1 means no CRC information
    bs.put(1, 2);  // profile, (MPEG-4 object type) - 1. 1 for AAC LC
    bs.put(m_sample_rate_index, 4); // sampling rate index
    bs.put(0, 1); // private bit
    bs.put(m_channel_config, 3); // channel configuration
    bs.put(0, 4); /*
                   * originaly/copy: 1
                   * home: 1
                   * copyright_identification_bit: 1
                   * copyright_identification_start: 1
                   */
    bs.put(length + m_pce_data.size() + 7, 13); // frame_length
    bs.put(0x7ff, 11); // adts_buffer_fullness, 0x7ff for VBR
    bs.put(0, 2); // number_of_raw_data_blocks_in_frame
    bs.byteAlign();

    write(bs.data(), 7);
    if (m_pce_data.size())
        write(&m_pce_data[0], m_pce_data.size());
    write(data, length);
}

void ADTSSink::init(const std::vector<uint8_t> &config)
{
    m_seekable = platform::is_seekable(fileno(m_fp.get()));
    unsigned rate;
    size_t off = parseDecSpecificConfig(config, &m_sample_rate_index, &rate,
                                        &m_channel_config);

    /* keep program config element stored in GASpecificConfig */
    if (m_channel_config == 0 && config.size() * 8 > off) {
        BitStream ibs(config.data(), config.size());
        ibs.advance(off + 3);
        BitStream obs;
        obs.put(5, 3); /* ID_PCE */
        obs.copy(ibs, 4+2+4); /* element_instance_tag, object_type, sf_index */

        /* number of channels */
        unsigned nfront, nside, nback, nlfe, ndata, ncc;
        nfront = obs.copy(ibs, 4);
        nside  = obs.copy(ibs, 4);
        nback  = obs.copy(ibs, 4);
        nlfe   = obs.copy(ibs, 2);
        ndata  = obs.copy(ibs, 3);
        ncc    = obs.copy(ibs, 4);

        if (obs.copy(ibs, 1)) obs.copy(ibs, 4); /* mono_mixdown */
        if (obs.copy(ibs, 1)) obs.copy(ibs, 4); /* stereo_mixdown */
        if (obs.copy(ibs, 1)) obs.copy(ibs, 2+1); /* matrix_mixdown */

        /* channel data */
        for (int i = 0; i < nfront + nside + nback; ++i)
            obs.copy(ibs, 1+4);
        for (int i = 0; i < nlfe + ndata; ++i)
            obs.copy(ibs, 4);
        for (int i = 0; i < ncc; ++i)
            obs.copy(ibs, 1+4);

        obs.byteAlign();
        obs.put(0, 8); /* comment_field_bytes */

        std::copy(obs.data(), obs.data() + obs.position() / 8,
                  std::back_inserter(m_pce_data));
    }
}
