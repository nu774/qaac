#ifndef VORBISCHANNELLAYOUT_H
#define VORBISCHANNELLAYOUT_H

#include <cstdint>

/*
 * Vorbis's own multichannel ordering (cf.
 * https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9), used both
 * by native Vorbis decode (to reorder vorbis_synthesis_pcmout()'s planar
 * channels into the WAV/SMPTE order the rest of qaac assumes) and by Opus's
 * mapping family 1 (which is defined as this same Vorbis channel order).
 * vorbis_channel_layout[channels-1][i] gives the source (Vorbis-native)
 * channel index to place at target (WAV-order) position i. Verified
 * empirically against ffmpeg's Vorbis decode for 1-6 channels.
 */
static const uint8_t vorbis_channel_layout[8][8] = {
    { 0 },
    { 0, 1 },
    { 0, 2, 1 },
    { 0, 1, 2, 3 },
    { 0, 2, 1, 3, 4 },
    { 0, 2, 1, 5, 3, 4 },
    { 0, 2, 1, 6, 5, 3, 4 },
    { 0, 2, 1, 7, 5, 6, 3, 4 },
};

#endif
