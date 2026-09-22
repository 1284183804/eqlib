#include "neizhiqx.h"
#include <cmath>

namespace eqlib {

namespace {

const CurvePoint kFlatPoints[] = {
    {20.0, 0.0},
    {20000.0, 0.0},
};

const CurvePoint kHarmanPoints[] = {
    {20.0, 6.0},
    {60.0, 4.0},
    {150.0, 1.0},
    {400.0, -1.0},
    {1000.0, 0.0},
    {3000.0, 2.0},
    {6000.0, 1.0},
    {10000.0, -1.0},
    {20000.0, -3.0},
};

const CurvePoint kDiffusePoints[] = {
    {20.0, 4.0},
    {200.0, 2.0},
    {1000.0, 0.0},
    {4000.0, -2.0},
    {10000.0, -4.0},
    {20000.0, -6.0},
};

const CurvePoint kStudioPoints[] = {
    {20.0, -2.0},
    {200.0, 0.0},
    {1000.0, 0.0},
    {4000.0, 1.0},
    {10000.0, 0.0},
    {20000.0, -2.0},
};

const CurvePoint kLoudnessPoints[] = {
    {20.0, 8.0},
    {100.0, 4.0},
    {500.0, 0.0},
    {2000.0, 0.0},
    {6000.0, 3.0},
    {20000.0, 5.0},
};

const BuiltinCurveData kCurveTable[] = {
    {"Flat",     kFlatPoints,     static_cast<int>(sizeof(kFlatPoints) / sizeof(CurvePoint))},
    {"Custom",   nullptr,         0},
    {"Harman",   kHarmanPoints,   static_cast<int>(sizeof(kHarmanPoints) / sizeof(CurvePoint))},
    {"Diffuse",  kDiffusePoints,  static_cast<int>(sizeof(kDiffusePoints) / sizeof(CurvePoint))},
    {"Studio",   kStudioPoints,   static_cast<int>(sizeof(kStudioPoints) / sizeof(CurvePoint))},
    {"Loudness", kLoudnessPoints, static_cast<int>(sizeof(kLoudnessPoints) / sizeof(CurvePoint))},
};

constexpr int kCurveCount =
    static_cast<int>(sizeof(kCurveTable) / sizeof(kCurveTable[0]));

}

const BuiltinCurveData& BuiltinCurves::get(BuiltinCurve curve) {
    int idx = static_cast<int>(curve);
    if (idx < 0 || idx >= kCurveCount) idx = 0;
    return kCurveTable[idx];
}

int BuiltinCurves::getGainAt(BuiltinCurve curve, double freq_hz, double* out_gain_db) {
    if (!out_gain_db) return -1;
    const BuiltinCurveData& d = get(curve);
    if (!d.points || d.num_points <= 0) {
        *out_gain_db = 0.0;
        return 0;
    }
    return interpolate(d.points, d.num_points, freq_hz, out_gain_db);
}

int BuiltinCurves::interpolate(const CurvePoint* points, int num_points,
                               double freq_hz, double* out_gain_db) {
    if (!points || num_points <= 0 || !out_gain_db) return -1;
    if (freq_hz <= points[0].freq_hz) {
        *out_gain_db = points[0].gain_db;
        return 0;
    }
    if (freq_hz >= points[num_points - 1].freq_hz) {
        *out_gain_db = points[num_points - 1].gain_db;
        return 0;
    }
    int lo = 0;
    int hi = num_points - 1;
    while (lo + 1 < hi) {
        int mid = (lo + hi) / 2;
        if (points[mid].freq_hz <= freq_hz) lo = mid;
        else hi = mid;
    }
    double f0 = points[lo].freq_hz;
    double f1 = points[hi].freq_hz;
    if (f1 <= f0) {
        *out_gain_db = points[lo].gain_db;
        return 0;
    }
    double t = (freq_hz - f0) / (f1 - f0);
    *out_gain_db = points[lo].gain_db * (1.0 - t) + points[hi].gain_db * t;
    return 0;
}

BuiltinCurve BuiltinCurves::detectFromSpectrum(const double* mags, const double* freqs,
                                               int num_bins, double sample_rate) {
    if (!mags || !freqs || num_bins <= 0) return BuiltinCurve::Flat;
    double low_energy = 0.0;
    double mid_energy = 0.0;
    double high_energy = 0.0;
    int low_n = 0;
    int mid_n = 0;
    int high_n = 0;
    for (int i = 0; i < num_bins; ++i) {
        double f = freqs[i];
        double m = mags[i];
        if (f < 200.0) {
            low_energy += m;
            ++low_n;
        } else if (f < 2000.0) {
            mid_energy += m;
            ++mid_n;
        } else {
            high_energy += m;
            ++high_n;
        }
    }
    double low_avg = low_n > 0 ? low_energy / low_n : 0.0;
    double mid_avg = mid_n > 0 ? mid_energy / mid_n : 0.0;
    double high_avg = high_n > 0 ? high_energy / high_n : 0.0;
    (void)sample_rate;
    if (low_avg > mid_avg * 2.0 && low_avg > high_avg * 4.0) {
        return BuiltinCurve::Loudness;
    }
    if (high_avg > mid_avg * 2.0) {
        return BuiltinCurve::Studio;
    }
    if (mid_avg > low_avg * 2.0 && mid_avg > high_avg * 2.0) {
        return BuiltinCurve::Diffuse;
    }
    return BuiltinCurve::Flat;
}

const char* BuiltinCurves::getName(BuiltinCurve curve) {
    const BuiltinCurveData& d = get(curve);
    return d.name ? d.name : "Unknown";
}

int BuiltinCurves::getCurveCount() {
    return kCurveCount;
}

BuiltinCurve BuiltinCurves::getCurveByIndex(int index) {
    if (index < 0 || index >= kCurveCount) {
        return BuiltinCurve::Flat;
    }
    return static_cast<BuiltinCurve>(index);
}

}
