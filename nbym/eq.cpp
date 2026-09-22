#include "eqlib/eq.hpp"
#include <cstring>
#include <string>

namespace eqlib {

struct EQ::Impl {
    eqlib_handle* handle{nullptr};
    int last_error{eqlib_ok};
    double sample_rate{48000.0};
    int channels{2};
};

EQ::EQ() : m_impl(std::make_unique<Impl>()) {
    m_impl->last_error = jhq_create(&m_impl->handle);
    if (m_impl->last_error == eqlib_ok && m_impl->handle) {
        jhq_set_sample_rate(m_impl->handle, m_impl->sample_rate);
        jhq_set_channels(m_impl->handle, m_impl->channels);
    }
}

EQ::EQ(double sample_rate, int channels) : m_impl(std::make_unique<Impl>()) {
    m_impl->sample_rate = sample_rate;
    m_impl->channels = channels;
    m_impl->last_error = jhq_create(&m_impl->handle);
    if (m_impl->last_error == eqlib_ok && m_impl->handle) {
        jhq_set_sample_rate(m_impl->handle, sample_rate);
        jhq_set_channels(m_impl->handle, channels);
    }
}

EQ::~EQ() {
    if (m_impl && m_impl->handle) {
        jhq_destroy(m_impl->handle);
        m_impl->handle = nullptr;
    }
}

EQ::EQ(EQ&& other) noexcept : m_impl(std::move(other.m_impl)) {}

EQ& EQ::operator=(EQ&& other) noexcept {
    if (this != &other) {
        if (m_impl && m_impl->handle) {
            jhq_destroy(m_impl->handle);
        }
        m_impl = std::move(other.m_impl);
    }
    return *this;
}

bool EQ::isValid() const {
    return m_impl && m_impl->handle != nullptr && m_impl->last_error == eqlib_ok;
}

int EQ::getLastError() const {
    return m_impl ? m_impl->last_error : eqlib_err_handle;
}

void EQ::setSampleRate(double sr) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_sample_rate(m_impl->handle, sr);
    if (m_impl->last_error == eqlib_ok) m_impl->sample_rate = sr;
}

double EQ::getSampleRate() const {
    return m_impl ? m_impl->sample_rate : 0.0;
}

void EQ::setChannels(int ch) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_channels(m_impl->handle, ch);
    if (m_impl->last_error == eqlib_ok) m_impl->channels = ch;
}

int EQ::getChannels() const {
    return m_impl ? m_impl->channels : 0;
}

void EQ::reset() {
    if (!isValid()) return;
    m_impl->last_error = jhq_reset(m_impl->handle);
}

void EQ::clearAll() {
    if (!isValid()) return;
    m_impl->last_error = jhq_clear_all(m_impl->handle);
}

void EQ::setBandType(int band, int type) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_band_type(m_impl->handle, band, type);
}

void EQ::setBandFreq(int band, double freq_hz) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_band_freq(m_impl->handle, band, freq_hz);
}

void EQ::setBandGain(int band, double gain_db) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_band_gain(m_impl->handle, band, gain_db);
}

void EQ::setBandQ(int band, double q) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_band_q(m_impl->handle, band, q);
}

void EQ::setBandEnable(int band, bool enable) {
    if (!isValid()) return;
    m_impl->last_error = jhq_set_band_enable(m_impl->handle, band, enable ? 1 : 0);
}

int EQ::getBandType(int band) const {
    if (!isValid()) return 0;
    int out = 0;
    jhq_get_band_type(m_impl->handle, band, &out);
    return out;
}

double EQ::getBandFreq(int band) const {
    if (!isValid()) return 0.0;
    double out = 0.0;
    jhq_get_band_freq(m_impl->handle, band, &out);
    return out;
}

double EQ::getBandGain(int band) const {
    if (!isValid()) return 0.0;
    double out = 0.0;
    jhq_get_band_gain(m_impl->handle, band, &out);
    return out;
}

double EQ::getBandQ(int band) const {
    if (!isValid()) return 0.0;
    double out = 0.0;
    jhq_get_band_q(m_impl->handle, band, &out);
    return out;
}

bool EQ::getBandEnable(int band) const {
    if (!isValid()) return false;
    int out = 0;
    jhq_get_band_enable(m_impl->handle, band, &out);
    return out != 0;
}

void EQ::setDynThreshold(int band, double threshold_db) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_threshold(m_impl->handle, band, threshold_db);
}

void EQ::setDynRatio(int band, double ratio) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_ratio(m_impl->handle, band, ratio);
}

void EQ::setDynAttack(int band, double attack_ms) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_attack(m_impl->handle, band, attack_ms);
}

void EQ::setDynRelease(int band, double release_ms) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_release(m_impl->handle, band, release_ms);
}

void EQ::setDynRange(int band, double range_db) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_range(m_impl->handle, band, range_db);
}

void EQ::setDynMode(int band, int mode) {
    if (!isValid()) return;
    m_impl->last_error = dtjhq_set_band_mode(m_impl->handle, band, mode);
}

void EQ::setSatType(int type) {
    if (!isValid()) return;
    m_impl->last_error = bh_set_type(m_impl->handle, type);
}

void EQ::setSatDrive(double drive_db) {
    if (!isValid()) return;
    m_impl->last_error = bh_set_drive(m_impl->handle, drive_db);
}

void EQ::setSatMix(double mix) {
    if (!isValid()) return;
    m_impl->last_error = bh_set_mix(m_impl->handle, mix);
}

void EQ::setSatOutput(double output_db) {
    if (!isValid()) return;
    m_impl->last_error = bh_set_output(m_impl->handle, output_db);
}

void EQ::setAgcTarget(double target_dbfs) {
    if (!isValid()) return;
    m_impl->last_error = zdzy_set_target(m_impl->handle, target_dbfs);
}

void EQ::setAgcAttack(double attack_ms) {
    if (!isValid()) return;
    m_impl->last_error = zdzy_set_attack(m_impl->handle, attack_ms);
}

void EQ::setAgcRelease(double release_ms) {
    if (!isValid()) return;
    m_impl->last_error = zdzy_set_release(m_impl->handle, release_ms);
}

void EQ::setAgcMaxBoost(double max_boost_db) {
    if (!isValid()) return;
    m_impl->last_error = zdzy_set_max_boost(m_impl->handle, max_boost_db);
}

void EQ::setAgcMaxCut(double max_cut_db) {
    if (!isValid()) return;
    m_impl->last_error = zdzy_set_max_cut(m_impl->handle, max_cut_db);
}

void EQ::setLinearKernelSize(int size) {
    if (!isValid()) return;
    m_impl->last_error = xxwjhq_set_kernel_size(m_impl->handle, size);
}

int EQ::getLinearKernelSize() const {
    if (!isValid()) return 0;
    int out = 0;
    xxwjhq_get_kernel_size(m_impl->handle, &out);
    return out;
}

int EQ::getLatency() const {
    if (!isValid()) return 0;
    int out = 0;
    xxwjhq_get_latency(m_impl->handle, &out);
    return out;
}

void EQ::setBandPhaseMode(int band, int mode) {
    if (!isValid()) return;
    m_impl->last_error = hhxw_set_band_phase_mode(m_impl->handle, band, mode);
}

void EQ::setMultiBandNumBands(int n) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_num_bands(m_impl->handle, n);
}

int EQ::getMultiBandNumBands() const {
    if (!isValid()) return 0;
    int out = 0;
    dsjhq_get_num_bands(m_impl->handle, &out);
    return out;
}

void EQ::autoDistributeBands(double low_hz, double high_hz) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_auto_distribute(m_impl->handle, low_hz, high_hz);
}

void EQ::setMultiBandRange(int band, double low_hz, double high_hz) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_range(m_impl->handle, band, low_hz, high_hz);
}

void EQ::setMultiBandCenter(int band, double center_hz) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_center(m_impl->handle, band, center_hz);
}

void EQ::setMultiBandGain(int band, double gain_db) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_gain(m_impl->handle, band, gain_db);
}

void EQ::setMultiBandQ(int band, double q) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_q(m_impl->handle, band, q);
}

void EQ::setMultiBandType(int band, int type) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_type(m_impl->handle, band, type);
}

void EQ::setMultiBandEnable(int band, bool enable) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_enable(m_impl->handle, band, enable ? 1 : 0);
}

void EQ::setMultiBandTargetDbfsAll(double target) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_target_dbfs_all(m_impl->handle, target);
}

void EQ::setMultiBandDynRangeAll(double range_db) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_dyn_range_all(m_impl->handle, range_db);
}

void EQ::setMultiBandDynAttackAll(double ms) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_dyn_attack_all(m_impl->handle, ms);
}

void EQ::setMultiBandDynReleaseAll(double ms) {
    if (!isValid()) return;
    m_impl->last_error = dsjhq_set_band_dyn_release_all(m_impl->handle, ms);
}

int EQ::loadMeasuredCurve(const double* freqs_hz, const double* levels_db,
                          int num_points, double reference_db) {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = dsjhq_load_measured_curve(m_impl->handle, freqs_hz, levels_db,
                                                    num_points, reference_db);
    return m_impl->last_error;
}

int EQ::loadGainCurve(const double* freqs_hz, const double* gains_db,
                      int num_points, int source) {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = dsjhq_load_gain_curve(m_impl->handle, freqs_hz, gains_db,
                                                num_points, source);
    return m_impl->last_error;
}

int EQ::loadBuiltinCurve() {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = dsjhq_load_builtin_curve(m_impl->handle);
    return m_impl->last_error;
}

int EQ::clearCurve() {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = dsjhq_clear_curve(m_impl->handle);
    return m_impl->last_error;
}

int EQ::getCurveInfo(int* num_points, int* source, int* is_measured,
                     double* reference_db) const {
    if (!isValid()) return eqlib_err_handle;
    return dsjhq_get_curve_info(m_impl->handle, num_points, source,
                                 is_measured, reference_db);
}

int EQ::getCurvePoint(int index, double* freq_hz, double* level_db, double* gain_db) const {
    if (!isValid()) return eqlib_err_handle;
    return dsjhq_get_curve_point(m_impl->handle, index, freq_hz, level_db, gain_db);
}

void EQ::setMidSideMode(int mode) {
    if (!isValid()) return;
    m_impl->last_error = zc_set_mode(m_impl->handle, mode);
}

void EQ::setMidGain(double db) {
    if (!isValid()) return;
    m_impl->last_error = zc_set_mid_gain(m_impl->handle, db);
}

void EQ::setSideGain(double db) {
    if (!isValid()) return;
    m_impl->last_error = zc_set_side_gain(m_impl->handle, db);
}

void EQ::setSidechainEnable(bool enable) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_enable(m_impl->handle, enable ? 1 : 0);
}

void EQ::setSidechainAttack(double ms) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_attack(m_impl->handle, ms);
}

void EQ::setSidechainRelease(double ms) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_release(m_impl->handle, ms);
}

void EQ::setSidechainAmount(double amount) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_amount(m_impl->handle, amount);
}

void EQ::setSidechainTarget(double db) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_target(m_impl->handle, db);
}

void EQ::setSidechainRange(double db) {
    if (!isValid()) return;
    m_impl->last_error = cl_set_range(m_impl->handle, db);
}

void EQ::setResampleChannels(int ch) {
    if (!isValid()) return;
    m_impl->last_error = cy_set_channels(m_impl->handle, ch);
}

void EQ::setResampleInputRate(double rate) {
    if (!isValid()) return;
    m_impl->last_error = cy_set_input_rate(m_impl->handle, rate);
}

void EQ::setResampleOutputRate(double rate) {
    if (!isValid()) return;
    m_impl->last_error = cy_set_output_rate(m_impl->handle, rate);
}

void EQ::setResampleQuality(int taps) {
    if (!isValid()) return;
    m_impl->last_error = cy_set_quality(m_impl->handle, taps);
}

int EQ::getResampleLatency() const {
    if (!isValid()) return 0;
    int out = 0;
    cy_get_latency(m_impl->handle, &out);
    return out;
}

void EQ::process(float* data, int frames) {
    if (!isValid() || !data || frames <= 0) return;
    m_impl->last_error = jhq_process(m_impl->handle, data, data, frames, m_impl->channels);
}

void EQ::processDouble(double* data, int frames) {
    if (!isValid() || !data || frames <= 0) return;
    m_impl->last_error = jhq_process_double(m_impl->handle, data, data, frames, m_impl->channels);
}

void EQ::processFull(float* data, int frames) {
    if (!isValid() || !data || frames <= 0) return;
    int err = jhq_process(m_impl->handle, data, data, frames, m_impl->channels);
    if (err == eqlib_ok) {
        err = dsjhq_process(m_impl->handle, data, data, frames, m_impl->channels);
    }
    if (err == eqlib_ok) {
        err = bh_process(m_impl->handle, data, data, frames, m_impl->channels);
    }
    if (err == eqlib_ok) {
        err = zdzy_process(m_impl->handle, data, data, frames, m_impl->channels);
    }
    m_impl->last_error = err;
}

void EQ::processFullDouble(double* data, int frames) {
    if (!isValid() || !data || frames <= 0) return;
    int err = jhq_process_double(m_impl->handle, data, data, frames, m_impl->channels);
    if (err == eqlib_ok) {
        err = dsjhq_process_double(m_impl->handle, data, data, frames, m_impl->channels);
    }
    if (err == eqlib_ok) {
        err = bh_process_double(m_impl->handle, data, data, frames, m_impl->channels);
    }
    if (err == eqlib_ok) {
        err = zdzy_process_double(m_impl->handle, data, data, frames, m_impl->channels);
    }
    m_impl->last_error = err;
}

int EQ::processResample(const float* in, int in_frames,
                        float* out, int out_capacity, int* out_frames) {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = cy_process(m_impl->handle, in, in_frames, m_impl->channels,
                                     out, out_capacity, out_frames);
    return m_impl->last_error;
}

int EQ::processResampleDouble(const double* in, int in_frames,
                              double* out, int out_capacity, int* out_frames) {
    if (!isValid()) return eqlib_err_handle;
    m_impl->last_error = cy_process_double(m_impl->handle, in, in_frames, m_impl->channels,
                                            out, out_capacity, out_frames);
    return m_impl->last_error;
}

void EQ::processFile(const char* input_path, const char* output_path, int output_format) {
    if (!isValid() || !input_path || !output_path) return;
    m_impl->last_error = ypdc_process_and_save(m_impl->handle, input_path,
                                                output_path, output_format);
}

const char* EQ::getVersion() const {
    static char buf[64];
    if (!isValid()) return "";
    jhq_get_version(buf, 64);
    return buf;
}

}
