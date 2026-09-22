#pragma once
#include "yyh.h"
#include <vector>

namespace eqlib {

class EQLIB_INTERNAL Resampler {
public:
    Resampler();
    ~Resampler();
    Resampler(const Resampler&) = delete;
    Resampler& operator=(const Resampler&) = delete;

    void setChannels(int ch);
    void setInputRate(double rate);
    void setOutputRate(double rate);
    void setQuality(int taps);
    int getChannels() const;
    double getInputRate() const;
    double getOutputRate() const;
    int getQuality() const;
    int getLatency() const;

    void reset();

    int process(const float* in, int in_frames, int channels,
                std::vector<float>& out, int& out_frames);
    int processDouble(const double* in, int in_frames, int channels,
                      std::vector<double>& out, int& out_frames);

private:
    void rebuildKernel();
    double sinc(double x) const;
    double blackmanWindow(double x) const;
    void updateHistory(const float* in, int in_frames, int channels);
    void updateHistoryDouble(const double* in, int in_frames, int channels);

    int m_channels{2};
    double m_in_rate{48000.0};
    double m_out_rate{48000.0};
    int m_taps{32};
    int m_phases{256};
    double m_output_time{0.0};
    std::vector<std::vector<double>> m_kernel;
    std::vector<double> m_history;
    bool m_dirty{true};
};

}
