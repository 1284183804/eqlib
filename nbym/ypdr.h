#pragma once
#include "gglx.h"
#include <cstdint>

namespace eqlib {

class AudioFileLoader {
    float* m_data_f{nullptr};
    double* m_data_d{nullptr};
    AudioFileInfo m_info{};
    uint64_t m_num_frames{0};
    int m_channels{0};
    int m_sample_rate{0};
    bool m_loaded{false};

public:
    AudioFileLoader();
    ~AudioFileLoader();
    AudioFileLoader(const AudioFileLoader&) = delete;
    AudioFileLoader& operator=(const AudioFileLoader&) = delete;
    AudioFileLoader(AudioFileLoader&&) = delete;
    AudioFileLoader& operator=(AudioFileLoader&&) = delete;

    bool loadWav(const char* path);
    bool loadMp3(const char* path);
    bool loadFlac(const char* path);
    void clear();
    bool isLoaded() const;
    AudioFileInfo getInfo() const;
    const float* getFloatData() const;
    const double* getDoubleData() const;
    uint64_t getNumFrames() const;
    int getChannels() const;
    int getSampleRate() const;
    bool convertToDouble();
    bool convertToFloat();
};

}
