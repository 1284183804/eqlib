#include "zdjhq.h"
#include <cmath>

namespace eqlib {

namespace {

double interpReference(const double* freqs, const double* gains, int n, double f) {
    if (n <= 0) return 0.0;
    if (f <= freqs[0]) return gains[0];
    if (f >= freqs[n - 1]) return gains[n - 1];
    for (int i = 0; i < n - 1; ++i) {
        if (f >= freqs[i] && f <= freqs[i + 1]) {
            double t = (f - freqs[i]) / (freqs[i + 1] - freqs[i]);
            return gains[i] * (1.0 - t) + gains[i + 1] * t;
        }
    }
    return 0.0;
}

}

AutoEqCore::AutoEqCore() = default;

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

bool AutoEqCore::setReferenceCurve(const double* freqs_hz, const double* gains_db, int num_points) {
    if (!freqs_hz || !gains_db || num_points <= 0) return false;
    if (num_points > AUTO_EQ_MAX_POINTS) return false;
    for (int i = 0; i < num_points; ++i) {
        if (!std::isfinite(freqs_hz[i]) || !std::isfinite(gains_db[i])) return false;
        if (i > 0 && freqs_hz[i] < freqs_hz[i - 1]) return false;
    }
    for (int i = 0; i < num_points; ++i) {
        m_reference_freqs_hz[i] = freqs_hz[i];
        m_reference_gains_db[i] = gains_db[i];
    }
    m_reference_points = num_points;
    m_reference_set = true;
    return true;
}

void AutoEqCore::clearReferenceCurve() {
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
    freq_hz = m_reference_freqs_hz[index];
    gain_db = m_reference_gains_db[index];
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
    for (int i = 0; i < n; ++i) {
        m_measured_freqs_hz[i] = frame.freqs_hz[i];
        double mag = frame.magnitudes[i];
        if (mag < 1e-12) mag = 1e-12;
        m_measured_db[i] = 20.0 * std::log10(mag);
    }
}

bool AutoEqCore::computeResult() {
    if (!m_reference_set) return false;
    if (m_measured_points <= 0) return false;
    if (m_reference_points <= 0) return false;

    double meas_mean = 0.0;
    for (int i = 0; i < m_measured_points; ++i) {
        meas_mean += m_measured_db[i];
    }
    meas_mean /= static_cast<double>(m_measured_points);

    double ref_mean = 0.0;
    for (int i = 0; i < m_reference_points; ++i) {
        ref_mean += m_reference_gains_db[i];
    }
    ref_mean /= static_cast<double>(m_reference_points);

    static const double band_freqs[NUM_BANDS] = {
        80.0, 200.0, 500.0, 1000.0, 2500.0, 6000.0, 12000.0
    };

    double total_error = 0.0;
    double max_error = 0.0;
    int count = 0;

    for (int b = 0; b < NUM_BANDS; ++b) {
        double f = band_freqs[b];
        double ref_raw = interpReference(m_reference_freqs_hz, m_reference_gains_db,
                                         m_reference_points, f);
        double meas_raw = interpReference(m_measured_freqs_hz, m_measured_db,
                                          m_measured_points, f);

        double ref_rel = ref_raw - ref_mean;
        double meas_rel = meas_raw - meas_mean;

        double diff = ref_rel - meas_rel;
        diff *= m_pct / 100.0;
        if (diff > GAIN_MAX_DB) diff = GAIN_MAX_DB;
        if (diff < GAIN_MIN_DB) diff = GAIN_MIN_DB;

        m_last_output.gains_db[b] = diff;
        total_error += diff * diff;
        if (std::abs(diff) > max_error) max_error = std::abs(diff);
        ++count;
    }

    if (count > 0) {
        m_last_output.rms_error_db = std::sqrt(total_error / count);
    } else {
        m_last_output.rms_error_db = 0.0;
    }
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
