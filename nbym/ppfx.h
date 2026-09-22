#pragma once
#include "yyh.h"
#include "gglx.h"
#include "per_channel.h"

namespace eqlib {

class EQLIB_INTERNAL FftRadix2 {
    int m_size{0};
    int m_log2_size{0};
    double m_cos_table[FFT_MAX_SIZE / 2];
    double m_sin_table[FFT_MAX_SIZE / 2];
    int m_rev_table[FFT_MAX_SIZE];

public:
    FftRadix2();
    bool init(int size);
    void forward(double* re, double* im) const;
    void inverse(double* re, double* im) const;
    int getSize() const;
};

struct EQLIB_INTERNAL ChannelSpectrumState {
    double frame_buffer[FFT_MAX_SIZE]{};
    double re_window[FFT_MAX_SIZE]{};
    double im_window[FFT_MAX_SIZE]{};
    double smoothed[FFT_MAX_SIZE / 2 + 1]{};
    int    frame_fill{0};
};

class EQLIB_INTERNAL SpectrumAnalyzer {
    double                   m_sample_rate{48000.0};
    int                      m_channels{2};
    SpectrumConfig           m_config;
    FftRadix2                m_fft;
    double                   m_window[FFT_MAX_SIZE];
    PerChannel<ChannelSpectrumState> m_states;
    int                      m_hop_size{512};
    double                   m_smoothing{0.7};
    bool                     m_initialized{false};

    void computeWindow();

public:
    SpectrumAnalyzer();
    bool init(double sr, int channels, const SpectrumConfig& config);
    void reset();
    void setSampleRate(double sr);
    void setChannels(int ch);
    int  getChannels() const;
    void setFftSize(int size);
    void setWindow(WindowType window);
    void setSmoothing(double smoothing);
    void processSample(double sample, int ch);
    bool getFrame(SpectrumFrame& out) const;
    int getFftSize() const;
    int getHopSize() const;
    double getSampleRate() const;
    WindowType getWindowType() const;
};

}
