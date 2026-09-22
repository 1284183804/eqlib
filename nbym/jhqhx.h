#pragma once
#include "gglx.h"
#include "csdl.h"
#include "sejlbq.h"
#include "per_channel.h"
#include "neizhiqx.h"

namespace eqlib {

class EqCore {
    double          m_sample_rate{48000.0};
    int             m_channels{2};
    int             m_num_bands{DEFAULT_BANDS};
    ParamQueue<128> m_param_queue;

    PerChannel<Biquad<double>> m_filters[MAX_BANDS];
    double m_target_gain[MAX_BANDS]{};
    double m_current_gain[MAX_BANDS]{};
    double m_last_applied_gain[MAX_BANDS]{};
    double m_smoothing_coeff{0.001};

    FilterType m_band_type[MAX_BANDS]{};
    double     m_band_freq[MAX_BANDS]{};
    double     m_band_q[MAX_BANDS]{};
    bool       m_band_enable[MAX_BANDS]{};
    bool       m_dirty[MAX_BANDS]{};

    BandRange m_band_range[MAX_BANDS]{};
    bool      m_range_set[MAX_BANDS]{};

    BuiltinCurve m_builtin_curve{BuiltinCurve::Flat};
    CurvePoint   m_custom_curve[CURVE_MAX_POINTS]{};
    int          m_custom_curve_points{0};
    double       m_global_gain_db{0.0};
    double       m_global_gain_linear{1.0};

public:
    EqCore();

    void setSampleRate(double sr);
    void setChannels(int ch);
    int  getChannels() const;
    int  getNumBands() const;
    void setNumBands(int n);

    void setBandType(int band, FilterType type);
    void setBandFreq(int band, double freq_hz);
    void setBandGain(int band, double gain_db);
    void setBandQ(int band, double q);
    void setBandEnable(int band, bool enable);

    void setBandRange(int band, double freq_low, double freq_high);
    void clearBandRange(int band);
    bool getBandRange(int band, double* out_low, double* out_high) const;

    void setGlobalGain(double gain_db);
    double getGlobalGain() const;

    void setBuiltinCurve(BuiltinCurve curve);
    BuiltinCurve getBuiltinCurve() const;
    int setCustomCurve(const CurvePoint* points, int num_points);
    int applyBuiltinCurveToBands();
    int applyCustomCurveToBands();
    int autoDetectCurveFromSpectrum(const double* mags, const double* freqs,
                                    int num_bins, double sample_rate);

    void reset();
    void consumeParamQueue();

    double processSample(double input, int ch);
    void   processBlock(const double* input, double* output, int frames, int channels);
    void   processBlockFloat(const float* input, float* output, int frames, int channels);

    double getBandGain(int band) const;
    FilterType getBandType(int band) const;
    double getBandFreq(int band) const;
    double getBandQ(int band) const;
    bool getBandEnable(int band) const;
};

}
