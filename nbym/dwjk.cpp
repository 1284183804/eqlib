#include "eqlib/eqlib.h"
#include "nbty.h"
#include "ypdr.h"
#include "dr_mp3.h"
#include "dr_flac.h"
#include <new>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

namespace {

eqlib::AudioFileFormat detect_file_format(const char* path) {
    if (!path) return eqlib::AudioFileFormat::Unknown;
    std::FILE* f = std::fopen(path, "rb");
    if (!f) return eqlib::AudioFileFormat::Unknown;
    unsigned char header[12] = {0};
    std::size_t n = std::fread(header, 1, 12, f);
    std::fclose(f);
    if (n < 4) return eqlib::AudioFileFormat::Unknown;

    if (n >= 12 &&
        header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
        header[8] == 'W' && header[9] == 'A' && header[10] == 'V' && header[11] == 'E') {
        return eqlib::AudioFileFormat::Wav;
    }
    if (header[0] == 'f' && header[1] == 'L' && header[2] == 'a' && header[3] == 'C') {
        return eqlib::AudioFileFormat::Flac;
    }
    if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') {
        return eqlib::AudioFileFormat::Mp3;
    }
    if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) {
        return eqlib::AudioFileFormat::Mp3;
    }
    return eqlib::AudioFileFormat::Unknown;
}

}

extern "C" {

int jhq_create(eqlib_handle** out) {
    if (!out) return eqlib_err_param;
    *out = new (std::nothrow) eqlib_handle();
    if (!*out) return eqlib_err_memory;
    return eqlib_ok;
}

int jhq_destroy(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    delete h;
    return eqlib_ok;
}

int jhq_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->core.reset();
    h->multiband_eq.reset();
    h->sidechain.reset();
    h->mid_side.reset();
    h->spectrum.reset();
    h->auto_eq.reset();
    h->agc.reset();
    h->saturation.reset();
    h->multichannel.reset();
    h->linear_phase.reset();
    h->hybrid_phase.reset();
    h->resampler.reset();
    return eqlib_ok;
}

int jhq_clear_all(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    jhq_reset(h);
    h->wav_reader.close();
    h->mem_reader.close();
    h->mem_writer.clear();
    h->audio_buffer_f.clear();
    h->audio_buffer_d.clear();
    h->audio_buffer_d_valid = false;
    h->loaded_info = eqlib::AudioFileInfo{};
    return eqlib_ok;
}

int jhq_set_sample_rate(eqlib_handle* h, double sr) {
    if (!h) return eqlib_err_handle;
    if (sr < eqlib::SR_MIN || sr > eqlib::SR_MAX) return eqlib_err_param;
    h->sample_rate = sr;
    h->core.setSampleRate(sr);
    h->multiband_eq.setSampleRate(sr);
    h->sidechain.setSampleRate(sr);
    h->auto_eq.setSampleRate(sr);
    h->agc.setSampleRate(sr);
    h->linear_phase.setSampleRate(sr);
    h->hybrid_phase.setSampleRate(sr);
    h->spectrum.setSampleRate(sr);
    return eqlib_ok;
}

int jhq_set_channels(eqlib_handle* h, int ch) {
    if (!h) return eqlib_err_handle;
    if (ch < 1 || ch > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    h->channels = ch;
    h->core.setChannels(ch);
    h->multiband_eq.setChannels(ch);
    h->sidechain.setChannels(ch);
    h->agc.setChannels(ch);
    h->spectrum.setChannels(ch);
    h->multichannel.setNumChannels(ch);
    h->linear_phase.setChannels(ch);
    h->hybrid_phase.setChannels(ch);
    h->resampler.setChannels(ch);
    return eqlib_ok;
}

int jhq_set_num_bands(eqlib_handle* h, int n) {
    if (!h) return eqlib_err_handle;
    if (n < 1 || n > eqlib::MAX_BANDS) return eqlib_err_param;
    h->core.setNumBands(n);
    return eqlib_ok;
}

int jhq_get_num_bands(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->core.getNumBands();
    return eqlib_ok;
}

int jhq_set_band_type(eqlib_handle* h, int band, int type) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (type < 0 || type > 7) return eqlib_err_param;
    h->core.setBandType(band, static_cast<eqlib::FilterType>(type));
    return eqlib_ok;
}

int jhq_set_band_freq(eqlib_handle* h, int band, double freq_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (freq_hz < eqlib::FREQ_MIN_HZ || freq_hz > eqlib::FREQ_MAX_HZ) return eqlib_err_param;
    h->core.setBandFreq(band, freq_hz);
    return eqlib_ok;
}

int jhq_set_band_freqs(eqlib_handle* h, const double* freqs_hz, int n) {
    if (!h) return eqlib_err_handle;
    if (!freqs_hz) return eqlib_err_param;
    if (n < 1 || n > eqlib::MAX_BANDS) return eqlib_err_param;
    for (int i = 0; i < n; ++i) {
        if (freqs_hz[i] < eqlib::FREQ_MIN_HZ || freqs_hz[i] > eqlib::FREQ_MAX_HZ) {
            return eqlib_err_param;
        }
    }
    h->core.setNumBands(n);
    for (int i = 0; i < n; ++i) {
        h->core.setBandFreq(i, freqs_hz[i]);
    }
    return eqlib_ok;
}

int jhq_set_band_gain(eqlib_handle* h, int band, double gain_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (gain_db < eqlib::GAIN_MIN_DB || gain_db > eqlib::GAIN_MAX_DB) return eqlib_err_param;
    h->core.setBandGain(band, gain_db);
    return eqlib_ok;
}

int jhq_set_band_q(eqlib_handle* h, int band, double q) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (q < eqlib::Q_MIN || q > eqlib::Q_MAX) return eqlib_err_param;
    h->core.setBandQ(band, q);
    return eqlib_ok;
}

int jhq_set_band_enable(eqlib_handle* h, int band, int enable) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    h->core.setBandEnable(band, enable != 0);
    return eqlib_ok;
}

int jhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->core.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int jhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->core.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int jhq_get_band_gain(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    *out = h->core.getBandGain(band);
    return eqlib_ok;
}

int jhq_get_band_type(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    *out = static_cast<int>(h->core.getBandType(band));
    return eqlib_ok;
}

int jhq_get_band_freq(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    *out = h->core.getBandFreq(band);
    return eqlib_ok;
}

int jhq_get_band_q(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    *out = h->core.getBandQ(band);
    return eqlib_ok;
}

int jhq_get_band_enable(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    *out = h->core.getBandEnable(band) ? 1 : 0;
    return eqlib_ok;
}

int jhq_get_version(char* buf, int buf_size) {
    if (!buf || buf_size <= 0) return eqlib_err_param;
    const char* ver = "eqlib 1.0.1";
    int len = 0;
    while (ver[len] && len < buf_size - 1) { buf[len] = ver[len]; ++len; }
    buf[len] = '\0';
    return eqlib_ok;
}

int dsjhq_set_num_bands(eqlib_handle* h, int n) {
    if (!h) return eqlib_err_handle;
    if (n < 1 || n > eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setNumBands(n) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_get_num_bands(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multiband_eq.getNumBands();
    return eqlib_ok;
}

int dsjhq_auto_distribute(eqlib_handle* h, double low_hz, double high_hz) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.autoDistributeBands(low_hz, high_hz) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_range(eqlib_handle* h, int band, double low_hz, double high_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandRange(band, low_hz, high_hz) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_center(eqlib_handle* h, int band, double center_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandCenter(band, center_hz) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_q(eqlib_handle* h, int band, double q) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandQ(band, q) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_gain(eqlib_handle* h, int band, double gain_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandGain(band, gain_db) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_type(eqlib_handle* h, int band, int type) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandType(band, type) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_enable(eqlib_handle* h, int band, int enable) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandEnable(band, enable) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_target_dbfs(eqlib_handle* h, int band, double target) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandTargetDbfs(band, target) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_dyn_range(eqlib_handle* h, int band, double range_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandDynRange(band, range_db) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_dyn_attack(eqlib_handle* h, int band, double ms) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandDynAttack(band, ms) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_dyn_release(eqlib_handle* h, int band, double ms) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandDynRelease(band, ms) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_target_dbfs_all(eqlib_handle* h, double target) {
    if (!h) return eqlib_err_handle;
    h->multiband_eq.setBandTargetDbfsAll(target);
    return eqlib_ok;
}

int dsjhq_set_band_dyn_range_all(eqlib_handle* h, double range_db) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.setBandDynRangeAll(range_db) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_dyn_attack_all(eqlib_handle* h, double ms) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.setBandDynAttackAll(ms) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_set_band_dyn_release_all(eqlib_handle* h, double ms) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.setBandDynReleaseAll(ms) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dsjhq_load_measured_curve(eqlib_handle* h,
                              const double* freqs_hz,
                              const double* levels_db,
                              int num_points,
                              double reference_db) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.loadMeasuredCurve(freqs_hz, levels_db, num_points, reference_db) != 0) {
        return eqlib_err_param;
    }
    return eqlib_ok;
}

int dsjhq_load_gain_curve(eqlib_handle* h,
                          const double* freqs_hz,
                          const double* gains_db,
                          int num_points,
                          int source) {
    if (!h) return eqlib_err_handle;
    if (h->multiband_eq.loadGainCurve(freqs_hz, gains_db, num_points, source) != 0) {
        return eqlib_err_param;
    }
    return eqlib_ok;
}

int dsjhq_load_builtin_curve(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->multiband_eq.loadBuiltinCurve();
    return eqlib_ok;
}

int dsjhq_clear_curve(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->multiband_eq.clearCurve();
    return eqlib_ok;
}

int dsjhq_get_curve_info(eqlib_handle* h,
                         int* num_points,
                         int* source,
                         int* is_measured,
                         double* reference_db) {
    if (!h || !num_points || !source || !is_measured || !reference_db) return eqlib_err_param;
    const eqlib::CurveData& c = h->multiband_eq.getCurveRef();
    *num_points = c.num_points;
    *source = c.source;
    *is_measured = c.is_measured ? 1 : 0;
    *reference_db = c.reference_db;
    return eqlib_ok;
}

int dsjhq_get_curve_point(eqlib_handle* h,
                          int index,
                          double* freq_hz,
                          double* level_db,
                          double* gain_db) {
    if (!h || !freq_hz || !level_db || !gain_db) return eqlib_err_param;
    const eqlib::CurveData& c = h->multiband_eq.getCurveRef();
    if (index < 0 || index >= c.num_points) return eqlib_err_param;
    *freq_hz = c.freqs_hz[static_cast<std::size_t>(index)];
    *level_db = c.levels_db[static_cast<std::size_t>(index)];
    *gain_db = c.gains_db[static_cast<std::size_t>(index)];
    return eqlib_ok;
}

int dsjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->multiband_eq.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int dsjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->multiband_eq.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int dtjhq_set_band_threshold(eqlib_handle* h, int band, double threshold_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    return dsjhq_set_band_target_dbfs(h, band, threshold_db);
}

int dtjhq_set_band_ratio(eqlib_handle* h, int band, double ratio) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandRatio(band, ratio) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_get_band_ratio(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.getBandRatio(band, *out) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_set_band_percent(eqlib_handle* h, int band, double percent) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandPercent(band, percent) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_get_band_percent(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.getBandPercent(band, *out) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_set_band_attack(eqlib_handle* h, int band, double attack_ms) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (attack_ms <= 0.0) return eqlib_err_param;
    return dsjhq_set_band_dyn_attack(h, band, attack_ms);
}

int dtjhq_set_band_release(eqlib_handle* h, int band, double release_ms) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (release_ms <= 0.0) return eqlib_err_param;
    return dsjhq_set_band_dyn_release(h, band, release_ms);
}

int dtjhq_set_band_range(eqlib_handle* h, int band, double range_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (range_db < 0.0) return eqlib_err_param;
    return dsjhq_set_band_dyn_range(h, band, range_db);
}

int dtjhq_set_band_mode(eqlib_handle* h, int band, int mode) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.setBandMode(band, mode) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_get_band_mode(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (h->multiband_eq.getBandMode(band, *out) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int dtjhq_set_band_type(eqlib_handle* h, int band, int type) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (type < 0 || type > 7) return eqlib_err_param;
    return dsjhq_set_band_type(h, band, type);
}

int dtjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (freq_hz < eqlib::FREQ_MIN_HZ || freq_hz > eqlib::FREQ_MAX_HZ) return eqlib_err_param;
    return dsjhq_set_band_center(h, band, freq_hz);
}

int dtjhq_set_band_gain(eqlib_handle* h, int band, double gain_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (gain_db < eqlib::GAIN_MIN_DB || gain_db > eqlib::GAIN_MAX_DB) return eqlib_err_param;
    return dsjhq_set_band_gain(h, band, gain_db);
}

int dtjhq_set_band_q(eqlib_handle* h, int band, double q) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (q < eqlib::Q_MIN || q > eqlib::Q_MAX) return eqlib_err_param;
    return dsjhq_set_band_q(h, band, q);
}

int dtjhq_set_band_enable(eqlib_handle* h, int band, int enable) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    return dsjhq_set_band_enable(h, band, enable);
}

int dtjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    return dsjhq_process(h, in, out, frames, channels);
}

int dtjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    return dsjhq_process_double(h, in, out, frames, channels);
}

int dtjhq_process_sidechain(eqlib_handle* h, const float* in, const float* sc,
                            float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !sc || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->multiband_eq.processBlockWithSidechainFloat(in, sc, out, frames, channels);
    return eqlib_ok;
}

int dtjhq_process_sidechain_double(eqlib_handle* h, const double* in, const double* sc,
                                   double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !sc || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->multiband_eq.processBlockWithSidechain(in, sc, out, frames, channels);
    return eqlib_ok;
}

int dtjhq_get_band_gain(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.gain_db;
    return eqlib_ok;
}

int dtjhq_get_band_threshold(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.target_dbfs;
    return eqlib_ok;
}

int dtjhq_get_band_attack(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.dyn_attack_ms;
    return eqlib_ok;
}

int dtjhq_get_band_release(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.dyn_release_ms;
    return eqlib_ok;
}

int dtjhq_get_band_range(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.dyn_range_db;
    return eqlib_ok;
}

int dtjhq_get_band_type(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.type;
    return eqlib_ok;
}

int dtjhq_get_band_freq(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.center_hz;
    return eqlib_ok;
}

int dtjhq_get_band_q(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.q;
    return eqlib_ok;
}

int dtjhq_get_band_enable(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    eqlib::BandConfig cfg;
    if (h->multiband_eq.getBandConfig(band, cfg) != 0) return eqlib_err_param;
    *out = cfg.enable;
    return eqlib_ok;
}

int cl_set_enable(eqlib_handle* h, int enable) {
    if (!h) return eqlib_err_handle;
    h->sidechain.setEnable(enable != 0);
    return eqlib_ok;
}

int cl_set_attack(eqlib_handle* h, double attack_ms) {
    if (!h) return eqlib_err_handle;
    if (attack_ms <= 0.0) return eqlib_err_param;
    h->sidechain.setAttack(attack_ms);
    return eqlib_ok;
}

int cl_set_release(eqlib_handle* h, double release_ms) {
    if (!h) return eqlib_err_handle;
    if (release_ms <= 0.0) return eqlib_err_param;
    h->sidechain.setRelease(release_ms);
    return eqlib_ok;
}

int cl_set_amount(eqlib_handle* h, double amount) {
    if (!h) return eqlib_err_handle;
    h->sidechain.setAmount(amount);
    return eqlib_ok;
}

int cl_set_target(eqlib_handle* h, double target_db) {
    if (!h) return eqlib_err_handle;
    h->sidechain.setTarget(target_db);
    return eqlib_ok;
}

int cl_set_range(eqlib_handle* h, double range_db) {
    if (!h) return eqlib_err_handle;
    if (range_db < 0.0) return eqlib_err_param;
    h->sidechain.setRange(range_db);
    return eqlib_ok;
}

int cl_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->sidechain.reset();
    return eqlib_ok;
}

int cl_process(eqlib_handle* h, const float* sc, float* gain_out, int frames, int channels) {
    if (!h || !sc || !gain_out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            gain_out[f * channels + c] = static_cast<float>(
                h->sidechain.process(static_cast<double>(sc[f * channels + c]), c));
        }
    }
    return eqlib_ok;
}

int cl_process_double(eqlib_handle* h, const double* sc, double* gain_out, int frames, int channels) {
    if (!h || !sc || !gain_out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            gain_out[f * channels + c] = h->sidechain.process(sc[f * channels + c], c);
        }
    }
    return eqlib_ok;
}

int cl_get_enable(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getEnable() > 0.5 ? 1 : 0;
    return eqlib_ok;
}

int cl_get_attack(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getAttack();
    return eqlib_ok;
}

int cl_get_release(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getRelease();
    return eqlib_ok;
}

int cl_get_amount(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getAmount();
    return eqlib_ok;
}

int cl_get_target(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getTarget();
    return eqlib_ok;
}

int cl_get_range(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->sidechain.getRange();
    return eqlib_ok;
}

int zc_set_mode(eqlib_handle* h, int mode) {
    if (!h) return eqlib_err_handle;
    if (mode < 0 || mode > 3) return eqlib_err_param;
    h->mid_side.setMode(static_cast<eqlib::MidSideMode>(mode));
    return eqlib_ok;
}

int zc_set_mid_gain(eqlib_handle* h, double db) {
    if (!h) return eqlib_err_handle;
    h->mid_side.setMidGain(db);
    return eqlib_ok;
}

int zc_set_side_gain(eqlib_handle* h, double db) {
    if (!h) return eqlib_err_handle;
    h->mid_side.setSideGain(db);
    return eqlib_ok;
}

int zc_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->mid_side.reset();
    return eqlib_ok;
}

int zc_process(eqlib_handle* h, const float* in_l, const float* in_r,
               float* out_l, float* out_r, int frames) {
    if (!h || !in_l || !in_r || !out_l || !out_r) return eqlib_err_param;
    if (frames <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        double ol = 0.0, or_ = 0.0;
        h->mid_side.process(static_cast<double>(in_l[f]), static_cast<double>(in_r[f]), ol, or_);
        out_l[f] = static_cast<float>(ol);
        out_r[f] = static_cast<float>(or_);
    }
    return eqlib_ok;
}

int zc_process_double(eqlib_handle* h, const double* in_l, const double* in_r,
                      double* out_l, double* out_r, int frames) {
    if (!h || !in_l || !in_r || !out_l || !out_r) return eqlib_err_param;
    if (frames <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        h->mid_side.process(in_l[f], in_r[f], out_l[f], out_r[f]);
    }
    return eqlib_ok;
}

int zc_get_mode(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = static_cast<int>(h->mid_side.getMode());
    return eqlib_ok;
}

int zc_get_mid_gain(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->mid_side.getMidGain();
    return eqlib_ok;
}

int zc_get_side_gain(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->mid_side.getSideGain();
    return eqlib_ok;
}

int ppfx_init(eqlib_handle* h, int fft_size, int window_type, double overlap) {
    if (!h) return eqlib_err_handle;
    if (fft_size <= 0 || fft_size > eqlib::FFT_MAX_SIZE) return eqlib_err_param;
    if (window_type < 0 || window_type > 4) return eqlib_err_param;
    if (overlap < 0.0 || overlap >= 1.0) return eqlib_err_param;
    eqlib::SpectrumConfig cfg;
    cfg.fft_size = fft_size;
    cfg.window = static_cast<eqlib::WindowType>(window_type);
    cfg.overlap = overlap;
    if (!h->spectrum.init(h->sample_rate, h->channels, cfg)) return eqlib_err_param;
    return eqlib_ok;
}

int ppfx_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->spectrum.reset();
    return eqlib_ok;
}

int ppfx_set_smoothing(eqlib_handle* h, double smoothing) {
    if (!h) return eqlib_err_handle;
    h->spectrum.setSmoothing(smoothing);
    return eqlib_ok;
}

int ppfx_process(eqlib_handle* h, const float* in, int frames, int channels) {
    if (!h || !in) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            h->spectrum.processSample(static_cast<double>(in[f * channels + c]), c);
        }
    }
    return eqlib_ok;
}

int ppfx_process_double(eqlib_handle* h, const double* in, int frames, int channels) {
    if (!h || !in) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            h->spectrum.processSample(in[f * channels + c], c);
        }
    }
    return eqlib_ok;
}

int ppfx_get_spectrum(eqlib_handle* h, float* mag_out, float* freq_out, int* num_bins) {
    if (!h || !mag_out || !freq_out || !num_bins) return eqlib_err_param;
    eqlib::SpectrumFrame frame;
    if (!h->spectrum.getFrame(frame)) return eqlib_err_state;
    *num_bins = frame.num_bins;
    for (int i = 0; i < frame.num_bins; ++i) {
        mag_out[i] = static_cast<float>(frame.magnitudes[static_cast<std::size_t>(i)]);
        freq_out[i] = static_cast<float>(frame.freqs_hz[static_cast<std::size_t>(i)]);
    }
    return eqlib_ok;
}

int ppfx_get_spectrum_double(eqlib_handle* h, double* mag_out, double* freq_out, int* num_bins) {
    if (!h || !mag_out || !freq_out || !num_bins) return eqlib_err_param;
    eqlib::SpectrumFrame frame;
    if (!h->spectrum.getFrame(frame)) return eqlib_err_state;
    *num_bins = frame.num_bins;
    for (int i = 0; i < frame.num_bins; ++i) {
        mag_out[i] = frame.magnitudes[static_cast<std::size_t>(i)];
        freq_out[i] = frame.freqs_hz[static_cast<std::size_t>(i)];
    }
    return eqlib_ok;
}

int ppfx_get_fft_size(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->spectrum.getFftSize();
    return eqlib_ok;
}

int ppfx_get_hop_size(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->spectrum.getHopSize();
    return eqlib_ok;
}

int zdjhq_set_sample_rate(eqlib_handle* h, double sr) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.setSampleRate(sr);
    h->spectrum.setSampleRate(sr);
    return eqlib_ok;
}

int zdjhq_set_fft_size(eqlib_handle* h, int size) {
    if (!h) return eqlib_err_handle;
    if (size <= 0 || size > eqlib::FFT_MAX_SIZE) return eqlib_err_param;
    h->auto_eq.setFftSize(size);
    h->spectrum.setFftSize(size);
    return eqlib_ok;
}

int zdjhq_set_window(eqlib_handle* h, int window_type) {
    if (!h) return eqlib_err_handle;
    if (window_type < 0 || window_type > 4) return eqlib_err_param;
    h->auto_eq.setWindow(static_cast<eqlib::WindowType>(window_type));
    h->spectrum.setWindow(static_cast<eqlib::WindowType>(window_type));
    return eqlib_ok;
}

int zdjhq_set_pct(eqlib_handle* h, double pct) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.setPct(pct);
    return eqlib_ok;
}

int zdjhq_set_num_bands(eqlib_handle* h, int n) {
    if (!h) return eqlib_err_handle;
    if (n < 1 || n > eqlib::MAX_BANDS) return eqlib_err_param;
    if (h->auto_eq.setNumBands(n) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_get_num_bands(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.getNumBands();
    return eqlib_ok;
}

int zdjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz) {
    if (!h) return eqlib_err_handle;
    if (h->auto_eq.setBandFreq(band, freq_hz) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_set_band_freqs(eqlib_handle* h, const double* freqs_hz, int n) {
    if (!h) return eqlib_err_handle;
    if (h->auto_eq.setBandFreqs(freqs_hz, n) != 0) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_get_band_freq(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (!h->auto_eq.getBandFreq(band, *out)) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_set_reference_curve(eqlib_handle* h, const double* freqs_hz,
                              const double* gains_db, int num_points) {
    if (!h) return eqlib_err_handle;
    if (!h->auto_eq.setReferenceCurve(freqs_hz, gains_db, num_points)) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_clear_reference_curve(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.clearReferenceCurve();
    return eqlib_ok;
}

int zdjhq_has_reference_curve(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.hasReferenceCurve() ? 1 : 0;
    return eqlib_ok;
}

int zdjhq_get_reference_point_count(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.getReferencePointCount();
    return eqlib_ok;
}

int zdjhq_get_reference_point(eqlib_handle* h, int index, double* freq_hz, double* gain_db) {
    if (!h || !freq_hz || !gain_db) return eqlib_err_param;
    if (!h->auto_eq.getReferencePoint(index, *freq_hz, *gain_db)) return eqlib_err_param;
    return eqlib_ok;
}

int zdjhq_start(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.start();
    return eqlib_ok;
}

int zdjhq_stop(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.stop();
    return eqlib_ok;
}

int zdjhq_is_running(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.isRunning() ? 1 : 0;
    return eqlib_ok;
}

int zdjhq_has_result(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.hasResult() ? 1 : 0;
    return eqlib_ok;
}

int zdjhq_feed_spectrum(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    eqlib::SpectrumFrame frame;
    if (!h->spectrum.getFrame(frame)) return eqlib_err_state;
    h->auto_eq.feedSpectrum(frame);
    return eqlib_ok;
}

int zdjhq_compute_result(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    if (!h->auto_eq.computeResult()) return eqlib_err_state;
    return eqlib_ok;
}

int zdjhq_get_result(eqlib_handle* h, double* gains_db, int* num_bands) {
    if (!h || !gains_db || !num_bands) return eqlib_err_param;
    eqlib::AutoEqOutput out = h->auto_eq.getResult();
    int n = h->auto_eq.getNumBands();
    if (n < 0) n = 0;
    if (n > eqlib::MAX_BANDS) n = eqlib::MAX_BANDS;
    *num_bands = n;
    for (int i = 0; i < n; ++i) {
        gains_db[i] = out.gains_db[i];
    }
    return eqlib_ok;
}

int zdjhq_get_rms_error(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.getResult().rms_error_db;
    return eqlib_ok;
}

int zdjhq_get_max_error(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->auto_eq.getResult().max_error_db;
    return eqlib_ok;
}

int zdjhq_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->auto_eq.reset();
    return eqlib_ok;
}

int zdzy_set_sample_rate(eqlib_handle* h, double sr) {
    if (!h) return eqlib_err_handle;
    h->agc.setSampleRate(sr);
    return eqlib_ok;
}

int zdzy_set_target(eqlib_handle* h, double target_dbfs) {
    if (!h) return eqlib_err_handle;
    h->agc.setTarget(target_dbfs);
    return eqlib_ok;
}

int zdzy_set_attack(eqlib_handle* h, double attack_ms) {
    if (!h) return eqlib_err_handle;
    if (attack_ms <= 0.0) return eqlib_err_param;
    h->agc.setAttack(attack_ms);
    return eqlib_ok;
}

int zdzy_set_release(eqlib_handle* h, double release_ms) {
    if (!h) return eqlib_err_handle;
    if (release_ms <= 0.0) return eqlib_err_param;
    h->agc.setRelease(release_ms);
    return eqlib_ok;
}

int zdzy_set_max_boost(eqlib_handle* h, double max_boost_db) {
    if (!h) return eqlib_err_handle;
    h->agc.setMaxBoost(max_boost_db);
    return eqlib_ok;
}

int zdzy_set_max_cut(eqlib_handle* h, double max_cut_db) {
    if (!h) return eqlib_err_handle;
    h->agc.setMaxCut(max_cut_db);
    return eqlib_ok;
}

int zdzy_set_gate_enable(eqlib_handle* h, int enable) {
    if (!h) return eqlib_err_handle;
    h->agc.setGateEnable(enable != 0);
    return eqlib_ok;
}

int zdzy_set_gate_threshold(eqlib_handle* h, double threshold_dbfs) {
    if (!h) return eqlib_err_handle;
    h->agc.setGateThreshold(threshold_dbfs);
    return eqlib_ok;
}

int zdzy_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->agc.reset();
    return eqlib_ok;
}

int zdzy_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->agc.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int zdzy_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->agc.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int zdzy_get_current_gain_db(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->agc.getCurrentGainDb();
    return eqlib_ok;
}

int zdzy_get_envelope_db(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->agc.getEnvelopeDb();
    return eqlib_ok;
}

int bh_set_type(eqlib_handle* h, int type) {
    if (!h) return eqlib_err_handle;
    if (type < 0 || type > 3) return eqlib_err_param;
    h->saturation.setType(static_cast<eqlib::SaturationType>(type));
    return eqlib_ok;
}

int bh_set_drive(eqlib_handle* h, double drive_db) {
    if (!h) return eqlib_err_handle;
    h->saturation.setDrive(drive_db);
    return eqlib_ok;
}

int bh_set_mix(eqlib_handle* h, double mix) {
    if (!h) return eqlib_err_handle;
    if (mix < 0.0 || mix > 1.0) return eqlib_err_param;
    h->saturation.setMix(mix);
    return eqlib_ok;
}

int bh_set_output(eqlib_handle* h, double output_db) {
    if (!h) return eqlib_err_handle;
    h->saturation.setOutput(output_db);
    return eqlib_ok;
}

int bh_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->saturation.reset();
    return eqlib_ok;
}

int bh_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->saturation.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int bh_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->saturation.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int bh_get_type(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = static_cast<int>(h->saturation.getType());
    return eqlib_ok;
}

int bh_get_drive(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->saturation.getDrive();
    return eqlib_ok;
}

int bh_get_mix(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->saturation.getMix();
    return eqlib_ok;
}

int bh_get_output(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->saturation.getOutput();
    return eqlib_ok;
}

int dtd_set_num_channels(eqlib_handle* h, int ch) {
    if (!h) return eqlib_err_handle;
    if (ch < 1 || ch > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    h->multichannel.setNumChannels(ch);
    return eqlib_ok;
}

int dtd_set_block_size(eqlib_handle* h, int size) {
    if (!h) return eqlib_err_handle;
    if (size < 1) return eqlib_err_param;
    h->multichannel.setBlockSize(size);
    return eqlib_ok;
}

int dtd_set_num_threads(eqlib_handle* h, int n) {
    if (!h) return eqlib_err_handle;
    if (n < 0) return eqlib_err_param;
    if (n > 64) return eqlib_err_param;
    h->multichannel.setNumThreads(n);
    return eqlib_ok;
}

int dtd_set_enable_multithread(eqlib_handle* h, int enable) {
    if (!h) return eqlib_err_handle;
    h->multichannel.setEnableMultithread(enable != 0);
    return eqlib_ok;
}

int dtd_get_num_channels(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getNumChannels();
    return eqlib_ok;
}

int dtd_get_block_size(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getBlockSize();
    return eqlib_ok;
}

int dtd_get_num_threads(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getNumThreads();
    return eqlib_ok;
}

int dtd_get_enable_multithread(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getEnableMultithread() ? 1 : 0;
    return eqlib_ok;
}

int dtd_get_effective_threads(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getEffectiveThreads();
    return eqlib_ok;
}

int dtd_get_hardware_threads(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->multichannel.getHardwareThreads();
    return eqlib_ok;
}

int dtd_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->multichannel.reset();
    return eqlib_ok;
}

int wjdx_wav_open_write(eqlib_handle* h, const char* path, int sample_rate,
                        int channels, int format) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;
    if (sample_rate < 8000 || sample_rate > 384000) return eqlib_err_param;
    if (channels < 1 || channels > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    if (format < 0 || format > 2) return eqlib_err_param;
    eqlib::WavHeaderInfo info{};
    info.sample_rate = sample_rate;
    info.channels = channels;
    info.format = static_cast<eqlib::WavFormat>(format);
    info.num_frames = 0;
    if (!h->wav_writer.open(path, info)) return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_write_float(eqlib_handle* h, const float* data, int frames) {
    if (!h) return eqlib_err_handle;
    if (!data || frames <= 0) return eqlib_err_param;
    if (!h->wav_writer.isOpen()) return eqlib_err_state;
    if (!h->wav_writer.writeFloat32(data, frames, h->wav_writer.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_write_double(eqlib_handle* h, const double* data, int frames) {
    if (!h) return eqlib_err_handle;
    if (!data || frames <= 0) return eqlib_err_param;
    if (!h->wav_writer.isOpen()) return eqlib_err_state;
    if (!h->wav_writer.writeDouble(data, frames, h->wav_writer.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_finalize(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    if (!h->wav_writer.isOpen()) return eqlib_err_state;
    if (!h->wav_writer.finalize()) return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_close(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->wav_writer.close();
    return eqlib_ok;
}

int wjdx_wav_get_frames_written(eqlib_handle* h, uint64_t* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->wav_writer.getFramesWritten();
    return eqlib_ok;
}

int wjdx_wav_open_read(eqlib_handle* h, const char* path) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;
    if (!h->wav_reader.open(path)) return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_get_info(eqlib_handle* h, int* sample_rate, int* channels,
                      int* bits, int* format, uint64_t* num_frames) {
    if (!h || !sample_rate || !channels || !bits || !format || !num_frames)
        return eqlib_err_param;
    if (!h->wav_reader.isOpen()) return eqlib_err_state;
    eqlib::WavHeaderInfo info = h->wav_reader.getInfo();
    *sample_rate = info.sample_rate;
    *channels = info.channels;
    *bits = info.bits_per_sample;
    *format = static_cast<int>(info.format);
    *num_frames = info.num_frames;
    return eqlib_ok;
}

int wjdx_wav_read_float(eqlib_handle* h, float* out, int frames) {
    if (!h) return eqlib_err_handle;
    if (!out || frames <= 0) return eqlib_err_param;
    if (!h->wav_reader.isOpen()) return eqlib_err_state;
    if (!h->wav_reader.readFloat32(out, frames, h->wav_reader.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_read_double(eqlib_handle* h, double* out, int frames) {
    if (!h) return eqlib_err_handle;
    if (!out || frames <= 0) return eqlib_err_param;
    if (!h->wav_reader.isOpen()) return eqlib_err_state;
    if (!h->wav_reader.readDouble(out, frames, h->wav_reader.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int wjdx_wav_read_close(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->wav_reader.close();
    return eqlib_ok;
}

int wjdx_wav_get_frames_read(eqlib_handle* h, uint64_t* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->wav_reader.getFramesRead();
    return eqlib_ok;
}

int nccwj_open_write(eqlib_handle* h, int sample_rate, int channels, int format) {
    if (!h) return eqlib_err_handle;
    if (sample_rate < 8000 || sample_rate > 384000) return eqlib_err_param;
    if (channels < 1 || channels > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    if (format < 0 || format > 2) return eqlib_err_param;
    eqlib::WavHeaderInfo info{};
    info.sample_rate = sample_rate;
    info.channels = channels;
    info.format = static_cast<eqlib::WavFormat>(format);
    info.num_frames = 0;
    if (!h->mem_writer.open(info)) return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_write_float(eqlib_handle* h, const float* data, int frames) {
    if (!h) return eqlib_err_handle;
    if (!data || frames <= 0) return eqlib_err_param;
    if (!h->mem_writer.isOpen()) return eqlib_err_state;
    if (!h->mem_writer.writeFloat32(data, frames, h->mem_writer.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_write_double(eqlib_handle* h, const double* data, int frames) {
    if (!h) return eqlib_err_handle;
    if (!data || frames <= 0) return eqlib_err_param;
    if (!h->mem_writer.isOpen()) return eqlib_err_state;
    if (!h->mem_writer.writeDouble(data, frames, h->mem_writer.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_finalize(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    if (!h->mem_writer.isOpen()) return eqlib_err_state;
    if (!h->mem_writer.finalize()) return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_get_data(eqlib_handle* h, const uint8_t** out_data, uint64_t* out_size) {
    if (!h || !out_data || !out_size) return eqlib_err_param;
    if (!h->mem_writer.isOpen()) return eqlib_err_state;
    *out_data = h->mem_writer.getData();
    *out_size = static_cast<uint64_t>(h->mem_writer.getSize());
    return eqlib_ok;
}

int nccwj_get_frames_written(eqlib_handle* h, uint64_t* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->mem_writer.getFramesWritten();
    return eqlib_ok;
}

int nccwj_clear(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->mem_writer.clear();
    return eqlib_ok;
}

int nccwj_open_read(eqlib_handle* h, const uint8_t* data, uint64_t size) {
    if (!h) return eqlib_err_handle;
    if (!data || size == 0) return eqlib_err_param;
    if (!h->mem_reader.open(data, static_cast<std::size_t>(size))) return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_get_info(eqlib_handle* h, int* sample_rate, int* channels,
                   int* bits, int* format) {
    if (!h || !sample_rate || !channels || !bits || !format) return eqlib_err_param;
    if (!h->mem_reader.isOpen()) return eqlib_err_state;
    eqlib::WavHeaderInfo info = h->mem_reader.getInfo();
    *sample_rate = info.sample_rate;
    *channels = info.channels;
    *bits = info.bits_per_sample;
    *format = static_cast<int>(info.format);
    return eqlib_ok;
}

int nccwj_read_float(eqlib_handle* h, float* out, int frames) {
    if (!h) return eqlib_err_handle;
    if (!out || frames <= 0) return eqlib_err_param;
    if (!h->mem_reader.isOpen()) return eqlib_err_state;
    if (!h->mem_reader.readFloat32(out, frames, h->mem_reader.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_read_double(eqlib_handle* h, double* out, int frames) {
    if (!h) return eqlib_err_handle;
    if (!out || frames <= 0) return eqlib_err_param;
    if (!h->mem_reader.isOpen()) return eqlib_err_state;
    if (!h->mem_reader.readDouble(out, frames, h->mem_reader.getInfo().channels))
        return eqlib_err_io;
    return eqlib_ok;
}

int nccwj_get_frames_read(eqlib_handle* h, uint64_t* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->mem_reader.getFramesRead();
    return eqlib_ok;
}

int nccwj_close_read(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->mem_reader.close();
    return eqlib_ok;
}

int ypdr_load_wav(eqlib_handle* h, const char* path) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;
    h->audio_buffer_f.clear();
    h->audio_buffer_d.clear();
    h->audio_buffer_d_valid = false;
    h->loaded_info = eqlib::AudioFileInfo{};

    eqlib::WavReader reader;
    if (!reader.open(path)) return eqlib_err_io;
    eqlib::WavHeaderInfo info = reader.getInfo();

    uint64_t total_frames = info.num_frames;
    if (total_frames == 0 || info.channels <= 0) {
        reader.close();
        return eqlib_err_io;
    }

    h->audio_buffer_f.resize(total_frames * static_cast<uint64_t>(info.channels));

    uint64_t read_total = 0;
    while (read_total < total_frames) {
        uint64_t chunk = total_frames - read_total;
        if (chunk > 4096) chunk = 4096;
        if (!reader.readFloat32(h->audio_buffer_f.data() + read_total * info.channels,
                                static_cast<int>(chunk), info.channels)) break;
        read_total += chunk;
    }
    reader.close();

    h->audio_buffer_f.resize(read_total * static_cast<uint64_t>(info.channels));

    h->loaded_info.sample_rate = info.sample_rate;
    h->loaded_info.channels = info.channels;
    h->loaded_info.bits_per_sample = info.bits_per_sample;
    h->loaded_info.num_frames = read_total;
    h->loaded_info.format = eqlib::AudioFileFormat::Wav;
    return eqlib_ok;
}

int ypdr_load_mp3(eqlib_handle* h, const char* path) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;
    h->audio_buffer_f.clear();
    h->audio_buffer_d.clear();
    h->audio_buffer_d_valid = false;
    h->loaded_info = eqlib::AudioFileInfo{};

    drmp3 mp3;
    if (!drmp3_init_file(&mp3, path, nullptr)) return eqlib_err_io;

    int sample_rate = static_cast<int>(mp3.sampleRate);
    int channels = static_cast<int>(mp3.channels);
    uint64_t total_frames = drmp3_get_pcm_frame_count(&mp3);
    if (total_frames == 0) {
        drmp3_uninit(&mp3);
        return eqlib_err_io;
    }

    h->audio_buffer_f.resize(total_frames * static_cast<uint64_t>(channels));

    uint64_t read_total = 0;
    while (read_total < total_frames) {
        uint64_t chunk = total_frames - read_total;
        if (chunk > 4096) chunk = 4096;
        uint64_t got = drmp3_read_pcm_frames_f32(&mp3, chunk,
                                                h->audio_buffer_f.data() + read_total * channels);
        if (got == 0) break;
        read_total += got;
    }
    drmp3_uninit(&mp3);

    h->audio_buffer_f.resize(read_total * static_cast<uint64_t>(channels));

    h->loaded_info.sample_rate = sample_rate;
    h->loaded_info.channels = channels;
    h->loaded_info.bits_per_sample = 16;
    h->loaded_info.num_frames = read_total;
    h->loaded_info.format = eqlib::AudioFileFormat::Mp3;
    return eqlib_ok;
}

int ypdr_load_flac(eqlib_handle* h, const char* path) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;
    h->audio_buffer_f.clear();
    h->audio_buffer_d.clear();
    h->audio_buffer_d_valid = false;
    h->loaded_info = eqlib::AudioFileInfo{};

    drflac* flac = drflac_open_file(path, nullptr);
    if (!flac) return eqlib_err_io;

    int sample_rate = static_cast<int>(flac->sampleRate);
    int channels = static_cast<int>(flac->channels);
    int bits = static_cast<int>(flac->bitsPerSample);
    uint64_t total_frames = flac->totalPCMFrameCount;
    if (total_frames == 0) {
        drflac_close(flac);
        return eqlib_err_io;
    }

    h->audio_buffer_f.resize(total_frames * static_cast<uint64_t>(channels));

    uint64_t read_total = 0;
    while (read_total < total_frames) {
        uint64_t chunk = total_frames - read_total;
        if (chunk > 4096) chunk = 4096;
        uint64_t got = drflac_read_pcm_frames_f32(flac, chunk,
                                                 h->audio_buffer_f.data() + read_total * channels);
        if (got == 0) break;
        read_total += got;
    }
    drflac_close(flac);

    h->audio_buffer_f.resize(read_total * static_cast<uint64_t>(channels));

    h->loaded_info.sample_rate = sample_rate;
    h->loaded_info.channels = channels;
    h->loaded_info.bits_per_sample = bits;
    h->loaded_info.num_frames = read_total;
    h->loaded_info.format = eqlib::AudioFileFormat::Flac;
    return eqlib_ok;
}

int ypdr_load(eqlib_handle* h, const char* path, int format) {
    if (!h) return eqlib_err_handle;
    if (!path) return eqlib_err_param;

    if (format == eqlib_file_unknown) {
        eqlib::AudioFileFormat detected = detect_file_format(path);
        if (detected == eqlib::AudioFileFormat::Wav)  return ypdr_load_wav(h, path);
        if (detected == eqlib::AudioFileFormat::Mp3)  return ypdr_load_mp3(h, path);
        if (detected == eqlib::AudioFileFormat::Flac) return ypdr_load_flac(h, path);
        return eqlib_err_param;
    }
    if (format == eqlib_file_wav)  return ypdr_load_wav(h, path);
    if (format == eqlib_file_mp3)  return ypdr_load_mp3(h, path);
    if (format == eqlib_file_flac) return ypdr_load_flac(h, path);
    return eqlib_err_param;
}

int ypdr_get_info(eqlib_handle* h, int* sample_rate, int* channels,
                  uint64_t* num_frames, int* format) {
    if (!h || !sample_rate || !channels || !num_frames || !format) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    *sample_rate = h->loaded_info.sample_rate;
    *channels = h->loaded_info.channels;
    *num_frames = h->loaded_info.num_frames;
    *format = static_cast<int>(h->loaded_info.format);
    return eqlib_ok;
}

int ypdr_get_float(eqlib_handle* h, float* out, uint64_t frames, int channels) {
    if (!h || !out) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    if (channels != h->loaded_info.channels) return eqlib_err_param;
    if (frames > h->loaded_info.num_frames) return eqlib_err_param;
    uint64_t n = frames * static_cast<uint64_t>(channels);
    for (uint64_t i = 0; i < n; ++i) out[i] = h->audio_buffer_f[i];
    return eqlib_ok;
}

int ypdr_get_double(eqlib_handle* h, double* out, uint64_t frames, int channels) {
    if (!h || !out) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    if (channels != h->loaded_info.channels) return eqlib_err_param;
    if (frames > h->loaded_info.num_frames) return eqlib_err_param;

    if (!h->audio_buffer_d_valid) {
        std::size_t total = static_cast<std::size_t>(h->loaded_info.num_frames)
                          * static_cast<std::size_t>(channels);
        h->audio_buffer_d.resize(total);
        for (std::size_t i = 0; i < total; ++i) {
            h->audio_buffer_d[i] = static_cast<double>(h->audio_buffer_f[i]);
        }
        h->audio_buffer_d_valid = true;
    }

    uint64_t n = frames * static_cast<uint64_t>(channels);
    for (uint64_t i = 0; i < n; ++i) out[i] = h->audio_buffer_d[i];
    return eqlib_ok;
}

int ypdr_get_num_frames(eqlib_handle* h, uint64_t* out) {
    if (!h || !out) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    *out = h->loaded_info.num_frames;
    return eqlib_ok;
}

int ypdr_get_channels(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    *out = h->loaded_info.channels;
    return eqlib_ok;
}

int ypdr_get_sample_rate(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (h->loaded_info.format == eqlib::AudioFileFormat::Unknown) return eqlib_err_state;
    *out = h->loaded_info.sample_rate;
    return eqlib_ok;
}

int ypdr_clear(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->audio_buffer_f.clear();
    h->audio_buffer_d.clear();
    h->audio_buffer_d_valid = false;
    h->loaded_info = eqlib::AudioFileInfo{};
    return eqlib_ok;
}

int ypdc_process_and_save(eqlib_handle* h, const char* input_path,
                          const char* output_path, int output_format) {
    if (!h || !input_path || !output_path) return eqlib_err_param;
    if (output_format < 0 || output_format > 2) return eqlib_err_param;
    if (ypdr_load(h, input_path, eqlib_file_unknown) != eqlib_ok) {
        return eqlib_err_io;
    }
    uint64_t frames = h->loaded_info.num_frames;
    int channels = h->loaded_info.channels;
    int sample_rate = h->loaded_info.sample_rate;
    if (frames == 0 || channels <= 0) return eqlib_err_state;

    eqlib::WavHeaderInfo info{};
    info.sample_rate = sample_rate;
    info.channels = channels;
    info.format = static_cast<eqlib::WavFormat>(output_format);
    if (!h->wav_writer.open(output_path, info)) return eqlib_err_io;

    uint64_t processed = 0;
    while (processed < frames) {
        uint64_t chunk = frames - processed;
        if (chunk > 4096) chunk = 4096;
        float* ptr = h->audio_buffer_f.data() + processed * channels;
        h->core.processBlockFloat(ptr, ptr, static_cast<int>(chunk), channels);
        h->multiband_eq.processBlockFloat(ptr, ptr, static_cast<int>(chunk), channels);
        h->saturation.processBlockFloat(ptr, ptr, static_cast<int>(chunk), channels);
        h->agc.processBlockFloat(ptr, ptr, static_cast<int>(chunk), channels);
        h->wav_writer.writeFloat32(ptr, static_cast<int>(chunk), channels);
        processed += chunk;
    }
    h->wav_writer.finalize();
    h->wav_writer.close();
    return eqlib_ok;
}

int ypdc_process_and_save_float(eqlib_handle* h, const char* input_path,
                                const char* output_path, int output_format) {
    return ypdc_process_and_save(h, input_path, output_path, output_format);
}

int ypdc_process_and_save_double(eqlib_handle* h, const char* input_path,
                                 const char* output_path, int output_format) {
    return ypdc_process_and_save(h, input_path, output_path, output_format);
}

int xxwjhq_set_kernel_size(eqlib_handle* h, int size) {
    if (!h) return eqlib_err_handle;
    if (size < 32 || size > eqlib::LinearPhaseEq::MAX_KERNEL) return eqlib_err_param;
    h->linear_phase.setKernelSize(size);
    return eqlib_ok;
}

int xxwjhq_get_kernel_size(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->linear_phase.getKernelSize();
    return eqlib_ok;
}

int xxwjhq_set_band_type(eqlib_handle* h, int band, int type) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (type < 0 || type > 7) return eqlib_err_param;
    h->linear_phase.setBandType(band, static_cast<eqlib::FilterType>(type));
    return eqlib_ok;
}

int xxwjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (freq_hz < eqlib::FREQ_MIN_HZ || freq_hz > eqlib::FREQ_MAX_HZ) return eqlib_err_param;
    h->linear_phase.setBandFreq(band, freq_hz);
    return eqlib_ok;
}

int xxwjhq_set_band_gain(eqlib_handle* h, int band, double gain_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (gain_db < eqlib::GAIN_MIN_DB || gain_db > eqlib::GAIN_MAX_DB) return eqlib_err_param;
    h->linear_phase.setBandGain(band, gain_db);
    return eqlib_ok;
}

int xxwjhq_set_band_q(eqlib_handle* h, int band, double q) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (q < eqlib::Q_MIN || q > eqlib::Q_MAX) return eqlib_err_param;
    h->linear_phase.setBandQ(band, q);
    return eqlib_ok;
}

int xxwjhq_set_band_enable(eqlib_handle* h, int band, int enable) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    h->linear_phase.setBandEnable(band, enable != 0);
    return eqlib_ok;
}

int xxwjhq_get_band_type(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = static_cast<int>(h->linear_phase.getBandType(band));
    return eqlib_ok;
}

int xxwjhq_get_band_freq(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->linear_phase.getBandFreq(band);
    return eqlib_ok;
}

int xxwjhq_get_band_gain(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->linear_phase.getBandGain(band);
    return eqlib_ok;
}

int xxwjhq_get_band_q(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->linear_phase.getBandQ(band);
    return eqlib_ok;
}

int xxwjhq_get_band_enable(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->linear_phase.getBandEnable(band) ? 1 : 0;
    return eqlib_ok;
}

int xxwjhq_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->linear_phase.reset();
    return eqlib_ok;
}

int xxwjhq_rebuild(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->linear_phase.rebuild();
    return eqlib_ok;
}

int xxwjhq_get_latency(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->linear_phase.getLatency();
    return eqlib_ok;
}

int xxwjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->linear_phase.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int xxwjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->linear_phase.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int hhxw_set_sample_rate(eqlib_handle* h, double sr) {
    if (!h) return eqlib_err_handle;
    if (sr < eqlib::SR_MIN || sr > eqlib::SR_MAX) return eqlib_err_param;
    h->hybrid_phase.setSampleRate(sr);
    return eqlib_ok;
}

int hhxw_set_channels(eqlib_handle* h, int ch) {
    if (!h) return eqlib_err_handle;
    if (ch < 1 || ch > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    h->hybrid_phase.setChannels(ch);
    return eqlib_ok;
}

int hhxw_set_band_phase_mode(eqlib_handle* h, int band, int mode) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (mode < 0 || mode > 2) return eqlib_err_param;
    h->hybrid_phase.setBandPhaseMode(band, static_cast<eqlib::PhaseMode>(mode));
    return eqlib_ok;
}

int hhxw_set_band_type(eqlib_handle* h, int band, int type) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (type < 0 || type > 7) return eqlib_err_param;
    h->hybrid_phase.setBandType(band, static_cast<eqlib::FilterType>(type));
    return eqlib_ok;
}

int hhxw_set_band_freq(eqlib_handle* h, int band, double freq_hz) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (freq_hz < eqlib::FREQ_MIN_HZ || freq_hz > eqlib::FREQ_MAX_HZ) return eqlib_err_param;
    h->hybrid_phase.setBandFreq(band, freq_hz);
    return eqlib_ok;
}

int hhxw_set_band_gain(eqlib_handle* h, int band, double gain_db) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (gain_db < eqlib::GAIN_MIN_DB || gain_db > eqlib::GAIN_MAX_DB) return eqlib_err_param;
    h->hybrid_phase.setBandGain(band, gain_db);
    return eqlib_ok;
}

int hhxw_set_band_q(eqlib_handle* h, int band, double q) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    if (q < eqlib::Q_MIN || q > eqlib::Q_MAX) return eqlib_err_param;
    h->hybrid_phase.setBandQ(band, q);
    return eqlib_ok;
}

int hhxw_set_band_enable(eqlib_handle* h, int band, int enable) {
    if (!h) return eqlib_err_handle;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    h->hybrid_phase.setBandEnable(band, enable != 0);
    return eqlib_ok;
}

int hhxw_set_linear_kernel_size(eqlib_handle* h, int size) {
    if (!h) return eqlib_err_handle;
    if (size < 32 || size > eqlib::LinearPhaseEq::MAX_KERNEL) return eqlib_err_param;
    h->hybrid_phase.setLinearKernelSize(size);
    return eqlib_ok;
}

int hhxw_get_band_phase_mode(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = static_cast<int>(h->hybrid_phase.getBandPhaseMode(band));
    return eqlib_ok;
}

int hhxw_get_band_type(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = static_cast<int>(h->hybrid_phase.getBandType(band));
    return eqlib_ok;
}

int hhxw_get_band_freq(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->hybrid_phase.getBandFreq(band);
    return eqlib_ok;
}

int hhxw_get_band_gain(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->hybrid_phase.getBandGain(band);
    return eqlib_ok;
}

int hhxw_get_band_q(eqlib_handle* h, int band, double* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->hybrid_phase.getBandQ(band);
    return eqlib_ok;
}

int hhxw_get_band_enable(eqlib_handle* h, int band, int* out) {
    if (!h || !out) return eqlib_err_param;
    if (band < 0 || band >= eqlib::NUM_BANDS) return eqlib_err_param;
    *out = h->hybrid_phase.getBandEnable(band) ? 1 : 0;
    return eqlib_ok;
}

int hhxw_get_linear_kernel_size(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->hybrid_phase.getLinearKernelSize();
    return eqlib_ok;
}

int hhxw_get_latency(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->hybrid_phase.getLatency();
    return eqlib_ok;
}

int hhxw_rebuild(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->hybrid_phase.rebuild();
    return eqlib_ok;
}

int hhxw_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->hybrid_phase.reset();
    return eqlib_ok;
}

int hhxw_process(eqlib_handle* h, const float* in, float* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->hybrid_phase.processBlockFloat(in, out, frames, channels);
    return eqlib_ok;
}

int hhxw_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels) {
    if (!h) return eqlib_err_handle;
    if (!in || !out) return eqlib_err_param;
    if (frames <= 0 || channels <= 0) return eqlib_err_param;
    h->hybrid_phase.processBlock(in, out, frames, channels);
    return eqlib_ok;
}

int cy_set_channels(eqlib_handle* h, int ch) {
    if (!h) return eqlib_err_handle;
    if (ch < 1 || ch > eqlib::WAV_MAX_CHANNELS) return eqlib_err_param;
    h->resampler.setChannels(ch);
    return eqlib_ok;
}

int cy_set_input_rate(eqlib_handle* h, double rate) {
    if (!h) return eqlib_err_handle;
    if (rate < 8000.0 || rate > 384000.0) return eqlib_err_param;
    h->resampler.setInputRate(rate);
    return eqlib_ok;
}

int cy_set_output_rate(eqlib_handle* h, double rate) {
    if (!h) return eqlib_err_handle;
    if (rate < 8000.0 || rate > 384000.0) return eqlib_err_param;
    h->resampler.setOutputRate(rate);
    return eqlib_ok;
}

int cy_set_quality(eqlib_handle* h, int taps) {
    if (!h) return eqlib_err_handle;
    if (taps < 8 || taps > 128) return eqlib_err_param;
    h->resampler.setQuality(taps);
    return eqlib_ok;
}

int cy_get_channels(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->resampler.getChannels();
    return eqlib_ok;
}

int cy_get_input_rate(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->resampler.getInputRate();
    return eqlib_ok;
}

int cy_get_output_rate(eqlib_handle* h, double* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->resampler.getOutputRate();
    return eqlib_ok;
}

int cy_get_quality(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->resampler.getQuality();
    return eqlib_ok;
}

int cy_get_latency(eqlib_handle* h, int* out) {
    if (!h || !out) return eqlib_err_param;
    *out = h->resampler.getLatency();
    return eqlib_ok;
}

int cy_reset(eqlib_handle* h) {
    if (!h) return eqlib_err_handle;
    h->resampler.reset();
    return eqlib_ok;
}

int cy_get_expected_output(eqlib_handle* h, int in_frames, int* out_frames) {
    if (!h || !out_frames) return eqlib_err_param;
    if (in_frames <= 0) return eqlib_err_param;
    double ratio = h->resampler.getOutputRate() / h->resampler.getInputRate();
    *out_frames = static_cast<int>(std::ceil(in_frames * ratio)) + 2;
    return eqlib_ok;
}

int cy_process(eqlib_handle* h, const float* in, int in_frames, int channels,
               float* out, int out_capacity, int* out_frames) {
    if (!h) return eqlib_err_handle;
    if (!in || !out || !out_frames) return eqlib_err_param;
    if (in_frames <= 0 || channels <= 0 || out_capacity <= 0) return eqlib_err_param;
    std::vector<float> tmp;
    int got = 0;
    if (h->resampler.process(in, in_frames, channels, tmp, got) != 0) return eqlib_err_state;
    if (got > out_capacity) return eqlib_err_param;
    if (got > 0) {
        std::memcpy(out, tmp.data(),
                    static_cast<std::size_t>(got) * channels * sizeof(float));
    }
    *out_frames = got;
    return eqlib_ok;
}

int cy_process_double(eqlib_handle* h, const double* in, int in_frames, int channels,
                      double* out, int out_capacity, int* out_frames) {
    if (!h) return eqlib_err_handle;
    if (!in || !out || !out_frames) return eqlib_err_param;
    if (in_frames <= 0 || channels <= 0 || out_capacity <= 0) return eqlib_err_param;
    std::vector<double> tmp;
    int got = 0;
    if (h->resampler.processDouble(in, in_frames, channels, tmp, got) != 0)
        return eqlib_err_state;
    if (got > out_capacity) return eqlib_err_param;
    if (got > 0) {
        std::memcpy(out, tmp.data(),
                    static_cast<std::size_t>(got) * channels * sizeof(double));
    }
    *out_frames = got;
    return eqlib_ok;
}

}
