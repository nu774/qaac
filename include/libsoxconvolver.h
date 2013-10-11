/* 
 * Copyright (c) 2008 robs@users.sourceforge.net 
 * Copyright (c) 2013 nu774
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
#ifndef SOXCONVOLVER_H
#define SOXCONVOLVER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lsx_convolver_t lsx_convolver_t;

lsx_convolver_t *lsx_convolver_create(unsigned nchannels,
                                      double *coefs, unsigned ncoefs,
                                      unsigned post_peak);

void lsx_convolver_close(lsx_convolver_t *state);

void lsx_convolver_process(lsx_convolver_t *state, const float *ibuf,
                           float *obuf, size_t *ilen, size_t *olen);

void lsx_convolver_process_ni(lsx_convolver_t *state,
                              const float * const *ibuf, float **obuf,
                              size_t istride, size_t ostride,
                              size_t *ilen, size_t *olen);

double * lsx_design_lpf(
    double Fp,      /* End of pass-band; ~= 0.01dB point */
    double Fc,      /* Start of stop-band */
    double Fn,      /* Nyquist freq; e.g. 0.5, 1, PI */
    double att,     /* Stop-band attenuation in dB */
    int * num_taps, /* (Single phase.)  0: value will be estimated */
    int k,          /* Number of phases; 0 for single-phase */
    double beta);

const char *lsx_convolver_version_string();

void lsx_free(void *ptr);


#ifdef __cplusplus
}
#endif

#endif /* SOXCONVOLVER_H */
