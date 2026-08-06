#ifndef FFT4G_FLOAT_H
#define FFT4G_FLOAT_H

/*
 * Single-precision build of Takuya Ooura's general purpose FFT package
 * (fft4g.c, http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html).
 * Only the real DFT (rdft) entry point is exposed here; qaac only needs
 * a forward/inverse real transform for FFT-based FIR convolution.
 *
 * rdft(n, 1, a, ip, w)  : forward transform, a[0..n-1] real -> half-complex
 *     a[0]     = R[0]
 *     a[1]     = R[n/2]
 *     a[2*k]   = R[k]   (0 < k < n/2)
 *     a[2*k+1] = I[k]   (0 < k < n/2)
 * rdft(n, -1, a, ip, w) : inverse of the above (unnormalized: result is
 *     scaled by n/2 and must be divided by n/2 by the caller if needed)
 *
 * ip[] must be zeroed before the first call for a given buffer; its
 * required length is 2 + (1 << ((int)(log(n/2+0.5)/log(2)) / 2)).
 * w[] must hold at least n/2 elements and is filled in on the first call.
 */

#ifdef __cplusplus
extern "C" {
#endif

void rdft(int n, int isgn, float *a, int *ip, float *w);

#ifdef __cplusplus
}
#endif

#endif /* FFT4G_FLOAT_H */
