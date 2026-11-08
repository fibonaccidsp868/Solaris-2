#pragma once
#include <cmath>
#include <algorithm>

namespace solaris
{
constexpr int kNumBands = 5;
inline constexpr double kCrossover[4] = { 120.0, 500.0, 2000.0, 6500.0 };
inline constexpr double kEdges[6]     = { 20.0, 120.0, 500.0, 2000.0, 6500.0, 20000.0 };
inline constexpr const char* kBandNames[5]  = { "LOW", "LOW MID", "MID", "HIGH MID", "HIGH" };
inline constexpr const char* kBandRanges[5] = { "20-120 Hz", "120-500 Hz", "0.5-2 kHz", "2-6.5 kHz", "6.5-20 kHz" };

// Single-mode voicing: how the master Drive/Warmth is distributed over the five bands.
// Lows and top end get a little less drive (tight bass, never harsh), mids get the most.
inline constexpr float  kVoiceDrive[5] = { 0.80f, 1.00f, 1.00f, 0.90f, 0.70f };
inline constexpr float  kVoiceWarm[5]  = { 1.30f, 1.10f, 0.90f, 0.60f, 0.35f };
// Envelope release per band (seconds): slow for lows so bass waveforms are never "followed".
inline constexpr double kBandRelease[5] = { 0.140, 0.090, 0.060, 0.040, 0.025 };

//==============================================================================
// Double-precision biquad (RBJ cookbook), transposed direct form II.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1[2] = { 0, 0 }, z2[2] = { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    inline float process (int ch, float xin) noexcept
    {
        const double x = (double) xin;
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    static constexpr double twoPi = 6.283185307179586;

    void lowPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void allPass (double fs, double f, double q)
    {
        const double w = twoPi * f / fs, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 - al, -2 * c, 1 + al, 1 + al, -2 * c, 1 - al);
    }
    void highShelf (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / (2 * q), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
    void lowShelf (double fs, double f, double q, double gainDb)
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w = twoPi * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / (2 * q), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
             (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
};

// Linkwitz-Riley 4th order crossover (LP + HP = 2nd-order allpass, compensated in the processor)
struct LR4
{
    Biquad lp1, lp2, hp1, hp2;
    void set (double fs, double f)
    {
        const double q = 0.70710678118654752;
        lp1.lowPass (fs, f, q);  lp2.lowPass (fs, f, q);
        hp1.highPass (fs, f, q); hp2.highPass (fs, f, q);
    }
    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }
    inline void split (int ch, float x, float& lo, float& hi) noexcept
    {
        lo = lp2.process (ch, lp1.process (ch, x));
        hi = hp2.process (ch, hp1.process (ch, x));
    }
};

//==============================================================================
//  PRISM ENGINE
//
//  Saturation curve:  f(u) = erf(sqrt(pi)/2 * u)
//   - slope 1 at the origin, saturates at +-1
//   - erf is an *entire* function: its harmonics fall off faster than any exponential,
//     so even at heavy drive the energy stays in the low, musical harmonics (2nd..7th)
//     instead of the fizzy 9th-20th harmonics a tanh / hard clip produces.
//   - closed-form antiderivative F(u) -> first-order ADAA (anti-aliased) on top of 4x oversampling.
//
//  Level-normalised ("auto-leveled") drive:
//   The curve sees  u = D * (x / env + bias).  Drive therefore controls the *shape* of the
//   waveform, not how much quiet material (hiss, room, tails) is boosted -> the noise floor is
//   never amplified and loudness stays constant while density rises.
//   A table-driven compensation keeps the fundamental amplitude identical at every depth.
//
//  Thermal state: a slow (350 ms) follower of the signal level nudges depth and bias by a few
//  percent, like a stage that settles as it warms up.
struct SatCurve
{
    static inline double f (double u) noexcept { return std::erf (0.88622692545275801 * u); }
    static inline double F (double u) noexcept
    {
        return u * std::erf (0.88622692545275801 * u) + 0.63661977236758134 * std::exp (-0.78539816339744831 * u * u);
    }
};

struct CompTable   // 1 / (fundamental amplitude of f(D*cos t)), D = 0 .. 8
{
    static constexpr int N = 401;
    float v[N];

    void build()
    {
        for (int i = 0; i < N; ++i)
        {
            const double D = i * 0.02;
            if (D < 1.0e-6) { v[i] = 1.0f; continue; }
            constexpr int M = 512;
            double sum = 0.0;
            for (int k = 0; k < M; ++k)
            {
                const double th = (k + 0.5) * 3.14159265358979323846 / M;
                sum += SatCurve::f (D * std::cos (th)) * std::cos (th);
            }
            v[i] = (float) (1.0 / std::max (2.0 * sum / M, 1.0e-3));
        }
    }
    inline double get (double D) const noexcept
    {
        const double p = std::min (std::max (D, 0.0) * 50.0, (double) (N - 2));
        const int i = (int) p;
        return v[i] + (v[i + 1] - v[i]) * (p - i);
    }
};

struct NormSat
{
    double uPrev = 0.0, Fprev = 0.0, env = 0.0, heat = 0.0;

    void reset() { uPrev = Fprev = env = heat = 0.0; }

    // returns the saturated signal with the same fundamental level as the input
    inline float process (float xin, double depth, double bias, const CompTable& ct,
                          double atkC, double relC, double heatC, double thermal) noexcept
    {
        const double x = (double) xin, ax = std::abs (x);
        env += (ax - env) * (ax > env ? atkC : relC);
        const double ref = env + 2.0e-4;                       // noise-floor guard
        heat += (env - heat) * heatC;
        const double th = std::tanh (3.0 * heat) * thermal;    // 0..thermal
        const double D = depth * (1.0 - 0.05 * th);
        const double bi = bias + 0.04 * th;

        const double u = D * (x / ref + bi);
        const double d = u - uPrev;
        const double Fu = SatCurve::F (u);
        const double y = std::abs (d) < 1.0e-5 ? SatCurve::f (0.5 * (u + uPrev)) : (Fu - Fprev) / d;
        uPrev = u; Fprev = Fu;

        return (float) (ref * (y - SatCurve::f (D * bi)) * ct.get (D));
    }
};

// Smooth-knee output soft clipper (C1 continuous)
inline float softClip (float x) noexcept
{
    constexpr float knee = 0.80f, ceiling = 0.97f;
    const float a = std::abs (x);
    if (a <= knee) return x;
    const float s = knee + (ceiling - knee) * std::tanh ((a - knee) / (ceiling - knee));
    return x < 0 ? -s : s;
}
} // namespace solaris
