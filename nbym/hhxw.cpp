#include "hhxw.h"

namespace eqlib {

HybridPhaseEq::HybridPhaseEq() {
    for (int i = 0; i < NUM_BANDS; ++i) {
        m_band_mode[i] = PhaseMode::Minimum;
        m_band_type[i] = FilterType::Peaking;
        m_band_freq[i] = 1000.0;
        m_band_gain[i] = 0.0;
        m_band_q[i] = 0.707;
        m_band_enable[i] = true;
    }
    m_min_phase.setSampleRate(m_sample_rate);
    m_min_phase.setChannels(m_channels);
    m_lin_phase.setSampleRate(m_sample_rate);
    m_lin_phase.setChannels(m_channels);
    applyAllBands();
}

void HybridPhaseEq::setSampleRate(double sr) {
    if (sr < SR_MIN || sr > SR_MAX) return;
    m_sample_rate = sr;
    m_min_phase.setSampleRate(sr);
    m_lin_phase.setSampleRate(sr);
    m_dirty = true;
}

void HybridPhaseEq::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    m_min_phase.setChannels(ch);
    m_lin_phase.setChannels(ch);
}

double HybridPhaseEq::getSampleRate() const { return m_sample_rate; }
int    HybridPhaseEq::getChannels() const   { return m_channels; }

void HybridPhaseEq::setBandPhaseMode(int band, PhaseMode mode) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_mode[band] = mode;
    applyBandToEngines(band);
}

void HybridPhaseEq::setBandType(int band, FilterType type) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_type[band] = type;
    applyBandToEngines(band);
}

void HybridPhaseEq::setBandFreq(int band, double freq_hz) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_freq[band] = freq_hz;
    applyBandToEngines(band);
}

void HybridPhaseEq::setBandGain(int band, double gain_db) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_gain[band] = gain_db;
    applyBandToEngines(band);
}

void HybridPhaseEq::setBandQ(int band, double q) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_q[band] = q;
    applyBandToEngines(band);
}

void HybridPhaseEq::setBandEnable(int band, bool enable) {
    if (band < 0 || band >= NUM_BANDS) return;
    m_band_enable[band] = enable;
    applyBandToEngines(band);
}

void HybridPhaseEq::setLinearKernelSize(int size) {
    m_lin_phase.setKernelSize(size);
    m_dirty = true;
}

PhaseMode HybridPhaseEq::getBandPhaseMode(int band) const {
    if (band < 0 || band >= NUM_BANDS) return PhaseMode::Minimum;
    return m_band_mode[band];
}

FilterType HybridPhaseEq::getBandType(int band) const {
    if (band < 0 || band >= NUM_BANDS) return FilterType::Peaking;
    return m_band_type[band];
}

double HybridPhaseEq::getBandFreq(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_freq[band];
}

double HybridPhaseEq::getBandGain(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_gain[band];
}

double HybridPhaseEq::getBandQ(int band) const {
    if (band < 0 || band >= NUM_BANDS) return 0.0;
    return m_band_q[band];
}

bool HybridPhaseEq::getBandEnable(int band) const {
    if (band < 0 || band >= NUM_BANDS) return false;
    return m_band_enable[band];
}

int HybridPhaseEq::getLinearKernelSize() const {
    return m_lin_phase.getKernelSize();
}

int HybridPhaseEq::getLatency() const {
    return m_lin_phase.getLatency();
}

void HybridPhaseEq::applyBandToEngines(int band) {
    if (band < 0 || band >= NUM_BANDS) return;

    bool is_min = (m_band_mode[band] == PhaseMode::Minimum
                || m_band_mode[band] == PhaseMode::Zero);
    bool is_lin = (m_band_mode[band] == PhaseMode::Linear);

    m_min_phase.setBandType(band, m_band_type[band]);
    m_min_phase.setBandFreq(band, m_band_freq[band]);
    m_min_phase.setBandGain(band, m_band_gain[band]);
    m_min_phase.setBandQ(band, m_band_q[band]);
    m_min_phase.setBandEnable(band, m_band_enable[band] && is_min);

    m_lin_phase.setBandType(band, m_band_type[band]);
    m_lin_phase.setBandFreq(band, m_band_freq[band]);
    m_lin_phase.setBandGain(band, m_band_gain[band]);
    m_lin_phase.setBandQ(band, m_band_q[band]);
    m_lin_phase.setBandEnable(band, m_band_enable[band] && is_lin);

    if (is_lin) m_dirty = true;
}

void HybridPhaseEq::applyAllBands() {
    for (int i = 0; i < NUM_BANDS; ++i) applyBandToEngines(i);
}

void HybridPhaseEq::rebuild() {
    m_lin_phase.rebuild();
    m_dirty = false;
}

void HybridPhaseEq::reset() {
    m_min_phase.reset();
    m_lin_phase.reset();
}

double HybridPhaseEq::processSample(double input, int ch) {
    if (m_dirty) rebuild();
    if (ch < 0 || ch >= m_channels) return input;
    double y = m_min_phase.processSample(input, ch);
    y = m_lin_phase.processSample(ch, y);
    return y;
}

void HybridPhaseEq::processBlock(const double* input, double* output, int frames, int channels) {
    if (m_dirty) rebuild();
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

void HybridPhaseEq::processBlockFloat(const float* input, float* output, int frames, int channels) {
    if (m_dirty) rebuild();
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

}
