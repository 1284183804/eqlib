#include "cl.h"
#include <cmath>

namespace eqlib {

SidechainProcessor::SidechainProcessor() {
    m_envelope.resize(m_channels);
    setSampleRate(48000.0);
    setAttack(10.0);
    setRelease(200.0);
}

void SidechainProcessor::setSampleRate(double sr) {
    m_sample_rate = sr;
    m_attack_coeff = std::exp(-1.0 / (sr * 10.0 / 1000.0));
    m_release_coeff = std::exp(-1.0 / (sr * 200.0 / 1000.0));
}

void SidechainProcessor::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    m_envelope.resize(ch);
    for (int c = 0; c < ch; ++c) m_envelope[c] = 0.0;
}

int SidechainProcessor::getChannels() const {
    return m_channels;
}

void SidechainProcessor::setAttack(double ms) {
    if (ms <= 0.0) return;
    m_attack_coeff = std::exp(-1.0 / (m_sample_rate * ms / 1000.0));
}

void SidechainProcessor::setRelease(double ms) {
    if (ms <= 0.0) return;
    m_release_coeff = std::exp(-1.0 / (m_sample_rate * ms / 1000.0));
}

void SidechainProcessor::setAmount(double amount) {
    if (amount < 0.0) amount = 0.0;
    if (amount > 1.0) amount = 1.0;
    m_amount = amount;
}

void SidechainProcessor::setTarget(double target_db) { m_target_db = target_db; }
void SidechainProcessor::setRange(double range_db)   { m_range_db = range_db; }
void SidechainProcessor::setEnable(bool enable)      { m_enable = enable; }

void SidechainProcessor::reset() {
    for (int c = 0; c < m_envelope.size(); ++c) m_envelope[c] = 0.0;
}

double SidechainProcessor::process(double sidechain_input, int ch) {
    if (!m_enable) return 1.0;
    if (ch < 0 || ch >= m_channels) return 1.0;

    double abs_in = std::abs(sidechain_input);
    double env = m_envelope[ch];
    if (abs_in > env) {
        env = m_attack_coeff * env + (1.0 - m_attack_coeff) * abs_in;
    } else {
        env = m_release_coeff * env + (1.0 - m_release_coeff) * abs_in;
    }
    m_envelope[ch] = env;

    double env_db = 20.0 * std::log10(env + 1e-12);
    double gain_db = (m_target_db - env_db) * m_amount;
    if (gain_db > m_range_db) gain_db = m_range_db;
    if (gain_db < -m_range_db) gain_db = -m_range_db;

    return std::pow(10.0, gain_db / 20.0);
}

double SidechainProcessor::getEnvelope() const {
    if (m_envelope.size() <= 0) return 0.0;
    double s = 0.0;
    for (int c = 0; c < m_envelope.size(); ++c) s += m_envelope[c];
    return s / static_cast<double>(m_envelope.size());
}

double SidechainProcessor::getEnable() const { return m_enable ? 1.0 : 0.0; }
double SidechainProcessor::getAmount() const { return m_amount; }

double SidechainProcessor::getAttack() const {
    if (m_attack_coeff <= 0.0) return 0.0;
    return -1000.0 / m_sample_rate * std::log(m_attack_coeff);
}

double SidechainProcessor::getRelease() const {
    if (m_release_coeff <= 0.0) return 0.0;
    return -1000.0 / m_sample_rate * std::log(m_release_coeff);
}

double SidechainProcessor::getTarget() const { return m_target_db; }
double SidechainProcessor::getRange()  const { return m_range_db; }

}
