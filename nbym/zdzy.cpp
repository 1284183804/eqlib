#include "zdzy.h"
#include <cmath>

namespace eqlib {

namespace {

double calcCoeff(double ms, double sr) {
    if (ms <= 0.0) return 0.0;
    return std::exp(-1.0 / (sr * ms / 1000.0));
}

}

AgcCore::AgcCore() {
    m_envelope.resize(m_channels);
    m_gain_linear.resize(m_channels);
    for (int c = 0; c < m_channels; ++c) {
        m_envelope[c] = 0.0;
        m_gain_linear[c] = 1.0;
    }
    setSampleRate(48000.0);
}

void AgcCore::setSampleRate(double sr) {
    m_sample_rate = sr;
    m_attack_coeff = calcCoeff(m_params.attack_ms, sr);
    m_release_coeff = calcCoeff(m_params.release_ms, sr);
}

void AgcCore::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    m_envelope.resize(ch);
    m_gain_linear.resize(ch);
    for (int c = 0; c < ch; ++c) {
        m_envelope[c] = 0.0;
        m_gain_linear[c] = 1.0;
    }
}

int AgcCore::getChannels() const { return m_channels; }

void AgcCore::setParams(const AgcParams& params) {
    m_params = params;
    m_attack_coeff = calcCoeff(m_params.attack_ms, m_sample_rate);
    m_release_coeff = calcCoeff(m_params.release_ms, m_sample_rate);
}

AgcParams AgcCore::getParams() const { return m_params; }
void AgcCore::setTarget(double target_dbfs) { m_params.target_dbfs = target_dbfs; }

void AgcCore::setAttack(double attack_ms) {
    if (attack_ms <= 0.0) return;
    m_params.attack_ms = attack_ms;
    m_attack_coeff = calcCoeff(attack_ms, m_sample_rate);
}

void AgcCore::setRelease(double release_ms) {
    if (release_ms <= 0.0) return;
    m_params.release_ms = release_ms;
    m_release_coeff = calcCoeff(release_ms, m_sample_rate);
}

void AgcCore::setMaxBoost(double max_boost_db) { m_params.max_boost_db = max_boost_db; }
void AgcCore::setMaxCut(double max_cut_db)     { m_params.max_cut_db = max_cut_db; }
void AgcCore::setGateEnable(bool enable)       { m_params.gate_enable = enable; }
void AgcCore::setGateThreshold(double threshold_dbfs) { m_params.gate_threshold_dbfs = threshold_dbfs; }

void AgcCore::reset() {
    for (int c = 0; c < m_envelope.size(); ++c) {
        m_envelope[c] = 0.0;
        m_gain_linear[c] = 1.0;
    }
}

double AgcCore::processSample(double input, int ch) {
    if (ch < 0 || ch >= m_channels) return input;

    double env = m_envelope[ch];
    double gain = m_gain_linear[ch];
    double abs_in = std::abs(input);

    if (m_params.gate_enable) {
        double env_db = 20.0 * std::log10(env + 1e-12);
        if (env_db < m_params.gate_threshold_dbfs) {
            return input * gain;
        }
    }

    if (abs_in > env) {
        env = m_attack_coeff * env + (1.0 - m_attack_coeff) * abs_in;
    } else {
        env = m_release_coeff * env + (1.0 - m_release_coeff) * abs_in;
    }
    m_envelope[ch] = env;

    double env_db = 20.0 * std::log10(env + 1e-12);
    double gain_db = m_params.target_dbfs - env_db;
    if (gain_db > m_params.max_boost_db) gain_db = m_params.max_boost_db;
    if (gain_db < m_params.max_cut_db)   gain_db = m_params.max_cut_db;

    double target_gain = std::pow(10.0, gain_db / 20.0);
    if (target_gain < gain) {
        gain = m_attack_coeff * gain + (1.0 - m_attack_coeff) * target_gain;
    } else {
        gain = m_release_coeff * gain + (1.0 - m_release_coeff) * target_gain;
    }
    m_gain_linear[ch] = gain;

    return input * gain;
}

void AgcCore::processBlock(const double* input, double* output, int frames, int channels) {
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

void AgcCore::processBlockFloat(const float* input, float* output, int frames, int channels) {
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

double AgcCore::getCurrentGainDb() const {
    if (m_gain_linear.size() <= 0) return 0.0;
    double s = 0.0;
    for (int c = 0; c < m_gain_linear.size(); ++c) s += m_gain_linear[c];
    double avg = s / static_cast<double>(m_gain_linear.size());
    return 20.0 * std::log10(avg + 1e-12);
}

double AgcCore::getEnvelopeDb() const {
    if (m_envelope.size() <= 0) return -120.0;
    double s = 0.0;
    for (int c = 0; c < m_envelope.size(); ++c) s += m_envelope[c];
    double avg = s / static_cast<double>(m_envelope.size());
    return 20.0 * std::log10(avg + 1e-12);
}

}
