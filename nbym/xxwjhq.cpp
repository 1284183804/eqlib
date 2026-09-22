#include "xxwjhq.h"
#include <cmath>
#include <vector>
#include <algorithm>

namespace eqlib {

namespace {

struct BiquadCoeffs {
    double b0{1.0};
    double b1{0.0};
    double b2{0.0};
    double a1{0.0};
    double a2{0.0};
};

BiquadCoeffs computeCoeffs(FilterType type, double freq_hz, double gain_db, double q, double sr) {
    const double pi = 3.14159265358979323846;
    BiquadCoeffs c{};
    double w0 = 2.0 * pi * freq_hz / sr;
    if (w0 > pi) w0 = pi;
    if (w0 < 0.0) w0 = 0.0;
    double cos_w0 = std::cos(w0);
    double sin_w0 = std::sin(w0);
    double alpha = sin_w0 / (2.0 * q);
    double A = std::pow(10.0, gain_db / 40.0);

    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a0 = 1.0;
    double a1 = 0.0;
    double a2 = 0.0;

    switch (type) {
    case FilterType::Peaking:
        b0 = 1.0 + alpha * A;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 - alpha * A;
        a0 = 1.0 + alpha / A;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha / A;
        break;
    case FilterType::LowShelf: {
        double sqrtA = std::sqrt(A);
        double tsa = 2.0 * sqrtA * alpha;
        b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + tsa);
        b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - tsa);
        a0 = (A + 1.0) + (A - 1.0) * cos_w0 + tsa;
        a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0);
        a2 = (A + 1.0) + (A - 1.0) * cos_w0 - tsa;
        break;
    }
    case FilterType::HighShelf: {
        double sqrtA = std::sqrt(A);
        double tsa = 2.0 * sqrtA * alpha;
        b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + tsa);
        b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
        b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - tsa);
        a0 = (A + 1.0) - (A - 1.0) * cos_w0 + tsa;
        a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0);
        a2 = (A + 1.0) - (A - 1.0) * cos_w0 - tsa;
        break;
    }
    case FilterType::LowPass:
        b0 = (1.0 - cos_w0) / 2.0;
        b1 = 1.0 - cos_w0;
        b2 = (1.0 - cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::HighPass:
        b0 = (1.0 + cos_w0) / 2.0;
        b1 = -(1.0 + cos_w0);
        b2 = (1.0 + cos_w0) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::BandPass:
        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::Notch:
        b0 = 1.0;
        b1 = -2.0 * cos_w0;
        b2 = 1.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    case FilterType::AllPass:
        b0 = 1.0 - alpha;
        b1 = -2.0 * cos_w0;
        b2 = 1.0 + alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cos_w0;
        a2 = 1.0 - alpha;
        break;
    }

    c.b0 = b0 / a0;
    c.b1 = b1 / a0;
    c.b2 = b2 / a0;
    c.a1 = a1 / a0;
    c.a2 = a2 / a0;
    return c;
}

double magAt(const BiquadCoeffs& c, double w) {
    double cos_w = std::cos(w);
    double sin_w = std::sin(w);
    double cos_2w = std::cos(2.0 * w);
    double sin_2w = std::sin(2.0 * w);

    double num_re = c.b0 + c.b1 * cos_w + c.b2 * cos_2w;
    double num_im = -(c.b1 * sin_w + c.b2 * sin_2w);
    double den_re = 1.0 + c.a1 * cos_w + c.a2 * cos_2w;
    double den_im = -(c.a1 * sin_w + c.a2 * sin_2w);

    double num_sq = num_re * num_re + num_im * num_im;
    double den_sq = den_re * den_re + den_im * den_im;
    if (den_sq < 1e-20) return 0.0;
    return std::sqrt(num_sq / den_sq);
}

}

LinearPhaseEq::LinearPhaseEq() {
    for (int i = 0; i < NUM_BANDS; ++i) {
        m_band_type[i] = FilterType::Peaking;
        m_band_freq[i] = 1000.0;
        m_band_gain[i] = 0.0;
        m_band_q[i] = 0.707;
        m_band_enable[i] = true;
    }
    reset();
    m_dirty = true;
}

void LinearPhaseEq::setSampleRate(double sr) {
    if (sr < SR_MIN || sr > SR_MAX) return;
    m_sample_rate = sr;
    m_dirty = true;
}

void LinearPhaseEq::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    reset();
}

void LinearPhaseEq::setKernelSize(int size) {
    if (size < 32) size = 32;
    if (size > MAX_KERNEL) size = MAX_KERNEL;
    int p = 1;
    while (p < size) p *= 2;
    size = p;
    if (size > MAX_KERNEL) size = MAX_KERNEL;
    if (m_kernel_size == size) return;
    m_kernel_size = size;
    m_fft_ready = false;
    m_dirty = true;
    reset();
}

int LinearPhaseEq::getKernelSize() const { return m_kernel_size; }
int LinearPhaseEq::getChannels() const { return m_channels; }
double LinearPhaseEq::getSampleRate() const { return m_sample_rate; }

void LinearPhaseEq::setBandType(int band, FilterType type) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_type[band] = type;
    m_dirty = true;
}

void LinearPhaseEq::setBandFreq(int band, double freq_hz) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_freq[band] = freq_hz;
    m_dirty = true;
}

void LinearPhaseEq::setBandGain(int band, double gain_db) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_gain[band] = gain_db;
    m_dirty = true;
}

void LinearPhaseEq::setBandQ(int band, double q) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_q[band] = q;
    m_dirty = true;
}

void LinearPhaseEq::setBandEnable(int band, bool enable) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_enable[band] = enable;
    m_dirty = true;
}

FilterType LinearPhaseEq::getBandType(int band) const {
    if (band < 0 || band >= NUM_BANDS) return FilterType::Peaking;
    return m_band_type[band];
}

double LinearPhaseEq::getBandFreq(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_freq[band];
}

double LinearPhaseEq::getBandGain(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_gain[band];
}

double LinearPhaseEq::getBandQ(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_q[band];
}

bool LinearPhaseEq::getBandEnable(int band) const {
    if (band < 0 || band >= NUM_BANDS) return false;
    return m_band_enable[band];
}

void LinearPhaseEq::reset() {
    m_history.assign(static_cast<std::size_t>(m_channels) * m_kernel_size, 0.0);
    m_hist_idx.assign(static_cast<std::size_t>(m_channels), 0);
}

void LinearPhaseEq::rebuild() {
    if (!m_fft_ready) {
        m_fft.init(m_kernel_size);
        m_fft_ready = true;
    }

    const int N = m_kernel_size;
    const double pi = 3.14159265358979323846;

    BiquadCoeffs coeffs[NUM_BANDS];
    bool enabled[NUM_BANDS];
    for (int b = 0; b < NUM_BANDS; ++b) {
        enabled[b] = m_band_enable[b];
        if (enabled[b]) {
            coeffs[b] = computeCoeffs(m_band_type[b], m_band_freq[b], m_band_gain[b], m_band_q[b], m_sample_rate);
        }
    }

    std::vector<double> re(static_cast<std::size_t>(N), 1.0);
    std::vector<double> im(static_cast<std::size_t>(N), 0.0);

    for (int k = 0; k <= N / 2; ++k) {
        double w = 2.0 * pi * k / N;
        double mag = 1.0;
        for (int b = 0; b < NUM_BANDS; ++b) {
            if (!enabled[b]) continue;
            mag *= magAt(coeffs[b], w);
        }
        re[static_cast<std::size_t>(k)] = mag;
    }
    for (int k = N / 2 + 1; k < N; ++k) {
        re[static_cast<std::size_t>(k)] = re[static_cast<std::size_t>(N - k)];
    }

    m_fft.inverse(re.data(), im.data());

    std::vector<double> shifted(static_cast<std::size_t>(N));
    for (int k = 0; k < N; ++k) {
        shifted[static_cast<std::size_t>(k)] = re[static_cast<std::size_t>((k + N / 2) % N)];
    }

    for (int k = 0; k < N; ++k) {
        double w = 0.35875
                   - 0.48829 * std::cos(2.0 * pi * k / (N - 1))
                   + 0.14128 * std::cos(4.0 * pi * k / (N - 1))
                   - 0.01168 * std::cos(6.0 * pi * k / (N - 1));
        shifted[static_cast<std::size_t>(k)] *= w;
    }

    double want_dc = 1.0;
    for (int b = 0; b < NUM_BANDS; ++b) {
        if (enabled[b]) want_dc *= magAt(coeffs[b], 0.0);
    }
    double actual_dc = 0.0;
    for (int k = 0; k < N; ++k) actual_dc += shifted[static_cast<std::size_t>(k)];
    if (std::abs(actual_dc) > 1e-12) {
        double scale = want_dc / actual_dc;
        for (int k = 0; k < N; ++k) shifted[static_cast<std::size_t>(k)] *= scale;
    }

    for (int k = 0; k < N; ++k) m_kernel[k] = shifted[static_cast<std::size_t>(k)];

    m_dirty = false;
}

int LinearPhaseEq::getLatency() const {
    return m_kernel_size / 2;
}

double LinearPhaseEq::processSample(int ch, double input) {
    if (m_dirty) rebuild();
    if (ch < 0 || ch >= m_channels) return input;

    int base = ch * m_kernel_size;
    int idx = m_hist_idx[static_cast<std::size_t>(ch)];

    m_history[static_cast<std::size_t>(base + idx)] = input;

    double out = 0.0;
    int rd = idx;
    const double* hist_ptr = m_history.data() + base;
    for (int k = 0; k < m_kernel_size; ++k) {
        out += m_kernel[k] * hist_ptr[rd];
        if (--rd < 0) rd = m_kernel_size - 1;
    }

    m_hist_idx[static_cast<std::size_t>(ch)] = (idx + 1) % m_kernel_size;
    return out;
}

void LinearPhaseEq::processBlock(const double* input, double* output, int frames, int channels) {
    if (m_dirty) rebuild();
    if (channels != m_channels) {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                output[f * channels + c] = input[f * channels + c];
            }
        }
        return;
    }

    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            output[f * channels + c] = processSample(c, input[f * channels + c]);
        }
    }
}

void LinearPhaseEq::processBlockFloat(const float* input, float* output, int frames, int channels) {
    if (m_dirty) rebuild();
    if (channels != m_channels) {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                output[f * channels + c] = input[f * channels + c];
            }
        }
        return;
    }

    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            output[f * channels + c] = static_cast<float>(
                processSample(c, static_cast<double>(input[f * channels + c])));
        }
    }
}

}
