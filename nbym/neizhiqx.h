#pragma once
#include "gglx.h"

namespace eqlib {

class BuiltinCurves {
public:
    static const BuiltinCurveData& get(BuiltinCurve curve);
    static int getGainAt(BuiltinCurve curve, double freq_hz, double* out_gain_db);
    static int interpolate(const CurvePoint* points, int num_points,
                           double freq_hz, double* out_gain_db);
    static BuiltinCurve detectFromSpectrum(const double* mags, const double* freqs,
                                            int num_bins, double sample_rate);
    static const char* getName(BuiltinCurve curve);
    static int getCurveCount();
    static BuiltinCurve getCurveByIndex(int index);
};

}
