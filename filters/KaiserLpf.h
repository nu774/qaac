#ifndef KAISERLPF_H
#define KAISERLPF_H

#include <vector>

/*
 * Kaiser-window lowpass FIR designer, using the classical Kaiser/Rabiner
 * approximation formulas for the window shape parameter and tap count
 * (the same formulas implemented independently by e.g.
 * scipy.signal.kaiserord, and found in standard DSP references such as
 * Oppenheim & Schafer, "Discrete-Time Signal Processing").
 */
namespace KaiserLpf {
    /*
     * Fp: passband edge, Hz
     * Fs: stopband edge, Hz (Fs > Fp)
     * Fn: Nyquist frequency, Hz (Fn > 0, Fs <= Fn)
     * attenuationDb: desired stopband attenuation, dB
     *
     * Returns the filter taps; the tap count is derived automatically from
     * the transition width (Fs - Fp) and attenuationDb.
     */
    std::vector<double> design(double Fp, double Fs, double Fn,
                               double attenuationDb);
}

#endif
