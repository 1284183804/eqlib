#include "zdjhq.h"
#include <cmath>

namespace eqlib {

namespace {

double interpCurve(const std::vector<double>& freqs,
                   const std::vector<double>& values,
                   int n, double f) {
    if (n <= 0) return 0.0;
    if (f <= freqs[0]) return values[0];
    if (f >= freqs[static_cast<std::size_t>(n - 1)])
        return values[static_cast<std::size_t>(n - 1)];
    int lo = 0;
    int hi = n - 1;
    while (lo + 1 < hi) {
        int mid = (lo + hi) / 2;
        if (freqs[static_cast<std::size_t>(mid)] <= f) lo = mid;
        else hi = mid;
    }
    double f0 = freqs[static_cast<std::size_t>(lo)];
    double f1 = freqs[static_cast<std::size_t>(hi)];
    if (f1 <= f0) return values[static_cast<std::size_t>(lo)];
    double t = (f - f0) / (f1 - f0);
    return values[static_cast<std::size_t>(lo)] * (1.0 - t)
         + values[static_cast<std::size_t>(hi)] * t;
}

const double kDefaultBandFreqs[DEFAULT_BANDS] = {
    80.0, 200.0, 500.0, 1000.0, 2500.0, 6000.0, 12000.0
};

}

AutoEqCore::AutoEqCore() {
    m_band_freqs.assign(kDefaultBandFreqs, kDefaultBandFreqs + DEFAULT_BANDS);
    m_reference_freqs_hz.reserve(AUTO_EQ_MAX_POINTS);
    m_reference_gains_db.reserve(AUTO_EQ_MAX_POINTS);
    m_measured_freqs_hz.reserve(AUTO_EQ_MAX_POINTS);
    m_measured_db.reserve(AUTO_EQ_MAX_POINTS);
}

void AutoEqCore::setSampleRate(double sr) {
    if (sr < SR_MIN || sr > SR_MAX) return;
    m_sample_rate = sr;
}

void AutoEqCore::setFftSize(int size) {
    if (size <= 0 || size > FFT_MAX_SIZE) return;
    m_fft_size = size;
}

void AutoEqCore::setWindow(WindowType type) {
    m_window = type;
}

void AutoEqCore::setPct(double pct) {
    if (pct < 0.0) pct = 0.0;
    if (pct > 200.0) pct = 200.0;
    m_pct = pct;
}

int AutoEqCore::setNumBands(int n) {
    if (n < 1 || n > MAX_BANDS) return -1;
    int old_n = m_num_bands;
    m_num_bands = n;
    m_band_freqs.resize(static_cast<std::size_t>(n));
    if (n > old_n) {
        for (int i = old_n; i < n; ++i) {
            double t = static_cast<double>(i) / static_cast<double>(n - 1 > 0 ? n - 1 : 1);
            m_band_freqs[static_cast<std::size_t>(i)] =
                FREQ_MIN_HZ * std::pow(FREQ_MAX_HZ / FREQ_MIN_HZ, t);
        }
    }
    return 0;
}

int AutoEqCore::getNumBands() const {
    return m_num_bands;
}

int AutoEqCore::setBandFreq(int band, double freq_hz) {
    if (band < 0 || band >= m_num_bands) return -1;
    if (freq_hz < FREQ_MIN_HZ || freq_hz > FREQ_MAX_HZ) return -1;
    m_band_freqs[static_cast<std::size_t>(band)] = freq_hz;
    return 0;
}

int AutoEqCore::setBandFreqs(const double* freqs_hz, int n) {
    if (!freqs_hz) return -1;
    if (n < 1 || n > MAX_BANDS) return -1;
    for (int i = 0; i < n; ++i) {
        if (!std::isfinite(freqs_hz[i])) return -1;
        if (freqs_hz[i] < FREQ_MIN_HZ || freqs_hz[i] > FREQ_MAX_HZ) return -1;
    }
    m_num_bands = n;
    m_band_freqs.assign(freqs_hz, freqs_hz + n);
    return 0;
}

bool AutoEqCore::getBandFreq(int band, double& out_freq_hz) const {
    if (band < 0 || band >= m_num_bands) return false;
    out_freq_hz = m_band_freqs[static_cast<std::size_t>(band)];
    return true;
}

bool AutoEqCore::setReferenceCurve(const double* freqs_hz, const double* gains_db, int num_points) {
    if (!freqs_hz || !gains_db || num_points <= 0) return false;
    if (num_points > AUTO_EQ_MAX_POINTS) return false;
    for (int i = 0; i < num_points; ++i) {
        if (!std::isfinite(freqs_hz[i]) || !std::isfinite(gains_db[i])) return false;
        if (i > 0 && freqs_hz[i] < freqs_hz[i - 1]) return false;
    }
    m_reference_freqs_hz.assign(freqs_hz, freqs_hz + num_points);
    m_reference_gains_db.assign(gains_db, gains_db + num_points);
    m_reference_points = num_points;
    m_reference_set = true;
    return true;
}

void AutoEqCore::clearReferenceCurve() {
    m_reference_freqs_hz.clear();
    m_reference_gains_db.clear();
    m_reference_points = 0;
    m_reference_set = false;
}

bool AutoEqCore::hasReferenceCurve() const {
    return m_reference_set;
}

int AutoEqCore::getReferencePointCount() const {
    return m_reference_points;
}

bool AutoEqCore::getReferencePoint(int index, double& freq_hz, double& gain_db) const {
    if (index < 0 || index >= m_reference_points) return false;
    freq_hz = m_reference_freqs_hz[static_cast<std::size_t>(index)];
    gain_db = m_reference_gains_db[static_cast<std::size_t>(index)];
    return true;
}

void AutoEqCore::start() {
    m_running = true;
    m_has_result = false;
}

void AutoEqCore::stop() {
    m_running = false;
}

bool AutoEqCore::isRunning() const {
    return m_running;
}

bool AutoEqCore::hasResult() const {
    return m_has_result;
}

void AutoEqCore::feedSpectrum(const SpectrumFrame& frame) {
    if (!m_running) return;
    if (frame.num_bins <= 0) return;
    int n = frame.num_bins;
    if (n > AUTO_EQ_MAX_POINTS) n = AUTO_EQ_MAX_POINTS;
    m_measured_points = n;
    m_measured_freqs_hz.resize(static_cast<std::size_t>(n));
    m_measured_db.resize(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        m_measured_freqs_hz[static_cast<std::size_t>(i)] = frame.freqs_hz[i];
        double mag = frame.magnitudes[i];
        if (mag < 1e-12) mag = 1e-12;
        m_measured_db[static_cast<std::size_t>(i)] = 20.0 * std::log10(mag);
    }
}

bool AutoEqCore::computeResult() {
    if (!m_reference_set) return false;
    if (m_measured_points <= 0) return false;
    if (m_reference_points <= 0) return false;
    if (m_num_bands <= 0) return false;

    double meas_mean = 0.0;
    for (int i = 0; i < m_measured_points; ++i) {
        meas_mean += m_measured_db[static_cast<std::size_t>(i)];
    }
    meas_mean /= static_cast<double>(m_measured_points);

    double ref_mean = 0.0;
    for (int i = 0; i < m_reference_points; ++i) {
        ref_mean += m_reference_gains_db[static_cast<std::size_t>(i)];
    }
    ref_mean /= static_cast<double>(m_reference_points);

    double total_error = 0.0;
    double max_error = 0.0;

    for (int b = 0; b < m_num_bands; ++b) {
        double f = m_band_freqs[static_cast<std::size_t>(b)];
        double ref_raw = interpCurve(m_reference_freqs_hz, m_reference_gains_db,
                                     m_reference_points, f);
        double meas_raw = interpCurve(m_measured_freqs_hz, m_measured_db,
                                      m_measured_points, f);

        double ref_rel = ref_raw - ref_mean;
        double meas_rel = meas_raw - meas_mean;

        double diff = ref_rel - meas_rel;
        diff *= m_pct / 100.0;
        if (diff > GAIN_MAX_DB) diff = GAIN_MAX_DB;
        if (diff < GAIN_MIN_DB) diff = GAIN_MIN_DB;

        m_last_output.gains_db[b] = diff;
        total_error += diff * diff;
        if (std::fabs(diff) > max_error) max_error = std::fabs(diff);
    }

    m_last_output.rms_error_db =
        std::sqrt(total_error / static_cast<double>(m_num_bands));
    m_last_output.max_error_db = max_error;
    m_has_result = true;
    return true;
}

AutoEqOutput AutoEqCore::getResult() const {
    return m_last_output;
}

void AutoEqCore::reset() {
    m_measured_points = 0;
    m_has_result = false;
    m_running = false;
    m_last_output = AutoEqOutput{};
}

}
