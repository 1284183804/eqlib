#pragma once
#include "yyh.h"
#include "gglx.h"
#include "per_channel.h"

namespace eqlib {

class EQLIB_INTERNAL AgcCore {
    double             m_sample_rate{48000.0};
    int                m_channels{2};
    AgcParams          m_params;
    PerChannel<double> m_envelope;
    PerChannel<double> m_gain_linear;
    double             m_attack_coeff{0.0};
    double             m_release_coeff{0.0};

public:
    AgcCore();
    void setSampleRate(double sr);
    void setChannels(int ch);
    int  getChannels() const;
    void setParams(const AgcParams& params);
    AgcParams getParams() const;
    void setTarget(double target_dbfs);
    void setAttack(double attack_ms);
    void setRelease(double release_ms);
    void setMaxBoost(double max_boost_db);
    void setMaxCut(double max_cut_db);
    void setGateEnable(bool enable);
    void setGateThreshold(double threshold_dbfs);
    void reset();
    double processSample(double input, int ch);
    void processBlock(const double* input, double* output, int frames, int channels);
    void processBlockFloat(const float* input, float* output, int frames, int channels);
    double getCurrentGainDb() const;
    double getEnvelopeDb() const;
};

}
