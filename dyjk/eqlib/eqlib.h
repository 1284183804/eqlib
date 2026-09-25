#pragma once
#include <stdint.h>

#if !defined(EQLIB_API)
    #if defined(_WIN32)
        #if defined(EQLIB_BUILD_SHARED)
            #define EQLIB_API __declspec(dllexport)
        #else
            #define EQLIB_API
        #endif
    #else
        #if defined(EQLIB_BUILD_SHARED)
            #define EQLIB_API __attribute__((visibility("default")))
        #else
            #define EQLIB_API
        #endif
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define eqlib_ok 0
#define eqlib_err_param -1
#define eqlib_err_handle -2
#define eqlib_err_memory -3
#define eqlib_err_unsupported -4
#define eqlib_err_state -5
#define eqlib_err_io -6

typedef struct eqlib_handle eqlib_handle;

enum {
    eqlib_filter_peaking   = 0,
    eqlib_filter_lowshelf  = 1,
    eqlib_filter_highshelf = 2,
    eqlib_filter_lowpass   = 3,
    eqlib_filter_highpass  = 4,
    eqlib_filter_bandpass  = 5,
    eqlib_filter_notch     = 6,
    eqlib_filter_allpass   = 7,
};

enum {
    eqlib_num_bands = 7,
};

enum {
    eqlib_max_bands        = 93,
    eqlib_curve_max_points = 20001,
};

enum {
    eqlib_dyn_downward = 0,
    eqlib_dyn_upward   = 1,
};

enum {
    eqlib_ms_stereo    = 0,
    eqlib_ms_mid_only  = 1,
    eqlib_ms_side_only = 2,
    eqlib_ms_mid_side  = 3,
};

enum {
    eqlib_window_rectangular = 0,
    eqlib_window_hann        = 1,
    eqlib_window_hamming     = 2,
    eqlib_window_blackman    = 3,
    eqlib_window_flattop     = 4,
};

enum {
    eqlib_fft_512  = 512,
    eqlib_fft_1024 = 1024,
    eqlib_fft_2048 = 2048,
    eqlib_fft_4096 = 4096,
    eqlib_fft_8192 = 8192,
};

enum {
    eqlib_sat_softclip = 0,
    eqlib_sat_tape     = 1,
    eqlib_sat_tube     = 2,
    eqlib_sat_diode    = 3,
};

enum {
    eqlib_wav_float32 = 0,
    eqlib_wav_int24   = 1,
    eqlib_wav_int16   = 2,
};

enum {
    eqlib_file_unknown = 0,
    eqlib_file_wav     = 1,
    eqlib_file_mp3     = 2,
    eqlib_file_flac    = 3,
};

enum {
    eqlib_lp_kernel_256  = 256,
    eqlib_lp_kernel_512  = 512,
    eqlib_lp_kernel_1024 = 1024,
};

enum {
    eqlib_phase_minimum = 0,
    eqlib_phase_linear  = 1,
    eqlib_phase_zero    = 2,
};

enum {
    eqlib_curve_flat     = 0,
    eqlib_curve_builtin  = 1,
    eqlib_curve_injected = 2,
};

EQLIB_API int jhq_create(eqlib_handle** out);
EQLIB_API int jhq_destroy(eqlib_handle* h);
EQLIB_API int jhq_reset(eqlib_handle* h);
EQLIB_API int jhq_clear_all(eqlib_handle* h);
EQLIB_API int jhq_set_sample_rate(eqlib_handle* h, double sr);
EQLIB_API int jhq_set_channels(eqlib_handle* h, int ch);
EQLIB_API int jhq_set_band_type(eqlib_handle* h, int band, int type);
EQLIB_API int jhq_set_band_freq(eqlib_handle* h, int band, double freq_hz);
EQLIB_API int jhq_set_band_gain(eqlib_handle* h, int band, double gain_db);
EQLIB_API int jhq_set_band_q(eqlib_handle* h, int band, double q);
EQLIB_API int jhq_set_band_enable(eqlib_handle* h, int band, int enable);
EQLIB_API int jhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int jhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);
EQLIB_API int jhq_get_band_gain(eqlib_handle* h, int band, double* out);
EQLIB_API int jhq_get_band_type(eqlib_handle* h, int band, int* out);
EQLIB_API int jhq_get_band_freq(eqlib_handle* h, int band, double* out);
EQLIB_API int jhq_get_band_q(eqlib_handle* h, int band, double* out);
EQLIB_API int jhq_get_band_enable(eqlib_handle* h, int band, int* out);
EQLIB_API int jhq_get_version(char* buf, int buf_size);

EQLIB_API int dsjhq_set_num_bands(eqlib_handle* h, int n);
EQLIB_API int dsjhq_get_num_bands(eqlib_handle* h, int* out);
EQLIB_API int dsjhq_auto_distribute(eqlib_handle* h, double low_hz, double high_hz);
EQLIB_API int dsjhq_set_band_range(eqlib_handle* h, int band, double low_hz, double high_hz);
EQLIB_API int dsjhq_set_band_center(eqlib_handle* h, int band, double center_hz);
EQLIB_API int dsjhq_set_band_q(eqlib_handle* h, int band, double q);
EQLIB_API int dsjhq_set_band_gain(eqlib_handle* h, int band, double gain_db);
EQLIB_API int dsjhq_set_band_type(eqlib_handle* h, int band, int type);
EQLIB_API int dsjhq_set_band_enable(eqlib_handle* h, int band, int enable);
EQLIB_API int dsjhq_set_band_target_dbfs(eqlib_handle* h, int band, double target);
EQLIB_API int dsjhq_set_band_dyn_range(eqlib_handle* h, int band, double range_db);
EQLIB_API int dsjhq_set_band_dyn_attack(eqlib_handle* h, int band, double ms);
EQLIB_API int dsjhq_set_band_dyn_release(eqlib_handle* h, int band, double ms);
EQLIB_API int dsjhq_set_band_target_dbfs_all(eqlib_handle* h, double target);
EQLIB_API int dsjhq_set_band_dyn_range_all(eqlib_handle* h, double range_db);
EQLIB_API int dsjhq_set_band_dyn_attack_all(eqlib_handle* h, double ms);
EQLIB_API int dsjhq_set_band_dyn_release_all(eqlib_handle* h, double ms);
EQLIB_API int dsjhq_load_measured_curve(eqlib_handle* h, const double* freqs_hz, const double* levels_db, int num_points, double reference_db);
EQLIB_API int dsjhq_load_gain_curve(eqlib_handle* h, const double* freqs_hz, const double* gains_db, int num_points, int source);
EQLIB_API int dsjhq_load_builtin_curve(eqlib_handle* h);
EQLIB_API int dsjhq_clear_curve(eqlib_handle* h);
EQLIB_API int dsjhq_get_curve_info(eqlib_handle* h, int* num_points, int* source, int* is_measured, double* reference_db);
EQLIB_API int dsjhq_get_curve_point(eqlib_handle* h, int index, double* freq_hz, double* level_db, double* gain_db);
EQLIB_API int dsjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int dsjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);

EQLIB_API int dtjhq_set_band_threshold(eqlib_handle* h, int band, double threshold_db);
EQLIB_API int dtjhq_set_band_ratio(eqlib_handle* h, int band, double ratio);
EQLIB_API int dtjhq_get_band_ratio(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_set_band_percent(eqlib_handle* h, int band, double percent);
EQLIB_API int dtjhq_get_band_percent(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_set_band_attack(eqlib_handle* h, int band, double attack_ms);
EQLIB_API int dtjhq_set_band_release(eqlib_handle* h, int band, double release_ms);
EQLIB_API int dtjhq_set_band_range(eqlib_handle* h, int band, double range_db);
EQLIB_API int dtjhq_set_band_mode(eqlib_handle* h, int band, int mode);
EQLIB_API int dtjhq_get_band_mode(eqlib_handle* h, int band, int* out);
EQLIB_API int dtjhq_set_band_type(eqlib_handle* h, int band, int type);
EQLIB_API int dtjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz);
EQLIB_API int dtjhq_set_band_gain(eqlib_handle* h, int band, double gain_db);
EQLIB_API int dtjhq_set_band_q(eqlib_handle* h, int band, double q);
EQLIB_API int dtjhq_set_band_enable(eqlib_handle* h, int band, int enable);
EQLIB_API int dtjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int dtjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);
EQLIB_API int dtjhq_process_sidechain(eqlib_handle* h, const float* in, const float* sc, float* out, int frames, int channels);
EQLIB_API int dtjhq_process_sidechain_double(eqlib_handle* h, const double* in, const double* sc, double* out, int frames, int channels);
EQLIB_API int dtjhq_get_band_gain(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_threshold(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_attack(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_release(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_range(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_type(eqlib_handle* h, int band, int* out);
EQLIB_API int dtjhq_get_band_freq(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_q(eqlib_handle* h, int band, double* out);
EQLIB_API int dtjhq_get_band_enable(eqlib_handle* h, int band, int* out);

EQLIB_API int cl_set_enable(eqlib_handle* h, int enable);
EQLIB_API int cl_set_attack(eqlib_handle* h, double attack_ms);
EQLIB_API int cl_set_release(eqlib_handle* h, double release_ms);
EQLIB_API int cl_set_amount(eqlib_handle* h, double amount);
EQLIB_API int cl_set_target(eqlib_handle* h, double target_db);
EQLIB_API int cl_set_range(eqlib_handle* h, double range_db);
EQLIB_API int cl_reset(eqlib_handle* h);
EQLIB_API int cl_process(eqlib_handle* h, const float* sc, float* gain_out, int frames, int channels);
EQLIB_API int cl_process_double(eqlib_handle* h, const double* sc, double* gain_out, int frames, int channels);
EQLIB_API int cl_get_enable(eqlib_handle* h, int* out);
EQLIB_API int cl_get_attack(eqlib_handle* h, double* out);
EQLIB_API int cl_get_release(eqlib_handle* h, double* out);
EQLIB_API int cl_get_amount(eqlib_handle* h, double* out);
EQLIB_API int cl_get_target(eqlib_handle* h, double* out);
EQLIB_API int cl_get_range(eqlib_handle* h, double* out);

EQLIB_API int zc_set_mode(eqlib_handle* h, int mode);
EQLIB_API int zc_set_mid_gain(eqlib_handle* h, double db);
EQLIB_API int zc_set_side_gain(eqlib_handle* h, double db);
EQLIB_API int zc_reset(eqlib_handle* h);
EQLIB_API int zc_process(eqlib_handle* h, const float* in_l, const float* in_r, float* out_l, float* out_r, int frames);
EQLIB_API int zc_process_double(eqlib_handle* h, const double* in_l, const double* in_r, double* out_l, double* out_r, int frames);
EQLIB_API int zc_get_mode(eqlib_handle* h, int* out);
EQLIB_API int zc_get_mid_gain(eqlib_handle* h, double* out);
EQLIB_API int zc_get_side_gain(eqlib_handle* h, double* out);

EQLIB_API int ppfx_init(eqlib_handle* h, int fft_size, int window_type, double overlap);
EQLIB_API int ppfx_reset(eqlib_handle* h);
EQLIB_API int ppfx_set_smoothing(eqlib_handle* h, double smoothing);
EQLIB_API int ppfx_process(eqlib_handle* h, const float* in, int frames, int channels);
EQLIB_API int ppfx_process_double(eqlib_handle* h, const double* in, int frames, int channels);
EQLIB_API int ppfx_get_spectrum(eqlib_handle* h, float* mag_out, float* freq_out, int* num_bins);
EQLIB_API int ppfx_get_spectrum_double(eqlib_handle* h, double* mag_out, double* freq_out, int* num_bins);
EQLIB_API int ppfx_get_fft_size(eqlib_handle* h, int* out);
EQLIB_API int ppfx_get_hop_size(eqlib_handle* h, int* out);

EQLIB_API int zdjhq_set_sample_rate(eqlib_handle* h, double sr);
EQLIB_API int zdjhq_set_fft_size(eqlib_handle* h, int size);
EQLIB_API int zdjhq_set_window(eqlib_handle* h, int window_type);
EQLIB_API int zdjhq_set_pct(eqlib_handle* h, double pct);
EQLIB_API int zdjhq_set_num_bands(eqlib_handle* h, int n);
EQLIB_API int zdjhq_get_num_bands(eqlib_handle* h, int* out);
EQLIB_API int zdjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz);
EQLIB_API int zdjhq_set_band_freqs(eqlib_handle* h, const double* freqs_hz, int n);
EQLIB_API int zdjhq_get_band_freq(eqlib_handle* h, int band, double* out);
EQLIB_API int zdjhq_set_reference_curve(eqlib_handle* h, const double* freqs_hz, const double* gains_db, int num_points);
EQLIB_API int zdjhq_clear_reference_curve(eqlib_handle* h);
EQLIB_API int zdjhq_has_reference_curve(eqlib_handle* h, int* out);
EQLIB_API int zdjhq_get_reference_point_count(eqlib_handle* h, int* out);
EQLIB_API int zdjhq_get_reference_point(eqlib_handle* h, int index, double* freq_hz, double* gain_db);
EQLIB_API int zdjhq_start(eqlib_handle* h);
EQLIB_API int zdjhq_stop(eqlib_handle* h);
EQLIB_API int zdjhq_is_running(eqlib_handle* h, int* out);
EQLIB_API int zdjhq_has_result(eqlib_handle* h, int* out);
EQLIB_API int zdjhq_feed_spectrum(eqlib_handle* h);
EQLIB_API int zdjhq_compute_result(eqlib_handle* h);
EQLIB_API int zdjhq_get_result(eqlib_handle* h, double* gains_db, int* num_bands);
EQLIB_API int zdjhq_get_rms_error(eqlib_handle* h, double* out);
EQLIB_API int zdjhq_get_max_error(eqlib_handle* h, double* out);
EQLIB_API int zdjhq_reset(eqlib_handle* h);

EQLIB_API int zdzy_set_sample_rate(eqlib_handle* h, double sr);
EQLIB_API int zdzy_set_target(eqlib_handle* h, double target_dbfs);
EQLIB_API int zdzy_set_attack(eqlib_handle* h, double attack_ms);
EQLIB_API int zdzy_set_release(eqlib_handle* h, double release_ms);
EQLIB_API int zdzy_set_max_boost(eqlib_handle* h, double max_boost_db);
EQLIB_API int zdzy_set_max_cut(eqlib_handle* h, double max_cut_db);
EQLIB_API int zdzy_set_gate_enable(eqlib_handle* h, int enable);
EQLIB_API int zdzy_set_gate_threshold(eqlib_handle* h, double threshold_dbfs);
EQLIB_API int zdzy_reset(eqlib_handle* h);
EQLIB_API int zdzy_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int zdzy_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);
EQLIB_API int zdzy_get_current_gain_db(eqlib_handle* h, double* out);
EQLIB_API int zdzy_get_envelope_db(eqlib_handle* h, double* out);

EQLIB_API int bh_set_type(eqlib_handle* h, int type);
EQLIB_API int bh_set_drive(eqlib_handle* h, double drive_db);
EQLIB_API int bh_set_mix(eqlib_handle* h, double mix);
EQLIB_API int bh_set_output(eqlib_handle* h, double output_db);
EQLIB_API int bh_reset(eqlib_handle* h);
EQLIB_API int bh_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int bh_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);
EQLIB_API int bh_get_type(eqlib_handle* h, int* out);
EQLIB_API int bh_get_drive(eqlib_handle* h, double* out);
EQLIB_API int bh_get_mix(eqlib_handle* h, double* out);
EQLIB_API int bh_get_output(eqlib_handle* h, double* out);

EQLIB_API int dtd_set_num_channels(eqlib_handle* h, int ch);
EQLIB_API int dtd_set_block_size(eqlib_handle* h, int size);
EQLIB_API int dtd_set_num_threads(eqlib_handle* h, int n);
EQLIB_API int dtd_set_enable_multithread(eqlib_handle* h, int enable);
EQLIB_API int dtd_get_num_channels(eqlib_handle* h, int* out);
EQLIB_API int dtd_get_block_size(eqlib_handle* h, int* out);
EQLIB_API int dtd_get_num_threads(eqlib_handle* h, int* out);
EQLIB_API int dtd_get_enable_multithread(eqlib_handle* h, int* out);
EQLIB_API int dtd_get_effective_threads(eqlib_handle* h, int* out);
EQLIB_API int dtd_get_hardware_threads(eqlib_handle* h, int* out);
EQLIB_API int dtd_reset(eqlib_handle* h);

EQLIB_API int wjdx_wav_open_write(eqlib_handle* h, const char* path, int sample_rate, int channels, int format);
EQLIB_API int wjdx_wav_write_float(eqlib_handle* h, const float* data, int frames);
EQLIB_API int wjdx_wav_write_double(eqlib_handle* h, const double* data, int frames);
EQLIB_API int wjdx_wav_finalize(eqlib_handle* h);
EQLIB_API int wjdx_wav_close(eqlib_handle* h);
EQLIB_API int wjdx_wav_get_frames_written(eqlib_handle* h, uint64_t* out);
EQLIB_API int wjdx_wav_open_read(eqlib_handle* h, const char* path);
EQLIB_API int wjdx_wav_get_info(eqlib_handle* h, int* sample_rate, int* channels, int* bits, int* format, uint64_t* num_frames);
EQLIB_API int wjdx_wav_read_float(eqlib_handle* h, float* out, int frames);
EQLIB_API int wjdx_wav_read_double(eqlib_handle* h, double* out, int frames);
EQLIB_API int wjdx_wav_read_close(eqlib_handle* h);
EQLIB_API int wjdx_wav_get_frames_read(eqlib_handle* h, uint64_t* out);

EQLIB_API int nccwj_open_write(eqlib_handle* h, int sample_rate, int channels, int format);
EQLIB_API int nccwj_write_float(eqlib_handle* h, const float* data, int frames);
EQLIB_API int nccwj_write_double(eqlib_handle* h, const double* data, int frames);
EQLIB_API int nccwj_finalize(eqlib_handle* h);
EQLIB_API int nccwj_get_data(eqlib_handle* h, const uint8_t** out_data, uint64_t* out_size);
EQLIB_API int nccwj_get_frames_written(eqlib_handle* h, uint64_t* out);
EQLIB_API int nccwj_clear(eqlib_handle* h);
EQLIB_API int nccwj_open_read(eqlib_handle* h, const uint8_t* data, uint64_t size);
EQLIB_API int nccwj_get_info(eqlib_handle* h, int* sample_rate, int* channels, int* bits, int* format);
EQLIB_API int nccwj_read_float(eqlib_handle* h, float* out, int frames);
EQLIB_API int nccwj_read_double(eqlib_handle* h, double* out, int frames);
EQLIB_API int nccwj_get_frames_read(eqlib_handle* h, uint64_t* out);
EQLIB_API int nccwj_close_read(eqlib_handle* h);

EQLIB_API int ypdr_load(eqlib_handle* h, const char* path, int format);
EQLIB_API int ypdr_load_wav(eqlib_handle* h, const char* path);
EQLIB_API int ypdr_load_mp3(eqlib_handle* h, const char* path);
EQLIB_API int ypdr_load_flac(eqlib_handle* h, const char* path);
EQLIB_API int ypdr_get_info(eqlib_handle* h, int* sample_rate, int* channels, uint64_t* num_frames, int* format);
EQLIB_API int ypdr_get_float(eqlib_handle* h, float* out, uint64_t frames, int channels);
EQLIB_API int ypdr_get_double(eqlib_handle* h, double* out, uint64_t frames, int channels);
EQLIB_API int ypdr_get_num_frames(eqlib_handle* h, uint64_t* out);
EQLIB_API int ypdr_get_channels(eqlib_handle* h, int* out);
EQLIB_API int ypdr_get_sample_rate(eqlib_handle* h, int* out);
EQLIB_API int ypdr_clear(eqlib_handle* h);

EQLIB_API int ypdc_process_and_save(eqlib_handle* h, const char* input_path, const char* output_path, int output_format);
EQLIB_API int ypdc_process_and_save_float(eqlib_handle* h, const char* input_path, const char* output_path, int output_format);
EQLIB_API int ypdc_process_and_save_double(eqlib_handle* h, const char* input_path, const char* output_path, int output_format);

EQLIB_API int xxwjhq_set_kernel_size(eqlib_handle* h, int size);
EQLIB_API int xxwjhq_get_kernel_size(eqlib_handle* h, int* out);
EQLIB_API int xxwjhq_set_band_type(eqlib_handle* h, int band, int type);
EQLIB_API int xxwjhq_set_band_freq(eqlib_handle* h, int band, double freq_hz);
EQLIB_API int xxwjhq_set_band_gain(eqlib_handle* h, int band, double gain_db);
EQLIB_API int xxwjhq_set_band_q(eqlib_handle* h, int band, double q);
EQLIB_API int xxwjhq_set_band_enable(eqlib_handle* h, int band, int enable);
EQLIB_API int xxwjhq_get_band_type(eqlib_handle* h, int band, int* out);
EQLIB_API int xxwjhq_get_band_freq(eqlib_handle* h, int band, double* out);
EQLIB_API int xxwjhq_get_band_gain(eqlib_handle* h, int band, double* out);
EQLIB_API int xxwjhq_get_band_q(eqlib_handle* h, int band, double* out);
EQLIB_API int xxwjhq_get_band_enable(eqlib_handle* h, int band, int* out);
EQLIB_API int xxwjhq_reset(eqlib_handle* h);
EQLIB_API int xxwjhq_rebuild(eqlib_handle* h);
EQLIB_API int xxwjhq_get_latency(eqlib_handle* h, int* out);
EQLIB_API int xxwjhq_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int xxwjhq_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);

EQLIB_API int hhxw_set_sample_rate(eqlib_handle* h, double sr);
EQLIB_API int hhxw_set_channels(eqlib_handle* h, int ch);
EQLIB_API int hhxw_set_band_phase_mode(eqlib_handle* h, int band, int mode);
EQLIB_API int hhxw_set_band_type(eqlib_handle* h, int band, int type);
EQLIB_API int hhxw_set_band_freq(eqlib_handle* h, int band, double freq_hz);
EQLIB_API int hhxw_set_band_gain(eqlib_handle* h, int band, double gain_db);
EQLIB_API int hhxw_set_band_q(eqlib_handle* h, int band, double q);
EQLIB_API int hhxw_set_band_enable(eqlib_handle* h, int band, int enable);
EQLIB_API int hhxw_set_linear_kernel_size(eqlib_handle* h, int size);
EQLIB_API int hhxw_get_band_phase_mode(eqlib_handle* h, int band, int* out);
EQLIB_API int hhxw_get_band_type(eqlib_handle* h, int band, int* out);
EQLIB_API int hhxw_get_band_freq(eqlib_handle* h, int band, double* out);
EQLIB_API int hhxw_get_band_gain(eqlib_handle* h, int band, double* out);
EQLIB_API int hhxw_get_band_q(eqlib_handle* h, int band, double* out);
EQLIB_API int hhxw_get_band_enable(eqlib_handle* h, int band, int* out);
EQLIB_API int hhxw_get_linear_kernel_size(eqlib_handle* h, int* out);
EQLIB_API int hhxw_get_latency(eqlib_handle* h, int* out);
EQLIB_API int hhxw_rebuild(eqlib_handle* h);
EQLIB_API int hhxw_reset(eqlib_handle* h);
EQLIB_API int hhxw_process(eqlib_handle* h, const float* in, float* out, int frames, int channels);
EQLIB_API int hhxw_process_double(eqlib_handle* h, const double* in, double* out, int frames, int channels);

EQLIB_API int cy_set_channels(eqlib_handle* h, int ch);
EQLIB_API int cy_set_input_rate(eqlib_handle* h, double rate);
EQLIB_API int cy_set_output_rate(eqlib_handle* h, double rate);
EQLIB_API int cy_set_quality(eqlib_handle* h, int taps);
EQLIB_API int cy_get_channels(eqlib_handle* h, int* out);
EQLIB_API int cy_get_input_rate(eqlib_handle* h, double* out);
EQLIB_API int cy_get_output_rate(eqlib_handle* h, double* out);
EQLIB_API int cy_get_quality(eqlib_handle* h, int* out);
EQLIB_API int cy_get_latency(eqlib_handle* h, int* out);
EQLIB_API int cy_reset(eqlib_handle* h);
EQLIB_API int cy_get_expected_output(eqlib_handle* h, int in_frames, int* out_frames);
EQLIB_API int cy_process(eqlib_handle* h, const float* in, int in_frames, int channels, float* out, int out_capacity, int* out_frames);
EQLIB_API int cy_process_double(eqlib_handle* h, const double* in, int in_frames, int channels, double* out, int out_capacity, int* out_frames);

#ifdef __cplusplus
}
#endif
