#pragma once
#include "yyh.h"
#include "gglx.h"
#include "per_channel.h"
#include <vector>

namespace eqlib {

class EQLIB_INTERNAL FftRadix2 {
    int m_size{0};
    int m_log2_size{0};
    std::vector<double> m_cos_table;
    std::vector<double> m_sin_table;
    std::vector<int>    m_rev_table;

public:
    FftRadix2();
    bool init(int size);
    void forward(double* re, double* im) const;
    void inverse(double* re, double* im) const;
    int getSize() const;
};

struct EQLIB_INTERNAL ChannelSpectrumState {
    std::vector<double> frame_buffer;
    std::vector<double> re_window;
    std::vector<double> im_window;
    std::vector<double> smoothed;
    int frame_fill{0};

    void resize(int fft_size) {
        if (fft_size < 2) fft_size = 2;
        std::size_t n = static_cast<std::size_t>(fft_size);
        std::size_t half = n / 2 + 1;
        frame_buffer.assign(n, 0.0);
        re_window.assign(n, 0.0);
        im_window.assign(n, 0.0);
        smoothed.assign(half, 0.0);
        frame_fill = 0;
    }

    void clear() {
        std::fill(frame_buffer.begin(), frame_buffer.end(), 0.0);
        std::fill(re_window.begin(), re_window.end(), 0.0);
        std::fill(im_window.begin(), im_window.end(), 0.0);
        std::fill(smoothed.begin(), smoothed.end(), 0.0);
        frame_fill = 0;
    }
};

class EQLIB_INTERNAL SpectrumAnalyzer {
    double              m_sample_rate{48000.0};
    int                 m_channels{2};
    SpectrumConfig      m_config;
    FftRadix2           m_fft;
    std::vector<double> m_window;
    PerChannel<ChannelSpectrumState> m_states;
    int                 m_hop_size{512};
    double              m_smoothing{0.7};
    bool                m_initialized{false};

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
