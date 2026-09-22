#pragma once
#include "yyh.h"
#include "gglx.h"
#include "jhqhx.h"
#include "xxwjhq.h"

namespace eqlib {

class EQLIB_INTERNAL HybridPhaseEq {
    EqCore        m_min_phase;
    LinearPhaseEq m_lin_phase;
    PhaseMode     m_band_mode[NUM_BANDS]{};
    FilterType    m_band_type[NUM_BANDS]{};
    double        m_band_freq[NUM_BANDS]{};
    double        m_band_gain[NUM_BANDS]{};
    double        m_band_q[NUM_BANDS]{};
    bool          m_band_enable[NUM_BANDS]{};
    double        m_sample_rate{48000.0};
    int           m_channels{2};
    bool          m_dirty{true};

public:
    HybridPhaseEq();
    HybridPhaseEq(const HybridPhaseEq&) = delete;
    HybridPhaseEq& operator=(const HybridPhaseEq&) = delete;
    HybridPhaseEq(HybridPhaseEq&&) = delete;
    HybridPhaseEq& operator=(HybridPhaseEq&&) = delete;

    void setSampleRate(double sr);
    void setChannels(int ch);
    double getSampleRate() const;
    int getChannels() const;

    void setBandPhaseMode(int band, PhaseMode mode);
    void setBandType(int band, FilterType type);
    void setBandFreq(int band, double freq_hz);
    void setBandGain(int band, double gain_db);
    void setBandQ(int band, double q);
    void setBandEnable(int band, bool enable);
    void setLinearKernelSize(int size);

    PhaseMode getBandPhaseMode(int band) const;
    FilterType getBandType(int band) const;
    double getBandFreq(int band) const;
    double getBandGain(int band) const;
    double getBandQ(int band) const;
    bool getBandEnable(int band) const;
    int getLinearKernelSize() const;
    int getLatency() const;

    void rebuild();
    void reset();

    double processSample(double input, int ch);
    void processBlock(const double* input, double* output, int frames, int channels);
    void processBlockFloat(const float* input, float* output, int frames, int channels);

private:
    void applyBandToEngines(int band);
    void applyAllBands();
};

}
