#include "ypdr.h"
#include "wjdx.h"
#include <cstring>
#include <cmath>
#include <cstdio>

#include "dr_mp3.h"
#include "dr_flac.h"

namespace eqlib {

AudioFileLoader::AudioFileLoader() = default;

AudioFileLoader::~AudioFileLoader() {
    clear();
}

void AudioFileLoader::clear() {
    delete[] m_data_f;
    m_data_f = nullptr;
    delete[] m_data_d;
    m_data_d = nullptr;
    m_num_frames = 0;
    m_channels = 0;
    m_sample_rate = 0;
    m_loaded = false;
    m_info = AudioFileInfo{};
}

bool AudioFileLoader::isLoaded() const {
    return m_loaded;
}

AudioFileInfo AudioFileLoader::getInfo() const {
    return m_info;
}

const float* AudioFileLoader::getFloatData() const {
    return m_data_f;
}

const double* AudioFileLoader::getDoubleData() const {
    return m_data_d;
}

uint64_t AudioFileLoader::getNumFrames() const {
    return m_num_frames;
}

int AudioFileLoader::getChannels() const {
    return m_channels;
}

int AudioFileLoader::getSampleRate() const {
    return m_sample_rate;
}

bool AudioFileLoader::loadWav(const char* path) {
    clear();
    if (!path) return false;

    WavReader reader;
    if (!reader.open(path)) return false;

    WavHeaderInfo info = reader.getInfo();
    m_sample_rate = info.sample_rate;
    m_channels = info.channels;

    std::FILE* f = std::fopen(path, "rb");
    if (!f) {
        reader.close();
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    long file_size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::fclose(f);
    reader.close();

    uint64_t est_frames = 0;
    if (file_size > 44) {
        int bytes_per_sample = info.bits_per_sample / 8;
        if (bytes_per_sample > 0 && m_channels > 0) {
            est_frames = (static_cast<uint64_t>(file_size) - 44) / (m_channels * bytes_per_sample);
        }
    }
    if (est_frames == 0) return false;

    if (!reader.open(path)) return false;

    m_num_frames = est_frames;
    m_data_f = new (std::nothrow) float[est_frames * m_channels];
    m_data_d = new (std::nothrow) double[est_frames * m_channels];
    if (!m_data_f || !m_data_d) {
        clear();
        reader.close();
        return false;
    }

    uint64_t read_total = 0;
    while (read_total < est_frames) {
        uint64_t chunk = est_frames - read_total;
        if (chunk > 4096) chunk = 4096;
        if (!reader.readFloat32(m_data_f + read_total * m_channels, static_cast<int>(chunk), m_channels)) {
            break;
        }
        read_total += chunk;
    }
    reader.close();
    m_num_frames = read_total;

    for (uint64_t i = 0; i < m_num_frames * m_channels; ++i) {
        m_data_d[i] = static_cast<double>(m_data_f[i]);
    }

    m_info.sample_rate = m_sample_rate;
    m_info.channels = m_channels;
    m_info.bits_per_sample = info.bits_per_sample;
    m_info.num_frames = m_num_frames;
    m_info.format = AudioFileFormat::Wav;
    m_loaded = true;
    return true;
}

bool AudioFileLoader::loadMp3(const char* path) {
    clear();
    if (!path) return false;

    drmp3 mp3;
    if (!drmp3_init_file(&mp3, path, nullptr)) return false;

    m_sample_rate = static_cast<int>(mp3.sampleRate);
    m_channels = static_cast<int>(mp3.channels);
    m_num_frames = drmp3_get_pcm_frame_count(&mp3);
    if (m_num_frames == 0) {
        drmp3_uninit(&mp3);
        return false;
    }

    m_data_f = new (std::nothrow) float[m_num_frames * m_channels];
    m_data_d = new (std::nothrow) double[m_num_frames * m_channels];
    if (!m_data_f || !m_data_d) {
        drmp3_uninit(&mp3);
        clear();
        return false;
    }

    uint64_t total_read = 0;
    while (total_read < m_num_frames) {
        uint64_t chunk = m_num_frames - total_read;
        if (chunk > 4096) chunk = 4096;
        uint64_t got = drmp3_read_pcm_frames_f32(&mp3, chunk, m_data_f + total_read * m_channels);
        if (got == 0) break;
        total_read += got;
    }
    m_num_frames = total_read;
    drmp3_uninit(&mp3);

    for (uint64_t i = 0; i < m_num_frames * m_channels; ++i) {
        m_data_d[i] = static_cast<double>(m_data_f[i]);
    }

    m_info.sample_rate = m_sample_rate;
    m_info.channels = m_channels;
    m_info.bits_per_sample = 32;
    m_info.num_frames = m_num_frames;
    m_info.format = AudioFileFormat::Mp3;
    m_loaded = true;
    return true;
}

bool AudioFileLoader::loadFlac(const char* path) {
    clear();
    if (!path) return false;

    drflac* flac = drflac_open_file(path, nullptr);
    if (!flac) return false;

    m_sample_rate = static_cast<int>(flac->sampleRate);
    m_channels = static_cast<int>(flac->channels);
    m_num_frames = flac->totalPCMFrameCount;
    if (m_num_frames == 0) {
        drflac_close(flac);
        return false;
    }

    m_data_f = new (std::nothrow) float[m_num_frames * m_channels];
    m_data_d = new (std::nothrow) double[m_num_frames * m_channels];
    if (!m_data_f || !m_data_d) {
        drflac_close(flac);
        clear();
        return false;
    }

    uint64_t total_read = 0;
    while (total_read < m_num_frames) {
        uint64_t chunk = m_num_frames - total_read;
        if (chunk > 4096) chunk = 4096;
        uint64_t got = drflac_read_pcm_frames_f32(flac, chunk, m_data_f + total_read * m_channels);
        if (got == 0) break;
        total_read += got;
    }
    m_num_frames = total_read;
    drflac_close(flac);

    for (uint64_t i = 0; i < m_num_frames * m_channels; ++i) {
        m_data_d[i] = static_cast<double>(m_data_f[i]);
    }

    m_info.sample_rate = m_sample_rate;
    m_info.channels = m_channels;
    m_info.bits_per_sample = 32;
    m_info.num_frames = m_num_frames;
    m_info.format = AudioFileFormat::Flac;
    m_loaded = true;
    return true;
}

bool AudioFileLoader::convertToDouble() {
    if (!m_loaded || !m_data_f) return false;
    if (m_data_d) return true;
    m_data_d = new (std::nothrow) double[m_num_frames * m_channels];
    if (!m_data_d) return false;
    for (uint64_t i = 0; i < m_num_frames * m_channels; ++i) {
        m_data_d[i] = static_cast<double>(m_data_f[i]);
    }
    return true;
}

bool AudioFileLoader::convertToFloat() {
    if (!m_loaded || !m_data_d) return false;
    if (m_data_f) return true;
    m_data_f = new (std::nothrow) float[m_num_frames * m_channels];
    if (!m_data_f) return false;
    for (uint64_t i = 0; i < m_num_frames * m_channels; ++i) {
        m_data_f[i] = static_cast<float>(m_data_d[i]);
    }
    return true;
}

}
