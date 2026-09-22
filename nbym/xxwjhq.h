#pragma once
#include "yyh.h"
#include "gglx.h"
#include "ppfx.h"
#include <vector>

namespace eqlib {

class EQLIB_INTERNAL LinearPhaseEq {
public:
    static constexpr int MAX_KERNEL = 1024;

    LinearPhaseEq();
    LinearPhaseEq(const LinearPhaseEq&) = delete;
    LinearPhaseEq& operator=(const LinearPhaseEq&) = delete;
    LinearPhaseEq(LinearPhaseEq&&) = delete;
    LinearPhaseEq& operator=(LinearPhaseEq&&) = delete;

    void setSampleRate(double sr);
    void setChannels(int ch);
    void setKernelSize(int size);
    int getKernelSize() const;
    int getChannels() const;
    double getSampleRate() const;

    void setBandType(int band, FilterType type);
    void setBandFreq(int band, double freq_hz);
    void setBandGain(int band, double gain_db);
    void setBandQ(int band, double q);
    void setBandEnable(int band, bool enable);

    FilterType getBandType(int band) const;
    double getBandFreq(int band) const;
    double getBandGain(int band) const;
    double getBandQ(int band) const;
    bool getBandEnable(int band) const;

    void reset();
    void rebuild();
    int getLatency() const;

    double processSample(int ch, double input);
    void processBlock(const double* input, double* output, int frames, int channels);
    void processBlockFloat(const float* input, float* output, int frames, int channels);

private:
    double m_sample_rate{48000.0};
    int m_channels{2};
    int m_kernel_size{512};
    FilterType m_band_type[NUM_BANDS]{};
    double m_band_freq[NUM_BANDS]{};
    double m_band_gain[NUM_BANDS]{};
    double m_band_q[NUM_BANDS]{};
    bool m_band_enable[NUM_BANDS]{};
    double m_kernel[MAX_KERNEL]{};
    std::vector<double> m_history;
    std::vector<int> m_hist_idx;
    bool m_dirty{true};
    bool m_fft_ready{false};
    FftRadix2 m_fft;
};

}
