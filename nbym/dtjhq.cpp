#include "dtjhq.h"
#include <cmath>

namespace eqlib {

namespace {

double calcCoeff(double ms, double sr) {
    if (ms <= 0.0) return 0.0;
    return std::exp(-1.0 / (sr * ms / 1000.0));
}

double interpCurveGain(const CurveData& c, double f) {
    if (c.num_points <= 0) return 0.0;
    if (f <= c.freqs_hz[0]) return c.gains_db[0];
    if (f >= c.freqs_hz[c.num_points - 1]) return c.gains_db[c.num_points - 1];
    int lo = 0;
    int hi = c.num_points - 1;
    while (lo + 1 < hi) {
        int mid = (lo + hi) / 2;
        if (c.freqs_hz[mid] <= f) lo = mid;
        else hi = mid;
    }
    double f0 = c.freqs_hz[lo];
    double f1 = c.freqs_hz[hi];
    if (f1 <= f0) return c.gains_db[lo];
    double t = (f - f0) / (f1 - f0);
    return c.gains_db[lo] * (1.0 - t) + c.gains_db[hi] * t;
}

void buildFlatCurve(CurveData& c) {
    c.num_points = CURVE_MAX_POINTS;
    c.source = static_cast<int>(CurveSource::Flat);
    c.is_measured = false;
    c.reference_db = 0.0;
    double span = (FREQ_MAX_HZ - FREQ_MIN_HZ) / static_cast<double>(CURVE_MAX_POINTS - 1);
    for (int i = 0; i < CURVE_MAX_POINTS; ++i) {
        c.freqs_hz[i] = FREQ_MIN_HZ + span * static_cast<double>(i);
        c.levels_db[i] = 0.0;
        c.gains_db[i] = 0.0;
    }
}

void measuredToGain(CurveData& c) {
    if (c.num_points <= 0) return;
    double sum = 0.0;
    for (int i = 0; i < c.num_points; ++i) {
        sum += c.levels_db[i];
    }
    double mean = sum / static_cast<double>(c.num_points);
    for (int i = 0; i < c.num_points; ++i) {
        double g = mean - c.levels_db[i];
        if (g > GAIN_MAX_DB) g = GAIN_MAX_DB;
        if (g < GAIN_MIN_DB) g = GAIN_MIN_DB;
        c.gains_db[i] = g;
    }
    c.reference_db = mean;
    c.is_measured = true;
}

}

MultiBandDynEq::MultiBandDynEq() {
    buildFlatCurve(m_curve);
    for (int i = 0; i < MAX_BANDS; ++i) {
        m_bands[i].filter.resize(m_channels);
        m_bands[i].envelope.resize(m_channels);
        m_bands[i].cfg.low_hz = 20.0;
        m_bands[i].cfg.high_hz = 20000.0;
        m_bands[i].cfg.center_hz = 1000.0;
        m_bands[i].cfg.q = 1.0;
        m_bands[i].cfg.gain_db = 0.0;
        m_bands[i].cfg.type = static_cast<int>(FilterType::Peaking);
        m_bands[i].cfg.enable = 1;
        m_bands[i].cfg.target_dbfs = -18.0;
        m_bands[i].cfg.dyn_range_db = 12.0;
        m_bands[i].cfg.dyn_attack_ms = 10.0;
        m_bands[i].cfg.dyn_release_ms = 200.0;
        for (int c = 0; c < m_channels; ++c) {
            m_bands[i].envelope[c] = 0.0;
        }
        m_bands[i].current_gain_db = 0.0;
        m_bands[i].target_gain_db = 0.0;
        m_bands[i].last_applied_gain_db = 0.0;
        m_bands[i].dirty = true;
    }
}

void MultiBandDynEq::setSampleRate(double sr) {
    m_sample_rate = sr;
    for (int i = 0; i < MAX_BANDS; ++i) {
        for (int c = 0; c < m_bands[i].filter.size(); ++c) {
            m_bands[i].filter[c].setSampleRate(sr);
            m_bands[i].filter[c].reset();
        }
        m_bands[i].dirty = true;
    }
}

void MultiBandDynEq::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    if (ch == m_channels) return;
    m_channels = ch;
    for (int i = 0; i < MAX_BANDS; ++i) {
        m_bands[i].filter.resize(ch);
        m_bands[i].envelope.resize(ch);
        for (int c = 0; c < ch; ++c) {
            m_bands[i].envelope[c] = 0.0;
        }
        m_bands[i].dirty = true;
    }
}

double MultiBandDynEq::getSampleRate() const { return m_sample_rate; }
int    MultiBandDynEq::getChannels() const { return m_channels; }

int MultiBandDynEq::setNumBands(int n) {
    if (n < 1 || n > MAX_BANDS) return -1;
    m_num_bands = n;
    return 0;
}

int MultiBandDynEq::getNumBands() const { return m_num_bands; }

int MultiBandDynEq::setBandConfig(int band, const BandConfig& cfg) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    m_bands[band].cfg = cfg;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::getBandConfig(int band, BandConfig& out) const {
    if (band < 0 || band >= MAX_BANDS) return -1;
    out = m_bands[band].cfg;
    return 0;
}

int MultiBandDynEq::setBandRange(int band, double low_hz, double high_hz) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (low_hz < FREQ_MIN_HZ || high_hz > FREQ_MAX_HZ) return -1;
    if (low_hz >= high_hz) return -1;
    m_bands[band].cfg.low_hz = low_hz;
    m_bands[band].cfg.high_hz = high_hz;
    m_bands[band].cfg.center_hz = std::sqrt(low_hz * high_hz);
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandCenter(int band, double center_hz) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (center_hz < FREQ_MIN_HZ || center_hz > FREQ_MAX_HZ) return -1;
    m_bands[band].cfg.center_hz = center_hz;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandQ(int band, double q) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (q < Q_MIN || q > Q_MAX) return -1;
    m_bands[band].cfg.q = q;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandGain(int band, double gain_db) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (gain_db < GAIN_MIN_DB || gain_db > GAIN_MAX_DB) return -1;
    m_bands[band].cfg.gain_db = gain_db;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandType(int band, int type) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (type < 0 || type > 7) return -1;
    m_bands[band].cfg.type = type;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandEnable(int band, int enable) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    m_bands[band].cfg.enable = enable != 0 ? 1 : 0;
    m_bands[band].dirty = true;
    return 0;
}

int MultiBandDynEq::setBandTargetDbfs(int band, double target) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    m_bands[band].cfg.target_dbfs = target;
    return 0;
}

int MultiBandDynEq::setBandDynRange(int band, double range_db) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (range_db < 0.0) return -1;
    m_bands[band].cfg.dyn_range_db = range_db;
    return 0;
}

int MultiBandDynEq::setBandDynAttack(int band, double ms) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (ms <= 0.0) return -1;
    m_bands[band].cfg.dyn_attack_ms = ms;
    return 0;
}

int MultiBandDynEq::setBandDynRelease(int band, double ms) {
    if (band < 0 || band >= MAX_BANDS) return -1;
    if (ms <= 0.0) return -1;
    m_bands[band].cfg.dyn_release_ms = ms;
    return 0;
}

int MultiBandDynEq::setBandTargetDbfsAll(double target) {
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].cfg.target_dbfs = target;
    return 0;
}

int MultiBandDynEq::setBandDynRangeAll(double range_db) {
    if (range_db < 0.0) return -1;
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].cfg.dyn_range_db = range_db;
    return 0;
}

int MultiBandDynEq::setBandDynAttackAll(double ms) {
    if (ms <= 0.0) return -1;
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].cfg.dyn_attack_ms = ms;
    return 0;
}

int MultiBandDynEq::setBandDynReleaseAll(double ms) {
    if (ms <= 0.0) return -1;
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].cfg.dyn_release_ms = ms;
    return 0;
}

int MultiBandDynEq::loadMeasuredCurve(const double* freqs_hz, const double* levels_db,
                                       int num_points, double reference_db) {
    if (!freqs_hz || !levels_db) return -1;
    if (num_points <= 0 || num_points > CURVE_MAX_POINTS) return -1;
    for (int i = 0; i < num_points; ++i) {
        if (!std::isfinite(freqs_hz[i]) || !std::isfinite(levels_db[i])) return -1;
        if (i > 0 && freqs_hz[i] < freqs_hz[i - 1]) return -1;
    }
    for (int i = 0; i < num_points; ++i) {
        m_curve.freqs_hz[i] = freqs_hz[i];
        m_curve.levels_db[i] = levels_db[i];
        m_curve.gains_db[i] = 0.0;
    }
    m_curve.num_points = num_points;
    m_curve.source = static_cast<int>(CurveSource::Injected);
    measuredToGain(m_curve);
    (void)reference_db;
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].dirty = true;
    return 0;
}

int MultiBandDynEq::loadGainCurve(const double* freqs_hz, const double* gains_db,
                                   int num_points, int source) {
    if (!freqs_hz || !gains_db) return -1;
    if (num_points <= 0 || num_points > CURVE_MAX_POINTS) return -1;
    if (source < 0 || source > 2) return -1;
    for (int i = 0; i < num_points; ++i) {
        if (!std::isfinite(freqs_hz[i]) || !std::isfinite(gains_db[i])) return -1;
        if (i > 0 && freqs_hz[i] < freqs_hz[i - 1]) return -1;
    }
    for (int i = 0; i < num_points; ++i) {
        m_curve.freqs_hz[i] = freqs_hz[i];
        m_curve.gains_db[i] = gains_db[i];
        m_curve.levels_db[i] = 0.0;
    }
    m_curve.num_points = num_points;
    m_curve.source = source;
    m_curve.is_measured = false;
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].dirty = true;
    return 0;
}

int MultiBandDynEq::loadBuiltinCurve() {
    buildFlatCurve(m_curve);
    for (int i = 0; i < m_num_bands; ++i) m_bands[i].dirty = true;
    return 0;
}

int MultiBandDynEq::clearCurve() {
    buildFlatCurve(m_curve);
    return 0;
}

int MultiBandDynEq::getCurve(CurveData& out) const {
    out = m_curve;
    return 0;
}

const CurveData& MultiBandDynEq::getCurveRef() const {
    return m_curve;
}

void MultiBandDynEq::reset() {
    for (int i = 0; i < MAX_BANDS; ++i) {
        for (int c = 0; c < m_bands[i].filter.size(); ++c) {
            m_bands[i].filter[c].reset();
            m_bands[i].envelope[c] = 0.0;
        }
        m_bands[i].current_gain_db = 0.0;
        m_bands[i].target_gain_db = 0.0;
        m_bands[i].last_applied_gain_db = 0.0;
        m_bands[i].dirty = true;
    }
    m_param_queue.drain();
}

void MultiBandDynEq::consumeParamQueue() {
    ParamChange pc{};
    while (m_param_queue.pop(pc)) {
        if (pc.band < 0 || pc.band >= MAX_BANDS) continue;
        if (pc.mask & PARAM_MASK_GAIN)   m_bands[pc.band].cfg.gain_db   = pc.gain_db;
        if (pc.mask & PARAM_MASK_TYPE)   m_bands[pc.band].cfg.type      = pc.type;
        if (pc.mask & PARAM_MASK_FREQ)   m_bands[pc.band].cfg.center_hz = pc.freq_hz;
        if (pc.mask & PARAM_MASK_Q)      m_bands[pc.band].cfg.q         = pc.q;
        if (pc.mask & PARAM_MASK_ENABLE) m_bands[pc.band].cfg.enable    = pc.enable;
        m_bands[pc.band].dirty = true;
    }
}

double MultiBandDynEq::processSampleInternal(double input, int ch,
                                              bool use_sidechain, double sidechain_input) {
    double output = input;
    if (ch < 0 || ch >= m_channels) return output;

    for (int i = 0; i < m_num_bands; ++i) {
        if (!m_bands[i].cfg.enable) continue;

        double curve_db = interpCurveGain(m_curve, m_bands[i].cfg.center_hz);
        double target_static = m_bands[i].cfg.gain_db + curve_db;

        output = m_bands[i].filter[ch].processSample(output);

        double env_source = use_sidechain ? sidechain_input : output;
        double abs_env = std::fabs(env_source);
        double atk = calcCoeff(m_bands[i].cfg.dyn_attack_ms, m_sample_rate);
        double rel = calcCoeff(m_bands[i].cfg.dyn_release_ms, m_sample_rate);
        double env = m_bands[i].envelope[ch];

        if (abs_env > env) {
            env = atk * env + (1.0 - atk) * abs_env;
        } else {
            env = rel * env + (1.0 - rel) * abs_env;
        }
        m_bands[i].envelope[ch] = env;

        double env_db = 20.0 * std::log10(env + 1e-12);
        double dyn_db = 0.0;
        if (env_db > m_bands[i].cfg.target_dbfs) {
            dyn_db = -(env_db - m_bands[i].cfg.target_dbfs);
            if (dyn_db < -m_bands[i].cfg.dyn_range_db) {
                dyn_db = -m_bands[i].cfg.dyn_range_db;
            }
        }
        m_bands[i].target_gain_db = target_static + dyn_db;

        double diff = m_bands[i].target_gain_db - m_bands[i].current_gain_db;
        if (std::fabs(diff) > 0.001) {
            m_bands[i].current_gain_db += diff * m_smoothing_coeff;
        } else {
            m_bands[i].current_gain_db = m_bands[i].target_gain_db;
        }

        if (std::fabs(m_bands[i].current_gain_db - m_bands[i].last_applied_gain_db) > 0.005
            || m_bands[i].dirty) {
            m_bands[i].filter[ch].setParams(
                static_cast<FilterType>(m_bands[i].cfg.type),
                m_bands[i].cfg.center_hz,
                m_bands[i].current_gain_db,
                m_bands[i].cfg.q);
            m_bands[i].last_applied_gain_db = m_bands[i].current_gain_db;
            m_bands[i].dirty = false;
        }
    }
    return output;
}

double MultiBandDynEq::processSample(double input, int ch) {
    return processSampleInternal(input, ch, false, 0.0);
}

void MultiBandDynEq::processBlock(const double* in, double* out, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            out[f * channels + c] = processSampleInternal(in[f * channels + c], c, false, 0.0);
        }
        for (int c = ch_count; c < channels; ++c) {
            out[f * channels + c] = in[f * channels + c];
        }
    }
}

void MultiBandDynEq::processBlockFloat(const float* in, float* out, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            out[f * channels + c] = static_cast<float>(
                processSampleInternal(static_cast<double>(in[f * channels + c]), c, false, 0.0));
        }
        for (int c = ch_count; c < channels; ++c) {
            out[f * channels + c] = in[f * channels + c];
        }
    }
}

void MultiBandDynEq::processBlockWithSidechain(const double* in, const double* sc,
                                                double* out, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            double s = sc[f * channels + c];
            out[f * channels + c] = processSampleInternal(in[f * channels + c], c, true, s);
        }
        for (int c = ch_count; c < channels; ++c) {
            out[f * channels + c] = in[f * channels + c];
        }
    }
}

void MultiBandDynEq::processBlockWithSidechainFloat(const float* in, const float* sc,
                                                     float* out, int frames, int channels) {
    consumeParamQueue();
    int ch_count = (channels < m_channels) ? channels : m_channels;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < ch_count; ++c) {
            double s = static_cast<double>(sc[f * channels + c]);
            out[f * channels + c] = static_cast<float>(
                processSampleInternal(static_cast<double>(in[f * channels + c]), c, true, s));
        }
        for (int c = ch_count; c < channels; ++c) {
            out[f * channels + c] = in[f * channels + c];
        }
    }
}

int MultiBandDynEq::autoDistributeBands(double low_hz, double high_hz) {
    if (low_hz < FREQ_MIN_HZ) low_hz = FREQ_MIN_HZ;
    if (high_hz > FREQ_MAX_HZ) high_hz = FREQ_MAX_HZ;
    if (low_hz >= high_hz) return -1;
    if (m_num_bands < 1) return -1;
    double ratio = std::pow(high_hz / low_hz, 1.0 / static_cast<double>(m_num_bands));
    for (int i = 0; i < m_num_bands; ++i) {
        double lo = low_hz * std::pow(ratio, static_cast<double>(i));
        double hi = low_hz * std::pow(ratio, static_cast<double>(i + 1));
        m_bands[i].cfg.low_hz = lo;
        m_bands[i].cfg.high_hz = hi;
        m_bands[i].cfg.center_hz = std::sqrt(lo * hi);
        m_bands[i].cfg.q = 0.707;
        m_bands[i].cfg.type = static_cast<int>(FilterType::Peaking);
        m_bands[i].dirty = true;
    }
    return 0;
}

}
