#include "nccwj.h"
#include "wav_common.h"
#include <cstring>
#include <cmath>

namespace eqlib {

namespace {

int format_to_bits(WavFormat fmt) {
    if (fmt == WavFormat::Int24) return 24;
    if (fmt == WavFormat::Int16) return 16;
    return 32;
}

int16_t clamp_to_i16(double x) {
    if (x > 1.0) x = 1.0;
    if (x < -1.0) x = -1.0;
    double scaled = (x < 0.0) ? (x * 32768.0) : (x * 32767.0);
    if (scaled > 32767.0) scaled = 32767.0;
    if (scaled < -32768.0) scaled = -32768.0;
    return static_cast<int16_t>(scaled);
}

int32_t clamp_to_i24(double x) {
    if (x > 1.0) x = 1.0;
    if (x < -1.0) x = -1.0;
    double scaled = (x < 0.0) ? (x * 8388608.0) : (x * 8388607.0);
    if (scaled > 8388607.0) scaled = 8388607.0;
    if (scaled < -8388608.0) scaled = -8388608.0;
    return static_cast<int32_t>(scaled);
}

void pack_i24(std::vector<uint8_t>& v, int32_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
}

int32_t unpack_i24(const uint8_t* p) {
    int32_t v = static_cast<int32_t>(p[0])
              | (static_cast<int32_t>(p[1]) << 8)
              | (static_cast<int32_t>(p[2]) << 16);
    if (v & 0x800000) v |= 0xFF000000;
    return v;
}

}

bool MemoryWavWriter::open(const WavHeaderInfo& info) {
    if (m_opened) return false;
    if (info.sample_rate <= 0) return false;
    if (info.channels <= 0 || info.channels > WAV_MAX_CHANNELS) return false;

    m_info = info;
    m_info.bits_per_sample = format_to_bits(info.format);

    m_data.clear();
    m_data.resize(44);
    wav_write_canonical_header(m_data.data(), info.sample_rate, info.channels,
                               m_info.bits_per_sample);

    m_frames_written = 0;
    m_finalized = false;
    m_opened = true;
    return true;
}

bool MemoryWavWriter::writeFloat32(const float* data, int frames, int channels) {
    if (!m_opened || !data) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    if (m_info.format == WavFormat::Float32) {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(data);
        m_data.insert(m_data.end(), src, src + n * sizeof(float));
    } else if (m_info.format == WavFormat::Int24) {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                pack_i24(m_data, clamp_to_i24(static_cast<double>(data[f * channels + c])));
            }
        }
    } else {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                int16_t v = clamp_to_i16(static_cast<double>(data[f * channels + c]));
                m_data.push_back(static_cast<uint8_t>(v & 0xFF));
                m_data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            }
        }
    }

    m_frames_written += static_cast<uint64_t>(frames);
    return true;
}

bool MemoryWavWriter::writeDouble(const double* data, int frames, int channels) {
    if (!m_opened || !data) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    if (m_info.format == WavFormat::Float32) {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        std::vector<float> buf(n);
        for (std::size_t i = 0; i < n; ++i) buf[i] = static_cast<float>(data[i]);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(buf.data());
        m_data.insert(m_data.end(), src, src + n * sizeof(float));
    } else if (m_info.format == WavFormat::Int24) {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                pack_i24(m_data, clamp_to_i24(data[f * channels + c]));
            }
        }
    } else {
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                int16_t v = clamp_to_i16(data[f * channels + c]);
                m_data.push_back(static_cast<uint8_t>(v & 0xFF));
                m_data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            }
        }
    }

    m_frames_written += static_cast<uint64_t>(frames);
    return true;
}

bool MemoryWavWriter::finalize() {
    if (!m_opened || m_finalized) return false;
    int bytes_per_sample = m_info.bits_per_sample / 8;
    if (bytes_per_sample <= 0) return false;

    uint64_t total_bytes = m_frames_written
                         * static_cast<uint64_t>(m_info.channels)
                         * static_cast<uint64_t>(bytes_per_sample);
    uint32_t data_size = static_cast<uint32_t>(total_bytes);
    uint32_t riff_size = 36u + data_size;

    wav_write_u32_le(m_data.data() + 4, riff_size);
    wav_write_u32_le(m_data.data() + 40, data_size);
    m_finalized = true;
    return true;
}

void MemoryWavWriter::clear() {
    m_data.clear();
    m_info = WavHeaderInfo{};
    m_frames_written = 0;
    m_finalized = false;
    m_opened = false;
}

bool MemoryWavWriter::isOpen() const {
    return m_opened;
}

const uint8_t* MemoryWavWriter::getData() const {
    return m_data.empty() ? nullptr : m_data.data();
}

std::size_t MemoryWavWriter::getSize() const {
    return m_data.size();
}

uint64_t MemoryWavWriter::getFramesWritten() const {
    return m_frames_written;
}

WavHeaderInfo MemoryWavWriter::getInfo() const {
    return m_info;
}

bool MemoryWavReader::open(const uint8_t* data, std::size_t size) {
    if (m_opened) return false;
    if (!data || size < 44) return false;

    WavParseResult parsed{};
    if (!wav_parse_header(data, size, parsed)) return false;

    m_data = data;
    m_size = size;
    m_info.sample_rate = parsed.sample_rate;
    m_info.channels = parsed.channels;
    m_info.bits_per_sample = parsed.bits_per_sample;
    m_info.format = parsed.format;
    m_info.num_frames = parsed.num_frames;
    m_data_offset = parsed.data_offset;
    m_data_size = parsed.data_size;
    m_frames_read = 0;
    m_opened = true;
    return true;
}

WavHeaderInfo MemoryWavReader::getInfo() const {
    return m_info;
}

bool MemoryWavReader::readFloat32(float* out, int frames, int channels) {
    if (!m_opened || !out) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    int bytes_per_sample = m_info.bits_per_sample / 8;
    if (bytes_per_sample <= 0) return false;

    uint64_t want = static_cast<uint64_t>(frames)
                  * static_cast<uint64_t>(channels)
                  * static_cast<uint64_t>(bytes_per_sample);
    uint64_t have = static_cast<uint64_t>(m_data_size)
                  - m_frames_read
                  * static_cast<uint64_t>(channels)
                  * static_cast<uint64_t>(bytes_per_sample);
    if (want > have) return false;

    const uint8_t* src = m_data + m_data_offset
                       + m_frames_read * static_cast<uint64_t>(channels)
                       * static_cast<uint64_t>(bytes_per_sample);

    if (m_info.format == WavFormat::Float32) {
        std::memcpy(out, src, static_cast<std::size_t>(want));
    } else if (m_info.format == WavFormat::Int24) {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        for (std::size_t i = 0; i < n; ++i) {
            int32_t v = unpack_i24(src + i * 3);
            out[i] = static_cast<float>(v) / 8388608.0f;
        }
    } else {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        for (std::size_t i = 0; i < n; ++i) {
            int16_t v = static_cast<int16_t>(wav_read_u16_le(src + i * 2));
            out[i] = static_cast<float>(v) / 32768.0f;
        }
    }

    m_frames_read += static_cast<uint64_t>(frames);
    return true;
}

bool MemoryWavReader::readDouble(double* out, int frames, int channels) {
    if (!m_opened || !out) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    int bytes_per_sample = m_info.bits_per_sample / 8;
    if (bytes_per_sample <= 0) return false;

    uint64_t want = static_cast<uint64_t>(frames)
                  * static_cast<uint64_t>(channels)
                  * static_cast<uint64_t>(bytes_per_sample);
    uint64_t have = static_cast<uint64_t>(m_data_size)
                  - m_frames_read
                  * static_cast<uint64_t>(channels)
                  * static_cast<uint64_t>(bytes_per_sample);
    if (want > have) return false;

    const uint8_t* src = m_data + m_data_offset
                       + m_frames_read * static_cast<uint64_t>(channels)
                       * static_cast<uint64_t>(bytes_per_sample);

    std::size_t n = static_cast<std::size_t>(frames)
                  * static_cast<std::size_t>(channels);

    if (m_info.format == WavFormat::Float32) {
        const float* fs = reinterpret_cast<const float*>(src);
        for (std::size_t i = 0; i < n; ++i) out[i] = static_cast<double>(fs[i]);
    } else if (m_info.format == WavFormat::Int24) {
        for (std::size_t i = 0; i < n; ++i) {
            int32_t v = unpack_i24(src + i * 3);
            out[i] = static_cast<double>(v) / 8388608.0;
        }
    } else {
        for (std::size_t i = 0; i < n; ++i) {
            int16_t v = static_cast<int16_t>(wav_read_u16_le(src + i * 2));
            out[i] = static_cast<double>(v) / 32768.0;
        }
    }

    m_frames_read += static_cast<uint64_t>(frames);
    return true;
}

void MemoryWavReader::close() {
    m_data = nullptr;
    m_size = 0;
    m_data_offset = 0;
    m_data_size = 0;
    m_frames_read = 0;
    m_opened = false;
}

bool MemoryWavReader::isOpen() const {
    return m_opened;
}

uint64_t MemoryWavReader::getFramesRead() const {
    return m_frames_read;
}

}
