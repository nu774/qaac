#include "KaiserLpf.h"
#include <cmath>
#include <stdexcept>

namespace {

const double kPi = 3.14159265358979323846;

/* Modified Bessel function of the first kind, order 0, via its power
 * series; iterate until the running sum stops changing in double
 * precision. */
double besselI0(double x)
{
    double halfX = x / 2.0;
    double term = 1.0, sum = 1.0, lastSum;
    for (int k = 1; ; ++k) {
        double y = halfX / k;
        term *= y * y;
        lastSum = sum;
        sum += term;
        if (sum == lastSum)
            return sum;
    }
}

/* Classical Kaiser window shape parameter from the desired stopband
 * attenuation (dB). */
double kaiserBeta(double attenuationDb)
{
    if (attenuationDb > 50.0)
        return 0.1102 * (attenuationDb - 8.7);
    if (attenuationDb >= 21.0)
        return 0.5842 * std::pow(attenuationDb - 21.0, 0.4)
             + 0.07886 * (attenuationDb - 21.0);
    return 0.0;
}

} // namespace

std::vector<double> KaiserLpf::design(double Fp, double Fs, double Fn,
                                      double attenuationDb)
{
    if (!(Fn > 0.0) || !(Fp > 0.0) || !(Fs > Fp) || !(Fs <= Fn))
        throw std::runtime_error("KaiserLpf::design: invalid frequency spec");

    double fp = Fp / Fn, fs = Fs / Fn;   /* normalize: Nyquist == 1 */
    double fc = 0.5 * (fp + fs);         /* ideal brick-wall cutoff */
    double omega = kPi * (fs - fp);     /* transition width, rad/sample */

    double beta = kaiserBeta(attenuationDb);
    double estimate = (attenuationDb - 7.95) / (2.285 * omega) + 1.0;
    size_t numTaps = estimate > 1.0 ? static_cast<size_t>(std::ceil(estimate))
                                     : 1;

    std::vector<double> h(numTaps);
    double m = static_cast<double>(numTaps - 1);
    double invI0Beta = 1.0 / besselI0(beta);

    for (size_t n = 0; n < numTaps; ++n) {
        double z = n - 0.5 * m;
        double x = kPi * fc * z;
        double sinc = (x != 0.0) ? std::sin(x) / x : 1.0;
        double windowArg = (m > 0.0) ? 2.0 * z / m : 0.0;
        double window = besselI0(beta * std::sqrt(1.0 - windowArg * windowArg))
                       * invI0Beta;
        h[n] = fc * sinc * window;
    }
    return h;
}
