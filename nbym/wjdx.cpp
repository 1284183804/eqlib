#include "wjdx.h"
#include "wav_common.h"
#include <cstring>
#include <cmath>
#include <vector>

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

void pack_i24(unsigned char* out, int32_t v) {
    out[0] = static_cast<unsigned char>(v & 0xFF);
    out[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
    out[2] = static_cast<unsigned char>((v >> 16) & 0xFF);
}

int32_t unpack_i24(const unsigned char* p) {
    int32_t v = static_cast<int32_t>(p[0])
              | (static_cast<int32_t>(p[1]) << 8)
              | (static_cast<int32_t>(p[2]) << 16);
    if (v & 0x800000) v |= 0xFF000000;
    return v;
}

bool fread_exact(std::FILE* f, void* buf, std::size_t bytes) {
    return std::fread(buf, 1, bytes, f) == bytes;
}

}

WavWriter::WavWriter() = default;

WavWriter::~WavWriter() {
    close();
}

bool WavWriter::open(const std::string& path, const WavHeaderInfo& info) {
    if (m_file) return false;
    if (info.sample_rate <= 0) return false;
    if (info.channels <= 0 || info.channels > WAV_MAX_CHANNELS) return false;

    m_file = std::fopen(path.c_str(), "wb");
    if (!m_file) return false;

    m_info = info;
    m_info.bits_per_sample = format_to_bits(info.format);

    uint8_t header[44];
    wav_write_canonical_header(header, info.sample_rate, info.channels,
                               m_info.bits_per_sample);
    if (std::fwrite(header, 1, 44, m_file) != 44) {
        std::fclose(m_file);
        m_file = nullptr;
        return false;
    }

    m_frames_written = 0;
    m_finalized = false;
    return true;
}

bool WavWriter::writeFloat32(const float* data, int frames, int channels) {
    if (!m_file || !data) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    if (m_info.format == WavFormat::Float32) {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        if (std::fwrite(data, sizeof(float), n, m_file) != n) return false;
    } else if (m_info.format == WavFormat::Int24) {
        std::vector<unsigned char> buf(static_cast<std::size_t>(frames)
                                     * static_cast<std::size_t>(channels) * 3);
        std::size_t w = 0;
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                int32_t v = clamp_to_i24(static_cast<double>(data[f * channels + c]));
                pack_i24(buf.data() + w, v);
                w += 3;
            }
        }
        if (std::fwrite(buf.data(), 1, buf.size(), m_file) != buf.size()) return false;
    } else {
        std::vector<int16_t> buf(static_cast<std::size_t>(frames)
                               * static_cast<std::size_t>(channels));
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                buf[static_cast<std::size_t>(f * channels + c)] =
                    clamp_to_i16(static_cast<double>(data[f * channels + c]));
            }
        }
        if (std::fwrite(buf.data(), sizeof(int16_t), buf.size(), m_file) != buf.size()) return false;
    }

    m_frames_written += static_cast<uint64_t>(frames);
    return true;
}

bool WavWriter::writeDouble(const double* data, int frames, int channels) {
    if (!m_file || !data) return false;
    if (frames <= 0) return false;
    if (channels != m_info.channels) return false;

    if (m_info.format == WavFormat::Float32) {
        std::vector<float> buf(static_cast<std::size_t>(frames)
                             * static_cast<std::size_t>(channels));
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                buf[static_cast<std::size_t>(f * channels + c)] =
                    static_cast<float>(data[f * channels + c]);
            }
        }
        if (std::fwrite(buf.data(), sizeof(float), buf.size(), m_file) != buf.size()) return false;
    } else if (m_info.format == WavFormat::Int24) {
        std::vector<unsigned char> buf(static_cast<std::size_t>(frames)
                                     * static_cast<std::size_t>(channels) * 3);
        std::size_t w = 0;
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                int32_t v = clamp_to_i24(data[f * channels + c]);
                pack_i24(buf.data() + w, v);
                w += 3;
            }
        }
        if (std::fwrite(buf.data(), 1, buf.size(), m_file) != buf.size()) return false;
    } else {
        std::vector<int16_t> buf(static_cast<std::size_t>(frames)
                               * static_cast<std::size_t>(channels));
        for (int f = 0; f < frames; ++f) {
            for (int c = 0; c < channels; ++c) {
                buf[static_cast<std::size_t>(f * channels + c)] =
                    clamp_to_i16(data[f * channels + c]);
            }
        }
        if (std::fwrite(buf.data(), sizeof(int16_t), buf.size(), m_file) != buf.size()) return false;
    }

    m_frames_written += static_cast<uint64_t>(frames);
    return true;
}

bool WavWriter::finalize() {
    if (!m_file || m_finalized) return false;
    int bytes_per_sample = m_info.bits_per_sample / 8;
    if (bytes_per_sample <= 0) return false;

    uint64_t total_bytes = m_frames_written
                         * static_cast<uint64_t>(m_info.channels)
                         * static_cast<uint64_t>(bytes_per_sample);
    uint32_t data_size = static_cast<uint32_t>(total_bytes);
    uint32_t riff_size = 36u + data_size;

    uint8_t buf[4];
    wav_write_u32_le(buf, riff_size);
    if (std::fseek(m_file, 4, SEEK_SET) != 0) return false;
    if (std::fwrite(buf, 1, 4, m_file) != 4) return false;

    wav_write_u32_le(buf, data_size);
    if (std::fseek(m_file, 40, SEEK_SET) != 0) return false;
    if (std::fwrite(buf, 1, 4, m_file) != 4) return false;

    std::fflush(m_file);
    m_finalized = true;
    return true;
}

void WavWriter::close() {
    if (!m_file) return;
    if (!m_finalized) finalize();
    std::fclose(m_file);
    m_file = nullptr;
}

bool WavWriter::isOpen() const {
    return m_file != nullptr;
}

uint64_t WavWriter::getFramesWritten() const {
    return m_frames_written;
}

WavHeaderInfo WavWriter::getInfo() const {
    return m_info;
}

WavReader::WavReader() = default;

WavReader::~WavReader() {
    close();
}

bool WavReader::open(const std::string& path) {
    if (m_file) return false;

    m_file = std::fopen(path.c_str(), "rb");
    if (!m_file) return false;

    if (std::fseek(m_file, 0, SEEK_END) != 0) {
        close();
        return false;
    }
    long fsize = std::ftell(m_file);
    if (fsize < 44) {
        close();
        return false;
    }
    if (std::fseek(m_file, 0, SEEK_SET) != 0) {
        close();
        return false;
    }

    std::vector<uint8_t> head(static_cast<std::size_t>(fsize));
    if (std::fread(head.data(), 1, head.size(), m_file) != head.size()) {
        close();
        return false;
    }

    WavParseResult parsed{};
    if (!wav_parse_header(head.data(), head.size(), parsed)) {
        close();
        return false;
    }

    m_info.sample_rate = parsed.sample_rate;
    m_info.channels = parsed.channels;
    m_info.bits_per_sample = parsed.bits_per_sample;
    m_info.format = parsed.format;
    m_info.num_frames = parsed.num_frames;

    m_data_offset = parsed.data_offset;
    m_data_size = parsed.data_size;
    m_frames_read = 0;

    if (std::fseek(m_file, static_cast<long>(m_data_offset), SEEK_SET) != 0) {
        close();
        return false;
    }
    return true;
}

WavHeaderInfo WavReader::getInfo() const {
    return m_info;
}

bool WavReader::readFloat32(float* out, int frames, int channels) {
    if (!m_file || !out) return false;
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

    std::size_t total = static_cast<std::size_t>(want);
    std::vector<uint8_t> raw(total);
    if (!fread_exact(m_file, raw.data(), total)) return false;

    if (m_info.format == WavFormat::Float32) {
        std::memcpy(out, raw.data(), total);
    } else if (m_info.format == WavFormat::Int24) {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        for (std::size_t i = 0; i < n; ++i) {
            int32_t v = unpack_i24(raw.data() + i * 3);
            out[i] = static_cast<float>(v) / 8388608.0f;
        }
    } else {
        std::size_t n = static_cast<std::size_t>(frames)
                      * static_cast<std::size_t>(channels);
        const int16_t* src = reinterpret_cast<const int16_t*>(raw.data());
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = static_cast<float>(src[i]) / 32768.0f;
        }
    }

    m_frames_read += static_cast<uint64_t>(frames);
    return true;
}

bool WavReader::readDouble(double* out, int frames, int channels) {
    if (!m_file || !out) return false;
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

    std::size_t total = static_cast<std::size_t>(want);
    std::vector<uint8_t> raw(total);
    if (!fread_exact(m_file, raw.data(), total)) return false;

    std::size_t n = static_cast<std::size_t>(frames)
                  * static_cast<std::size_t>(channels);

    if (m_info.format == WavFormat::Float32) {
        const float* src = reinterpret_cast<const float*>(raw.data());
        for (std::size_t i = 0; i < n; ++i) out[i] = static_cast<double>(src[i]);
    } else if (m_info.format == WavFormat::Int24) {
        for (std::size_t i = 0; i < n; ++i) {
            int32_t v = unpack_i24(raw.data() + i * 3);
            out[i] = static_cast<double>(v) / 8388608.0;
        }
    } else {
        const int16_t* src = reinterpret_cast<const int16_t*>(raw.data());
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = static_cast<double>(src[i]) / 32768.0;
        }
    }

    m_frames_read += static_cast<uint64_t>(frames);
    return true;
}

void WavReader::close() {
    if (m_file) {
        std::fclose(m_file);
        m_file = nullptr;
    }
    m_data_offset = 0;
    m_data_size = 0;
    m_frames_read = 0;
}

bool WavReader::isOpen() const {
    return m_file != nullptr;
}

uint64_t WavReader::getFramesRead() const {
    return m_frames_read;
}

}
