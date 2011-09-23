/* 
 * Copyright (c) 2008 robs@users.sourceforge.net 
 * Copyright (c) 2011 nu774
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef SOXRATE_H
#define SOXRATE_H

#ifdef __cplusplus
extern "C" {
#endif

struct lsx_rate_state_tag;
typedef struct lsx_rate_state_tag lsx_rate_t;

/*
 * default configuration is:
 * quality: very high
 * phase response: linear
 * band width: 0 (which means default)
 * allow aliasing: deny
 */
enum lsx_rate_config_e {
    SOX_RATE_QUALITY,        /* 0:mid, 1:high:, 2:very high */
    SOX_RATE_PHASE_RESPONSE, /* 0:minimum, 1:intermediate, 2:linear */
    SOX_RATE_BANDWIDTH,      /* double. 80-99.7 */
    SOX_RATE_ALLOW_ALIASING, /* 0:deny, 1:allow */
};

lsx_rate_t *lsx_rate_create(unsigned nchannels,
	unsigned in_rate, unsigned out_rate);
void lsx_rate_close(lsx_rate_t *state);

/*
 * lsx_rate_config() must be called before lsx_rate_start().
 * takes 3 parameter.
 * 2nd parameter("type") is one of lsx_rate_config_e.
 * 3rd parameter varies with 2nd parameter.
 * Basically it is an integer, but SOX_RATE_BANDWIDTH takes double.
 * returns 0 on success, -1 on error (means parameter is invalid and ignored)
 */
int lsx_rate_config(lsx_rate_t *state, enum lsx_rate_config_e type, ...);

/*
 * lsx_rate_start() must be called before lsx_process().
 * This actually starts worker threads (number of threads is equal to 
 * number of channels.
 * returns 0 on success, -1 on error.
 */
int lsx_rate_start(lsx_rate_t *state);

/*
 * Actual rate conversion routine.
 *
 * ilen and olen are bidirectional parameter (I/O).
 * When calling this, you set number of samples in ibuf into ilen,
 * and capacity of obuf (counted in number of samples) into olen.
 * You can calculate as olen = (size of obuf) / (number of channels).
 *
 * When function returns, ilen points to number of consumed input samples,
 * and olen points to number of produced output samples.
 *
 * When you have no more input, just tell it by setting ilen to 0.
 * After that, when lsx_rate_process() produced no more output (olen = 0),
 * you can judge the rate conversion has finished.
 *
 * Audio samples in ibuf must be interleaved.
 */
size_t lsx_rate_process(lsx_rate_t *state, const float *ibuf, float *obuf,
	size_t *ilen, size_t *olen);

#ifdef __cplusplus
}
#endif

#endif /* SOXRATE_H */
