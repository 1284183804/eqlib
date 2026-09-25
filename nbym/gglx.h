#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace eqlib {

constexpr int    NUM_BANDS          = 7;
constexpr int    MAX_BANDS          = 93;
constexpr int    DEFAULT_BANDS      = 7;
constexpr int    CURVE_MAX_POINTS   = 20001;
constexpr int    AUTO_EQ_MAX_POINTS = 20001;
constexpr double SR_MIN             = 32000.0;
constexpr double SR_MAX             = 384000.0;
constexpr double GAIN_MIN_DB        = -60.0;
constexpr double GAIN_MAX_DB        =  24.0;
constexpr double FREQ_MIN_HZ        =  20.0;
constexpr double FREQ_MAX_HZ        = 20000.0;
constexpr double Q_MIN              = 0.1;
constexpr double Q_MAX              = 18.0;
constexpr int    FFT_MAX_SIZE       = 8192;
constexpr int    WAV_MAX_CHANNELS   = 64;
constexpr int    AUDIO_BLOCK_FRAMES = 4096;

constexpr uint32_t PARAM_MASK_GAIN    = 1u << 0;
constexpr uint32_t PARAM_MASK_TYPE    = 1u << 1;
constexpr uint32_t PARAM_MASK_FREQ    = 1u << 2;
constexpr uint32_t PARAM_MASK_Q       = 1u << 3;
constexpr uint32_t PARAM_MASK_ENABLE  = 1u << 4;
constexpr uint32_t PARAM_MASK_TARGET  = 1u << 5;
constexpr uint32_t PARAM_MASK_RANGE   = 1u << 6;
constexpr uint32_t PARAM_MASK_ATK     = 1u << 7;
constexpr uint32_t PARAM_MASK_REL     = 1u << 8;
constexpr uint32_t PARAM_MASK_PERCENT = 1u << 9;
constexpr uint32_t PARAM_MASK_MODE    = 1u << 10;

enum class FilterType : int {
    Peaking = 0,
    LowShelf,
    HighShelf,
    LowPass,
    HighPass,
    BandPass,
    Notch,
    AllPass,
};

enum class PhaseMode : int {
    Minimum = 0,
    Linear,
    Zero,
};

enum class LatencyClass : int {
    Realtime = 0,
    Interactive,
    Production,
    Offline,
};

enum class Precision : int {
    Float = 0,
    Double,
};

enum class EqError : int {
    Ok = 0,
    InvalidParam,
    InvalidHandle,
    OutOfMemory,
    Unsupported,
    InvalidState,
    IoError,
};

enum class DynMode : int {
    Downward = 0,
    Upward,
};

enum class SidechainSource : int {
    Internal = 0,
    External,
};

enum class MidSideMode : int {
    Stereo = 0,
    MidOnly,
    SideOnly,
    MidSide,
};

enum class WindowType : int {
    Rectangular = 0,
    Hann,
    Hamming,
    Blackman,
    FlatTop,
};

enum class SaturationType : int {
    SoftClip = 0,
    Tape,
    Tube,
    Diode,
};

enum class WavFormat : int {
    Float32 = 0,
    Int24,
    Int16,
};

enum class AudioFileFormat : int {
    Unknown = 0,
    Wav,
    Mp3,
    Flac,
};

enum class CurveSource : int {
    Flat     = 0,
    Builtin  = 1,
    Injected = 2,
};

enum class BuiltinCurve : int {
    Flat       = 0,
    Custom     = 1,
    Harman     = 2,
    Diffuse    = 3,
    Studio     = 4,
    Loudness   = 5,
    Count      = 6,
};

struct BandRange {
    double freq_low_hz{20.0};
    double freq_high_hz{20000.0};
};

struct CurvePoint {
    double freq_hz{20.0};
    double gain_db{0.0};
};

struct BuiltinCurveData {
    const char* name{nullptr};
    const CurvePoint* points{nullptr};
    int num_points{0};
};

struct ParamChange {
    int      band{-1};
    int      type{-1};
    double   freq_hz{0.0};
    double   gain_db{0.0};
    double   q{0.0};
    int      enable{-1};
    double   percent{0.0};
    int      mode{-1};
    uint32_t mask{0};
};

struct DynParams {
    double  threshold_db{-18.0};
    double  ratio{2.0};
    double  attack_ms{10.0};
    double  release_ms{200.0};
    double  range_db{12.0};
    DynMode mode{DynMode::Downward};
};

struct AutoEqInput {
    const double* measured_db;
    const double* reference_db;
    const double* freqs_hz;
    int           num_bins;
    double        pct;
};

struct AutoEqOutput {
    double gains_db[MAX_BANDS];
    double rms_error_db;
    double max_error_db;
};

struct SpectrumConfig {
    int        fft_size{2048};
    WindowType window{WindowType::Hann};
    double     overlap{0.75};
};

struct SpectrumFrame {
    std::vector<double> magnitudes;
    std::vector<double> freqs_hz;
    int    num_bins{0};
    double sample_rate{48000.0};

    void resize(int n) {
        if (n < 0) n = 0;
        magnitudes.resize(static_cast<std::size_t>(n));
        freqs_hz.resize(static_cast<std::size_t>(n));
        num_bins = n;
    }
};

struct AgcParams {
    double target_dbfs{-18.0};
    double attack_ms{10.0};
    double release_ms{200.0};
    double max_boost_db{12.0};
    double max_cut_db{-12.0};
    double gate_threshold_dbfs{-60.0};
    bool   gate_enable{true};
};

struct SaturationParams {
    SaturationType type{SaturationType::SoftClip};
    double         drive_db{6.0};
    double         mix{1.0};
    double         output_db{0.0};
};

struct MultiChannelConfig {
    int  num_channels{2};
    int  block_size{512};
    int  num_threads{0};
    bool enable_multithread{true};
};

struct WavHeaderInfo {
    int        sample_rate;
    int        channels;
    int        bits_per_sample;
    WavFormat  format;
    uint64_t   num_frames;
};

struct AudioFileInfo {
    int              sample_rate{0};
    int              channels{0};
    int              bits_per_sample{0};
    uint64_t         num_frames{0};
    AudioFileFormat  format{AudioFileFormat::Unknown};
};

struct CurveData {
    std::vector<double> freqs_hz;
    std::vector<double> levels_db;
    std::vector<double> gains_db;
    int    num_points{0};
    int    source{0};
    bool   is_measured{false};
    double reference_db{-18.0};

    void resize(int n) {
        if (n < 0) n = 0;
        freqs_hz.resize(static_cast<std::size_t>(n));
        levels_db.resize(static_cast<std::size_t>(n));
        gains_db.resize(static_cast<std::size_t>(n));
        num_points = n;
    }
};

struct BandConfig {
    double low_hz{20.0};
    double high_hz{20000.0};
    double center_hz{1000.0};
    double q{1.0};
    double gain_db{0.0};
    int    type{0};
    int    enable{1};
    double target_dbfs{-18.0};
    double dyn_range_db{12.0};
    double dyn_attack_ms{10.0};
    double dyn_release_ms{200.0};
    double dyn_percent{100.0};
    int    dyn_mode{0};
};

struct BenchResult {
    int      block_size;
    double   cpu_time_ms;
    double   real_time_ms;
    double   cpu_load_percent;
    uint64_t samples_processed;
};

}
