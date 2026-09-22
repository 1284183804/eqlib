#include "jhqhx.h"
#include <cmath>

namespace eqlib {

EqCore::EqCore() {
    for (int i = 0; i < MAX_BANDS; ++i) {
        m_filters[i].resize(m_channels);
        m_band_type[i] = FilterType::Peaking;
        m_band_freq[i] = 1000.0;
        m_band_q[i] = 0.707;
        m_band_enable[i] = true;
        m_dirty[i] = true;
    }
}

void EqCore::setSampleRate(double sr) {
    m_sample_rate = sr;
    for (int i = 0; i < MAX_BANDS; ++i) {
        for (int c = 0; c < m_filters[i].size(); ++c) {
            m_filters[i][c].setSampleRate(sr);
            m_filters[i][c].reset();
        }
        m_dirty[i] = true;
    }
}

void EqCore::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    for (int i = 0; i < MAX_BANDS; ++i) {
        m_filters[i].resize(ch);
        m_dirty[i] = true;
    }
}

int EqCore::getChannels() const {
    return m_channels;
}

int EqCore::getNumBands() const {
    return m_num_bands;
}

void EqCore::setNumBands(int n) {
    if (n < 1) n = 1;
    if (n > MAX_BANDS) n = MAX_BANDS;
    m_num_bands = n;
}

void EqCore::setBandType(int band, FilterType type) {
    if (band < 0 || band >= MAX_BANDS) return;
    m_band_type[band] = type;
    m_dirty[band] = true;
}

void EqCore::setBandFreq(int band, double freq_hz) {
    if (band < 0 || band >= MAX_BANDS) return;
    m_band_freq[band] = freq_hz;
    m_dirty[band] = true;
}

void EqCore::setBandGain(int band, double gain_db) {
    if (band < 0 || band >= MAX_BANDS) return;
    ParamChange pc{};
    pc.band = band;
    pc.gain_db = gain_db;
    pc.mask = PARAM_MASK_GAIN;
    if (!m_param_queue.push(pc)) {
        m_target_gain[band] = gain_db;
        m_dirty[band] = true;
    }
}

void EqCore::setBandQ(int band, double q) {
    if (band < 0 || band >= MAX_BANDS) return;
    m_band_q[band] = q;
    m_dirty[band] = true;
}

void EqCore::setBandEnable(int band, bool enable) {
    if (band < 0 || band >= MAX_BANDS) return;
    m_band_enable[band] = enable;
    m_dirty[band] = true;
}

void EqCore::setBandRange(int band, double freq_low, double freq_high) {
    if (band < 0 || band >= MAX_BANDS) return;
    if (freq_low < FREQ_MIN_HZ) freq_low = FREQ_MIN_HZ;
    if (freq_high > FREQ_MAX_HZ) freq_high = FREQ_MAX_HZ;
    if (freq_low >= freq_high) return;
    m_band_range[band].freq_low_hz = freq_low;
    m_band_range[band].freq_high_hz = freq_high;
    m_range_set[band] = true;
    double center = std::sqrt(freq_low * freq_high);
    double bw = freq_high - freq_low;
    double q = center / bw;
    if (q < Q_MIN) q = Q_MIN;
    if (q > Q_MAX) q = Q_MAX;
    m_band_freq[band] = center;
    m_band_q[band] = q;
    m_dirty[band] = true;
}

void EqCore::clearBandRange(int band) {
    if (band < 0 || band >= MAX_BANDS) return;
    m_range_set[band] = false;
}

bool EqCore::getBandRange(int band, double* out_low, double* out_high) const {
    if (band < 0 || band >= MAX_BANDS) return false;
    if (!m_range_set[band]) return false;
    if (out_low) *out_low = m_band_range[band].freq_low_hz;
    if (out_high) *out_high = m_band_range[band].freq_high_hz;
    return true;
}

void EqCore::setGlobalGain(double gain_db) {
    m_global_gain_db = gain_db;
    m_global_gain_linear = std::pow(10.0, gain_db / 20.0);
}

double EqCore::getGlobalGain() const {
    return m_global_gain_db;
}

void EqCore::setBuiltinCurve(BuiltinCurve curve) {
    m_builtin_curve = curve;
}

BuiltinCurve EqCore::getBuiltinCurve() const {
    return m_builtin_curve;
}

int EqCore::setCustomCurve(const CurvePoint* points, int num_points) {
    if (!points || num_points <= 0 || num_points > CURVE_MAX_POINTS) return -1;
    for (int i = 0; i < num_points; ++i) {
        m_custom_curve[i] = points[i];
    }
    m_custom_curve_points = num_points;
    m_builtin_curve = BuiltinCurve::Custom;
    return 0;
}

int EqCore::applyBuiltinCurveToBands() {
    if (m_builtin_curve == BuiltinCurve::Custom) {
        return applyCustomCurveToBands();
    }
    const BuiltinCurveData& d = BuiltinCurves::get(m_builtin_curve);
    if (d.num_points <= 0 || !d.points) return -1;
    for (int i = 0; i < m_num_bands; ++i) {
        int idx = (i * d.num_points) / m_num_bands;
        if (idx >= d.num_points) idx = d.num_points - 1;
        if (idx < 0) idx = 0;
        m_band_freq[i] = d.points[idx].freq_hz;
        m_target_gain[i] = d.points[idx].gain_db;
        m_band_type[i] = FilterType::Peaking;
        m_band_q[i] = 0.707;
        m_band_enable[i] = true;
        m_dirty[i] = true;
    }
    return 0;
}

int EqCore::applyCustomCurveToBands() {
    if (m_custom_curve_points <= 0) return -1;
    for (int i = 0; i < m_num_bands; ++i) {
        int idx = (i * m_custom_curve_points) / m_num_bands;
        if (idx >= m_custom_curve_points) idx = m_custom_curve_points - 1;
        if (idx < 0) idx = 0;
        m_band_freq[i] = m_custom_curve[idx].freq_hz;
        m_target_gain[i] = m_custom_curve[idx].gain_db;
        m_band_type[i] = FilterType::Peaking;
        m_band_q[i] = 0.707;
        m_band_enable[i] = true;
        m_dirty[i] = true;
    }
    return 0;
}

int EqCore::autoDetectCurveFromSpectrum(const double* mags, const double* freqs,
                                         int num_bins, double sample_rate) {
    BuiltinCurve detected = BuiltinCurves::detectFromSpectrum(mags, freqs, num_bins, sample_rate);
    m_builtin_curve = detected;
    return applyBuiltinCurveToBands();
}

void EqCore::reset() {
    for (int i = 0; i < MAX_BANDS; ++i) {
        for (int c = 0; c < m_filters[i].size(); ++c) {
            m_filters[i][c].reset();
        }
        m_current_gain[i] = m_target_gain[i];
        m_last_applied_gain[i] = m_target_gain[i];
        m_dirty[i] = true;
    }
    m_param_queue.drain();
}

void EqCore::consumeParamQueue() {
    ParamChange pc{};
    while (m_param_queue.pop(pc)) {
        if (pc.band < 0 || pc.band >= MAX_BANDS) continue;
        if (pc.mask & PARAM_MASK_GAIN) {
            m_target_gain[pc.band] = pc.gain_db;
        }
        if (pc.mask & PARAM_MASK_TYPE) {
            m_band_type[pc.band] = static_cast<FilterType>(pc.type);
            m_dirty[pc.band] = true;
        }
        if (pc.mask & PARAM_MASK_FREQ) {
            m_band_freq[pc.band] = pc.freq_hz;
            m_dirty[pc.band] = true;
        }
        if (pc.mask & PARAM_MASK_Q) {
            m_band_q[pc.band] = pc.q;
            m_dirty[pc.band] = true;
        }
        if (pc.mask & PARAM_MASK_ENABLE) {
            m_band_enable[pc.band] = (pc.enable != 0);
            m_dirty[pc.band] = true;
        }
    }
}

double EqCore::processSample(double input, int ch) {
    double output = input;
    if (ch < 0 || ch >= m_channels) return output * m_global_gain_linear;

    for (int i = 0; i < m_num_bands; ++i) {
        if (!m_band_enable[i]) continue;
        double diff = m_target_gain[i] - m_current_gain[i];
        if (std::abs(diff) > 0.001) {
            m_current_gain[i] += diff * m_smoothing_coeff;
        } else {
            m_current_gain[i] = m_target_gain[i];
        }
        if (std::abs(m_current_gain[i] - m_last_applied_gain[i]) > 0.005 || m_dirty[i]) {
            m_filters[i][ch].setParams(m_band_type[i], m_band_freq[i],
                                        m_current_gain[i], m_band_q[i]);
            m_last_applied_gain[i] = m_current_gain[i];
            m_dirty[i] = false;
        }
        output = m_filters[i][ch].processSample(output);
    }
    return output * m_global_gain_linear;
}

void EqCore::processBlock(const double* input, double* output, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            output[f * channels + c] = processSample(input[f * channels + c], c);
        }
        for (int c = ch_count; c < channels; ++c) {
            output[f * channels + c] = input[f * channels + c];
        }
    }
}

void EqCore::processBlockFloat(const float* input, float* output, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            output[f * channels + c] = static_cast<float>(
                processSample(static_cast<double>(input[f * channels + c]), c));
        }
        for (int c = ch_count; c < channels; ++c) {
            output[f * channels + c] = input[f * channels + c];
        }
    }
}

double EqCore::getBandGain(int band) const {
    if (band < 0 || band >= MAX_BANDS) return 0.0;
    return m_target_gain[band];
}

FilterType EqCore::getBandType(int band) const {
    if (band < 0 || band >= MAX_BANDS) return FilterType::Peaking;
    return m_band_type[band];
}

double EqCore::getBandFreq(int band) const {
    if (band < 0 || band >= MAX_BANDS) return 0.0;
    return m_band_freq[band];
}

double EqCore::getBandQ(int band) const {
    if (band < 0 || band >= MAX_BANDS) return 0.0;
    return m_band_q[band];
}

bool EqCore::getBandEnable(int band) const {
    if (band < 0 || band >= MAX_BANDS) return false;
    return m_band_enable[band];
}

}
