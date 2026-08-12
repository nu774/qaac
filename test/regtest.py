#!/usr/bin/env python3
"""Gapless + fidelity regression test for qaac's AAC-LC / AAC-HE output
(MP4 and CAF).

Two subcommands, meant to be driven by CI around an actual qaac encode
followed by a qaac *self*-decode (`qaac -D --bits-per-sample 16`) --
not e.g. ffmpeg: ffmpeg's own MP4/AAC gapless (edit list) handling is
known incomplete (it doesn't trim the trailing remainder when
transcoding to PCM, and for HE-AAC it also doesn't compensate for the
SBR decoder's own extra algorithmic delay on top of the AAC encoder's
delay -- both of which qaac's own decoder does handle), so it would
give a spuriously-misaligned/wrong-length comparison even for a
correct encode.

  gen     Write a short multi-tone mono WAV test signal. The signal has
          content both below and above the HE-AAC SBR crossover band
          (~sample_rate/4) so both the AAC-LC-coded core band and the
          SBR-reconstructed band carry real energy to check.

  check   Compare an encode->decode round trip against the original:
            1. exact sample-count match -- this is not a fuzzy metric,
               it IS the gapless contract.
            2. waveform fidelity, as a *spectral magnitude* SNR (band-
               limited to the AAC-LC core band for HE-AAC, since SBR is
               a spectral approximation of the upper band, not a
               waveform match) rather than a time-domain one: a pure
               sample delay only shifts phase, not magnitude, so this
               sidesteps any alignment question entirely instead of
               chasing it with a correlation search (which turned out to
               be unreliable for this signal: its tones are periodic
               enough that a search can lock onto a plausible-looking
               but wrong lag). A correct round trip is actually sample-
               exact (see the seekTo() fix below), but this metric is
               kept anyway since it's a strictly more robust technique.
            3. (HE only) high-band energy is not collapsed/silent, which
               "2." alone would never catch since it explicitly excludes
               that band.

This test previously found a real qaac decode bug: MMTISOBMFFSource::seekTo()
didn't clear its internal decode buffer, so the redundant seekTo(0) calls
that happen on every normal decode start (once from the source's own
constructor, once from the generic pipeline) queued the post-priming preroll
samples twice, corrupting the first ~20ms of every MP4 decode and silently
truncating the last equivalent amount. Fixed in MMTISOBMFFSource.cpp. This is
why the fidelity numbers below now match between MP4 and CAF -- they didn't
before that fix, which is precisely what exposed it.
"""
import argparse
import sys
import wave

import numpy as np


def read_wav_mono_f64(path):
    with wave.open(path, "rb") as w:
        if w.getsampwidth() != 2:
            raise ValueError(f"{path}: expected 16-bit PCM, got {w.getsampwidth()*8}-bit")
        n = w.getnframes()
        ch = w.getnchannels()
        sr = w.getframerate()
        raw = w.readframes(n)
    data = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    if ch > 1:
        data = data.reshape(-1, ch).mean(axis=1)
    return data, sr


def write_wav_mono_i16(path, data, sr):
    clipped = np.clip(data, -1.0, 1.0)
    ints = np.round(clipped * 32767.0).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sr)
        w.writeframes(ints.tobytes())


AAC_FRAME = 1024
# CoreAudioToolbox's AAC encoder delay, in core-rate samples -- the same
# constant for both LC and HE (confirmed via iTunSMPB: both show priming
# 0x840 = 2112).
HE_ENCODER_PRIMING_CORE = 2112


def samples_for_he_tail_padding(padding, sr, k=10):
    """Full-rate sample count that leaves exactly `padding` core-rate
    samples of natural trailing padding after the last full 1024-sample
    core-rate AAC frame, for an HE-AAC encode at sample rate `sr`.

    HE-AAC's core coder always emits whole 1024-sample (core-rate) frames,
    so encoder priming (2112 core samples) + content, once content isn't
    itself a multiple of 1024, leaves some natural trailing padding before
    the last frame boundary. For LC that's the whole story. But HE-AAC's
    SBR decoder adds its own extra algorithmic delay on top of the
    encoder's (481 core-rate samples / 962 full-rate samples), which
    shifts the real content's decode window back by that much -- so unlike
    LC, HE-AAC's trailing padding must always be >= 481 core-rate samples,
    or there's no room left to shift into. CoreAudioToolbox's HE-AAC
    encoder doesn't guarantee that; qaac's CoreAudioPaddedEncoder covers
    the gap (LPC-extrapolating a frame at each end and adjusting
    getGaplessInfo()'s mNumberValidFrames/mRemainderFrames accordingly).
    This generates content lengths that specifically target small/edge
    values of that natural padding (down to 0), to regression-test that
    mechanism -- see the padding formula derivation in the commit that
    added this.
    """
    priming_mod = HE_ENCODER_PRIMING_CORE % AAC_FRAME
    n_core = (AAC_FRAME - priming_mod - padding) % AAC_FRAME
    n_core += AAC_FRAME * k
    return n_core * 2  # HE-AAC's core rate is exactly sr/2


def cmd_gen(args):
    sr = args.sr
    if args.he_tail_padding is not None:
        n = samples_for_he_tail_padding(args.he_tail_padding, sr)
    elif args.samples:
        n = args.samples
    else:
        n = int(round(args.duration * sr))
    if n % 2:
        n += 1  # keep the sample count even: required for an exact
                # roundtrip through HE-AAC's halved (core-rate) timescale
    t = np.arange(n) / sr
    # tones straddling the HE-AAC SBR crossover (~sr/4) so both the
    # LC-coded core band and the SBR-reconstructed band carry real content
    low_tones = [523.25, 1975.5]
    high_tones = [12000.0, 15500.0]
    sig = np.zeros(n)
    for f in low_tones + high_tones:
        sig += np.sin(2 * np.pi * f * t)
    sig /= len(low_tones) + len(high_tones)
    sig *= 0.7
    fade = min(200, n // 10)
    if fade > 0:
        ramp = np.linspace(0.0, 1.0, fade)
        sig[:fade] *= ramp
        sig[-fade:] *= ramp[::-1]
    write_wav_mono_i16(args.out, sig, sr)
    print(f"wrote {args.out}: {n} samples @ {sr}Hz")


def lowpass_fft(x, sr, cutoff):
    X = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), d=1.0 / sr)
    X[freqs > cutoff] = 0
    return np.fft.irfft(X, n=len(x))


def spectral_snr_db(ref, test):
    """SNR between the FFT magnitude spectra of ref/test (not a time-domain
    diff): invariant to any pure sample delay between the two signals,
    which only shifts phase, not magnitude. See module docstring.
    """
    n = min(len(ref), len(test))
    R = np.abs(np.fft.rfft(ref[:n]))
    T = np.abs(np.fft.rfft(test[:n]))
    err_energy = float(np.sum((R - T) ** 2))
    ref_energy = float(np.sum(R ** 2))
    if err_energy <= 0:
        return float("inf")
    if ref_energy <= 0:
        return float("-inf")
    return 10 * np.log10(ref_energy / err_energy)


def cmd_check(args):
    orig, sr_o = read_wav_mono_f64(args.orig)
    dec, sr_d = read_wav_mono_f64(args.decoded)
    if sr_o != sr_d:
        print(f"FAIL: sample rate mismatch: orig={sr_o} decoded={sr_d}")
        sys.exit(1)
    sr = sr_o
    ok = True

    # 1. exact sample count -- the actual gapless contract. Checked against
    # qaac's own decode (see module docstring for why not e.g. ffmpeg's).
    if len(orig) != len(dec):
        print(f"FAIL: gapless roundtrip length mismatch: orig={len(orig)} "
              f"decoded={len(dec)}")
        ok = False
    else:
        print(f"OK: gapless roundtrip length matches ({len(orig)} samples)")

    n = min(len(orig), len(dec))
    o, d = orig[:n], dec[:n]

    if args.mode == "he":
        cutoff = sr / 4.0
        o_band = lowpass_fft(o, sr, cutoff)
        d_band = lowpass_fft(d, sr, cutoff)
        snr = spectral_snr_db(o_band, d_band)
        print(f"HE core-band (<{cutoff:.0f}Hz) spectral SNR: {snr:.1f} dB")
        if snr < args.snr_threshold:
            print(f"FAIL: core-band spectral SNR below threshold ({args.snr_threshold} dB)")
            ok = False

        # SBR presence check: the decoded high band should carry real
        # energy (not be silent/collapsed) since the original does.
        o_high = o - o_band
        d_high = d - lowpass_fft(d, sr, cutoff)
        o_energy = float(np.mean(o_high ** 2))
        d_energy = float(np.mean(d_high ** 2))
        ratio_db = 10 * np.log10(d_energy / o_energy) if o_energy > 0 and d_energy > 0 else float("-inf")
        print(f"HE high-band energy ratio (decoded/original): {ratio_db:.1f} dB")
        if ratio_db < args.sbr_energy_floor_db:
            print("FAIL: high-band energy collapsed (SBR not reconstructing?), "
                  f"ratio {ratio_db:.1f} dB < floor {args.sbr_energy_floor_db} dB")
            ok = False
    else:
        snr = spectral_snr_db(o, d)
        print(f"LC full-band spectral SNR: {snr:.1f} dB")
        if snr < args.snr_threshold:
            print(f"FAIL: spectral SNR below threshold ({args.snr_threshold} dB)")
            ok = False

    sys.exit(0 if ok else 1)


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(required=True)

    g = sub.add_parser("gen", help="generate the test signal WAV")
    g.add_argument("--sr", type=int, default=44100)
    g.add_argument("--duration", type=float, default=2.0)
    g.add_argument("--samples", type=int, default=0,
                    help="exact sample count, overrides --duration")
    g.add_argument("--he-tail-padding", type=int, default=None,
                    help="overrides --samples/--duration: generate content whose "
                         "HE-AAC encode leaves exactly this many core-rate samples "
                         "of natural trailing padding (0 is the worst case -- see "
                         "samples_for_he_tail_padding()'s docstring)")
    g.add_argument("--out", required=True)
    g.set_defaults(func=cmd_gen)

    c = sub.add_parser("check", help="check a decoded roundtrip against the original")
    c.add_argument("--orig", required=True)
    c.add_argument("--decoded", required=True,
                    help="the .m4a/.caf decoded back to 16-bit PCM WAV via "
                         "qaac itself (qaac -D --bits-per-sample 16), not ffmpeg "
                         "-- see module docstring")
    c.add_argument("--mode", choices=["lc", "he"], required=True)
    c.add_argument("--snr-threshold", type=float, default=15.0,
                    help="minimum acceptable spectral SNR in dB (default: %(default)s)")
    c.add_argument("--sbr-energy-floor-db", type=float, default=-20.0,
                    help="minimum acceptable decoded/original high-band energy "
                         "ratio in dB (default: %(default)s)")
    c.set_defaults(func=cmd_check)

    args = p.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
