#include "caiyang.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace eqlib {

Resampler::Resampler() {
    setChannels(2);
    setInputRate(48000.0);
    setOutputRate(48000.0);
    setQuality(32);
}

Resampler::~Resampler() = default;

void Resampler::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > 64) ch = 64;
    if (ch == m_channels) return;
    m_channels = ch;
    reset();
}

void Resampler::setInputRate(double rate) {
    if (rate < 8000.0 || rate > 384000.0) return;
    if (std::fabs(rate - m_in_rate) < 1e-9) return;
    m_in_rate = rate;
    m_dirty = true;
    reset();
}

void Resampler::setOutputRate(double rate) {
    if (rate < 8000.0 || rate > 384000.0) return;
    if (std::fabs(rate - m_out_rate) < 1e-9) return;
    m_out_rate = rate;
    m_dirty = true;
    reset();
}

void Resampler::setQuality(int taps) {
    if (taps < 8) taps = 8;
    if (taps > 128) taps = 128;
    if (taps == m_taps) return;
    m_taps = taps;
    m_dirty = true;
    reset();
}

int Resampler::getChannels() const { return m_channels; }
double Resampler::getInputRate() const { return m_in_rate; }
double Resampler::getOutputRate() const { return m_out_rate; }
int Resampler::getQuality() const { return m_taps; }
int Resampler::getLatency() const { return m_taps / 2; }

void Resampler::reset() {
    m_history.assign(static_cast<std::size_t>(m_channels)
                   * static_cast<std::size_t>(m_taps), 0.0);
    m_output_time = 0.0;
}

double Resampler::sinc(double x) const {
    if (std::fabs(x) < 1e-12) return 1.0;
    const double pi = 3.14159265358979323846;
    double px = pi * x;
    return std::sin(px) / px;
}

double Resampler::blackmanWindow(double x) const {
    if (x < -1.0 || x > 1.0) return 0.0;
    const double pi = 3.14159265358979323846;
    double t = (x + 1.0) * 0.5;
    return 0.42 - 0.5 * std::cos(2.0 * pi * t) + 0.08 * std::cos(4.0 * pi * t);
}

void Resampler::rebuildKernel() {
    if (m_taps < 2) m_taps = 2;
    int half = m_taps / 2;
    if (half < 1) half = 1;

    double cutoff = 1.0;
    if (m_out_rate < m_in_rate) {
        cutoff = m_out_rate / m_in_rate;
        if (cutoff < 0.001) cutoff = 0.001;
    }

    m_kernel.assign(static_cast<std::size_t>(m_phases), {});

    for (int p = 0; p < m_phases; ++p) {
        m_kernel[static_cast<std::size_t>(p)].assign(static_cast<std::size_t>(m_taps), 0.0);
        double frac = static_cast<double>(p) / static_cast<double>(m_phases);
        double sum = 0.0;

        for (int j = 0; j < m_taps; ++j) {
            double t = frac + static_cast<double>(half) - static_cast<double>(j);
            double s = sinc(t * cutoff) * cutoff;
            double u = t / static_cast<double>(half);
            double w = blackmanWindow(u);
            double v = s * w;
            m_kernel[static_cast<std::size_t>(p)][static_cast<std::size_t>(j)] = v;
            sum += v;
        }

        if (std::fabs(sum) > 1e-12) {
            double inv = 1.0 / sum;
            for (int j = 0; j < m_taps; ++j) {
                m_kernel[static_cast<std::size_t>(p)][static_cast<std::size_t>(j)] *= inv;
            }
        }
    }

    m_dirty = false;
}

void Resampler::updateHistory(const float* in, int in_frames, int channels) {
    if (in_frames >= m_taps) {
        for (int i = 0; i < m_taps; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    static_cast<double>(in[(in_frames - m_taps + i) * channels + c]);
            }
        }
    } else {
        int shift = m_taps - in_frames;
        for (int i = 0; i < shift; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    m_history[static_cast<std::size_t>(i + in_frames) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
            }
        }
        for (int i = 0; i < in_frames; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(shift + i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    static_cast<double>(in[i * channels + c]);
            }
        }
    }
}

void Resampler::updateHistoryDouble(const double* in, int in_frames, int channels) {
    if (in_frames >= m_taps) {
        for (int i = 0; i < m_taps; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    in[(in_frames - m_taps + i) * channels + c];
            }
        }
    } else {
        int shift = m_taps - in_frames;
        for (int i = 0; i < shift; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    m_history[static_cast<std::size_t>(i + in_frames) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
            }
        }
        for (int i = 0; i < in_frames; ++i) {
            for (int c = 0; c < channels; ++c) {
                m_history[static_cast<std::size_t>(shift + i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                    in[i * channels + c];
            }
        }
    }
}

int Resampler::process(const float* in, int in_frames, int channels,
                       std::vector<float>& out, int& out_frames) {
    if (!in || in_frames <= 0 || channels != m_channels) return -1;
    if (m_dirty) rebuildKernel();

    if (std::fabs(m_in_rate - m_out_rate) < 1e-9) {
        out.assign(in, in + static_cast<std::size_t>(in_frames) * static_cast<std::size_t>(channels));
        out_frames = in_frames;
        updateHistory(in, in_frames, channels);
        m_output_time = 0.0;
        return 0;
    }

    int half = m_taps / 2;
    if (half < 1) half = 1;

    int total = m_taps + in_frames;
    std::vector<double> buf(static_cast<std::size_t>(total) * static_cast<std::size_t>(channels));

    for (int i = 0; i < m_taps; ++i) {
        for (int c = 0; c < channels; ++c) {
            buf[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
        }
    }
    for (int i = 0; i < in_frames; ++i) {
        for (int c = 0; c < channels; ++c) {
            buf[static_cast<std::size_t>(m_taps + i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                static_cast<double>(in[i * channels + c]);
        }
    }

    double step = m_in_rate / m_out_rate;
    double est_out = (static_cast<double>(in_frames) - m_output_time) / step + 2.0;
    int max_out = static_cast<int>(est_out);
    if (max_out < 0) max_out = 0;

    out.assign(static_cast<std::size_t>(max_out) * static_cast<std::size_t>(channels), 0.0f);

    int out_idx = 0;
    double pos = static_cast<double>(m_taps) + m_output_time;
    int max_tap_index = total - 1;

    for (int j = 0; j < max_out; ++j) {
        int ipos = static_cast<int>(std::floor(pos));
        double frac = pos - static_cast<double>(ipos);
        if (frac < 0.0) { frac += 1.0; ipos -= 1; }

        if (ipos - half < 0) break;
        if (ipos + half - 1 > max_tap_index) break;

        int phase = static_cast<int>(frac * static_cast<double>(m_phases));
        if (phase < 0) phase = 0;
        if (phase >= m_phases) phase = m_phases - 1;
        const double* kern = m_kernel[static_cast<std::size_t>(phase)].data();

        std::size_t out_base = static_cast<std::size_t>(out_idx) * static_cast<std::size_t>(channels);
        for (int c = 0; c < channels; ++c) {
            double sum = 0.0;
            for (int k = 0; k < m_taps; ++k) {
                int n = ipos - half + k;
                sum += kern[k] * buf[static_cast<std::size_t>(n) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
            }
            out[out_base + static_cast<std::size_t>(c)] = static_cast<float>(sum);
        }

        ++out_idx;
        pos += step;
    }

    out.resize(static_cast<std::size_t>(out_idx) * static_cast<std::size_t>(channels));
    out_frames = out_idx;

    m_output_time = m_output_time + static_cast<double>(out_idx) * step
                  - static_cast<double>(in_frames);

    updateHistory(in, in_frames, channels);
    return 0;
}

int Resampler::processDouble(const double* in, int in_frames, int channels,
                             std::vector<double>& out, int& out_frames) {
    if (!in || in_frames <= 0 || channels != m_channels) return -1;
    if (m_dirty) rebuildKernel();

    if (std::fabs(m_in_rate - m_out_rate) < 1e-9) {
        out.assign(in, in + static_cast<std::size_t>(in_frames) * static_cast<std::size_t>(channels));
        out_frames = in_frames;
        updateHistoryDouble(in, in_frames, channels);
        m_output_time = 0.0;
        return 0;
    }

    int half = m_taps / 2;
    if (half < 1) half = 1;

    int total = m_taps + in_frames;
    std::vector<double> buf(static_cast<std::size_t>(total) * static_cast<std::size_t>(channels));

    for (int i = 0; i < m_taps; ++i) {
        for (int c = 0; c < channels; ++c) {
            buf[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                m_history[static_cast<std::size_t>(i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
        }
    }
    for (int i = 0; i < in_frames; ++i) {
        for (int c = 0; c < channels; ++c) {
            buf[static_cast<std::size_t>(m_taps + i) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)] =
                in[i * channels + c];
        }
    }

    double step = m_in_rate / m_out_rate;
    double est_out = (static_cast<double>(in_frames) - m_output_time) / step + 2.0;
    int max_out = static_cast<int>(est_out);
    if (max_out < 0) max_out = 0;

    out.assign(static_cast<std::size_t>(max_out) * static_cast<std::size_t>(channels), 0.0);

    int out_idx = 0;
    double pos = static_cast<double>(m_taps) + m_output_time;
    int max_tap_index = total - 1;

    for (int j = 0; j < max_out; ++j) {
        int ipos = static_cast<int>(std::floor(pos));
        double frac = pos - static_cast<double>(ipos);
        if (frac < 0.0) { frac += 1.0; ipos -= 1; }

        if (ipos - half < 0) break;
        if (ipos + half - 1 > max_tap_index) break;

        int phase = static_cast<int>(frac * static_cast<double>(m_phases));
        if (phase < 0) phase = 0;
        if (phase >= m_phases) phase = m_phases - 1;
        const double* kern = m_kernel[static_cast<std::size_t>(phase)].data();

        std::size_t out_base = static_cast<std::size_t>(out_idx) * static_cast<std::size_t>(channels);
        for (int c = 0; c < channels; ++c) {
            double sum = 0.0;
            for (int k = 0; k < m_taps; ++k) {
                int n = ipos - half + k;
                sum += kern[k] * buf[static_cast<std::size_t>(n) * static_cast<std::size_t>(channels) + static_cast<std::size_t>(c)];
            }
            out[out_base + static_cast<std::size_t>(c)] = sum;
        }

        ++out_idx;
        pos += step;
    }

    out.resize(static_cast<std::size_t>(out_idx) * static_cast<std::size_t>(channels));
    out_frames = out_idx;

    m_output_time = m_output_time + static_cast<double>(out_idx) * step
                  - static_cast<double>(in_frames);

    updateHistoryDouble(in, in_frames, channels);
    return 0;
}

}
