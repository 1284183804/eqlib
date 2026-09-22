#include "ppfx.h"
#include <cmath>

namespace eqlib {

FftRadix2::FftRadix2() = default;

bool FftRadix2::init(int size) {
    if (size < 2) return false;
    if (size > FFT_MAX_SIZE) return false;
    int log2_size = 0;
    int n = size;
    while (n > 1) {
        if (n & 1) return false;
        n >>= 1;
        ++log2_size;
    }
    m_size = size;
    m_log2_size = log2_size;
    const double pi = 3.14159265358979323846;
    for (int i = 0; i < size / 2; ++i) {
        double angle = -2.0 * pi * i / size;
        m_cos_table[i] = std::cos(angle);
        m_sin_table[i] = std::sin(angle);
    }
    for (int i = 0; i < size; ++i) {
        int rev = 0;
        int x = i;
        for (int j = 0; j < log2_size; ++j) {
            rev = (rev << 1) | (x & 1);
            x >>= 1;
        }
        m_rev_table[i] = rev;
    }
    return true;
}

void FftRadix2::forward(double* re, double* im) const {
    for (int i = 0; i < m_size; ++i) {
        int j = m_rev_table[i];
        if (j > i) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
    for (int len = 2; len <= m_size; len <<= 1) {
        int half = len >> 1;
        int step = m_size / len;
        for (int i = 0; i < m_size; i += len) {
            for (int j = 0; j < half; ++j) {
                int k = j * step;
                double wr = m_cos_table[k];
                double wi = m_sin_table[k];
                int a = i + j;
                int b = a + half;
                double tr = wr * re[b] - wi * im[b];
                double ti = wr * im[b] + wi * re[b];
                re[b] = re[a] - tr;
                im[b] = im[a] - ti;
                re[a] += tr;
                im[a] += ti;
            }
        }
    }
}

void FftRadix2::inverse(double* re, double* im) const {
    for (int i = 0; i < m_size; ++i) im[i] = -im[i];
    forward(re, im);
    double inv = 1.0 / m_size;
    for (int i = 0; i < m_size; ++i) {
        re[i] *= inv;
        im[i] *= -inv;
    }
}

int FftRadix2::getSize() const { return m_size; }

SpectrumAnalyzer::SpectrumAnalyzer() = default;

void SpectrumAnalyzer::computeWindow() {
    const double pi = 3.14159265358979323846;
    int size = m_config.fft_size;
    if (size < 1) return;
    for (int i = 0; i < size; ++i) {
        double w = 0.0;
        switch (m_config.window) {
        case WindowType::Rectangular:
            w = 1.0;
            break;
        case WindowType::Hann:
            w = 0.5 * (1.0 - std::cos(2.0 * pi * i / (size - 1)));
            break;
        case WindowType::Hamming:
            w = 0.54 - 0.46 * std::cos(2.0 * pi * i / (size - 1));
            break;
        case WindowType::Blackman:
            w = 0.42 - 0.5 * std::cos(2.0 * pi * i / (size - 1))
                + 0.08 * std::cos(4.0 * pi * i / (size - 1));
            break;
        case WindowType::FlatTop:
            w = 1.0 - 1.93 * std::cos(2.0 * pi * i / (size - 1))
                + 1.29 * std::cos(4.0 * pi * i / (size - 1))
                - 0.388 * std::cos(6.0 * pi * i / (size - 1));
            break;
        }
        m_window[i] = w;
    }
}

bool SpectrumAnalyzer::init(double sr, int channels, const SpectrumConfig& config) {
    if (config.fft_size <= 0 || config.fft_size > FFT_MAX_SIZE) return false;
    if (channels < 1) channels = 1;
    if (channels > WAV_MAX_CHANNELS) channels = WAV_MAX_CHANNELS;
    m_sample_rate = sr;
    m_channels = channels;
    m_config = config;
    m_states.resize(channels);
    if (!m_fft.init(config.fft_size)) return false;
    m_hop_size = static_cast<int>(config.fft_size * (1.0 - config.overlap));
    if (m_hop_size < 1) m_hop_size = 1;
    computeWindow();
    m_initialized = true;
    reset();
    return true;
}

void SpectrumAnalyzer::reset() {
    for (int c = 0; c < m_states.size(); ++c) {
        ChannelSpectrumState& s = m_states[c];
        s.frame_fill = 0;
        for (int i = 0; i < m_config.fft_size; ++i) {
            s.frame_buffer[i] = 0.0;
            s.re_window[i] = 0.0;
            s.im_window[i] = 0.0;
        }
        for (int i = 0; i <= m_config.fft_size / 2; ++i) {
            s.smoothed[i] = 0.0;
        }
    }
}

void SpectrumAnalyzer::setSampleRate(double sr) {
    m_sample_rate = sr;
}

void SpectrumAnalyzer::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    m_states.resize(ch);
    reset();
}

int SpectrumAnalyzer::getChannels() const {
    return m_channels;
}

void SpectrumAnalyzer::setFftSize(int size) {
    if (size <= 0 || size > FFT_MAX_SIZE) return;
    if (size == m_config.fft_size) return;
    if (m_initialized) {
        SpectrumConfig cfg = m_config;
        cfg.fft_size = size;
        init(m_sample_rate, m_channels, cfg);
    } else {
        m_config.fft_size = size;
        m_hop_size = static_cast<int>(size * (1.0 - m_config.overlap));
        if (m_hop_size < 1) m_hop_size = 1;
        computeWindow();
    }
}

void SpectrumAnalyzer::setWindow(WindowType window) {
    if (window == m_config.window) return;
    if (m_initialized) {
        SpectrumConfig cfg = m_config;
        cfg.window = window;
        init(m_sample_rate, m_channels, cfg);
    } else {
        m_config.window = window;
        computeWindow();
    }
}

void SpectrumAnalyzer::setSmoothing(double smoothing) {
    if (smoothing < 0.0) smoothing = 0.0;
    if (smoothing > 0.999) smoothing = 0.999;
    m_smoothing = smoothing;
}

void SpectrumAnalyzer::processSample(double sample, int ch) {
    if (!m_initialized) return;
    if (ch < 0 || ch >= m_channels) return;

    ChannelSpectrumState& s = m_states[ch];
    s.frame_buffer[s.frame_fill++] = sample;
    if (s.frame_fill < m_config.fft_size) return;

    for (int i = 0; i < m_config.fft_size; ++i) {
        s.re_window[i] = s.frame_buffer[i] * m_window[i];
        s.im_window[i] = 0.0;
    }
    m_fft.forward(s.re_window, s.im_window);

    int half = m_config.fft_size / 2;
    for (int i = 0; i <= half; ++i) {
        double mag = std::sqrt(s.re_window[i] * s.re_window[i]
                             + s.im_window[i] * s.im_window[i]);
        mag /= m_config.fft_size;
        if (i > 0 && i < half) mag *= 2.0;
        s.smoothed[i] = m_smoothing * s.smoothed[i] + (1.0 - m_smoothing) * mag;
    }

    int shift = m_hop_size;
    int remain = m_config.fft_size - shift;
    for (int i = 0; i < remain; ++i) {
        s.frame_buffer[i] = s.frame_buffer[i + shift];
    }
    s.frame_fill = remain;
}

bool SpectrumAnalyzer::getFrame(SpectrumFrame& out) const {
    if (!m_initialized) return false;
    if (m_states.size() <= 0) return false;

    const ChannelSpectrumState& s = m_states[0];
    int half = m_config.fft_size / 2;
    out.num_bins = half + 1;
    out.sample_rate = m_sample_rate;
    for (int i = 0; i <= half; ++i) {
        out.magnitudes[i] = s.smoothed[i];
        out.freqs_hz[i] = static_cast<double>(i) * m_sample_rate / m_config.fft_size;
    }
    return true;
}

int SpectrumAnalyzer::getFftSize() const { return m_config.fft_size; }
int SpectrumAnalyzer::getHopSize() const { return m_hop_size; }
double SpectrumAnalyzer::getSampleRate() const { return m_sample_rate; }
WindowType SpectrumAnalyzer::getWindowType() const { return m_config.window; }

}
