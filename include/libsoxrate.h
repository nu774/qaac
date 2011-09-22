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

enum lsx_rate_config_e {
    SOX_RATE_QUALITY, /* 0:mid, 1:high:, 2:very high */
    SOX_RATE_PHASE_RESPONSE, /* 0:minimum, 1:intermediate, 2:linear */
    SOX_RATE_BANDWIDTH, /* double. 80-99.7 */
    SOX_RATE_ALLOW_ALIASING, /* 0:deny, 1:allow */
};

lsx_rate_t *lsx_rate_create(unsigned nchannels,
	unsigned in_rate, unsigned out_rate);
void lsx_rate_close(lsx_rate_t *state);
int lsx_rate_config(lsx_rate_t *state, enum lsx_rate_config_e type, ...);
void lsx_rate_start(lsx_rate_t *state);
size_t lsx_rate_process(lsx_rate_t *state, const float *ibuf, float *obuf,
	size_t *ilen, size_t *olen);

#ifdef __cplusplus
}
#endif

#endif /* SOXRATE_H */
