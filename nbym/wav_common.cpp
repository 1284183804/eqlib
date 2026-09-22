#include "wav_common.h"
#include <cstring>

namespace eqlib {

uint16_t wav_read_u16_le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0])
         | (static_cast<uint16_t>(p[1]) << 8);
}

uint32_t wav_read_u32_le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

void wav_write_u16_le(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
}

void wav_write_u32_le(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

bool wav_match_fourcc(const uint8_t* p, const char* s) {
    return p[0] == static_cast<uint8_t>(s[0])
        && p[1] == static_cast<uint8_t>(s[1])
        && p[2] == static_cast<uint8_t>(s[2])
        && p[3] == static_cast<uint8_t>(s[3]);
}

WavFormat wav_format_from_audio_info(int audio_format, int bits) {
    if (audio_format == 3 && bits == 32) return WavFormat::Float32;
    if (audio_format == 1 && bits == 16) return WavFormat::Int16;
    if (audio_format == 1 && bits == 24) return WavFormat::Int24;
    return WavFormat::Float32;
}

bool wav_parse_header(const uint8_t* data, std::size_t size, WavParseResult& out) {
    if (!data || size < 44) return false;
    if (!wav_match_fourcc(data, "RIFF")) return false;
    if (!wav_match_fourcc(data + 8, "WAVE")) return false;

    std::size_t pos = 12;
    bool fmt_found = false;
    bool data_found = false;

    while (pos + 8 <= size) {
        const uint8_t* fourcc = data + pos;
        uint32_t chunk_size = wav_read_u32_le(data + pos + 4);
        std::size_t payload = pos + 8;

        if (wav_match_fourcc(fourcc, "fmt ")) {
            if (payload + 16 > size) return false;
            out.audio_format = static_cast<int>(wav_read_u16_le(data + payload));
            out.channels = static_cast<int>(wav_read_u16_le(data + payload + 2));
            out.sample_rate = static_cast<int>(wav_read_u32_le(data + payload + 4));
            out.bits_per_sample = static_cast<int>(wav_read_u16_le(data + payload + 14));
            fmt_found = true;
        } else if (wav_match_fourcc(fourcc, "data")) {
            out.data_offset = payload;
            out.data_size = chunk_size;
            data_found = true;
            break;
        }

        std::size_t skip = static_cast<std::size_t>(chunk_size) + (chunk_size & 1u);
        pos = payload + skip;
    }

    if (!fmt_found || !data_found) return false;
    if (out.channels <= 0 || out.channels > WAV_MAX_CHANNELS) return false;
    if (out.sample_rate <= 0) return false;

    if (out.audio_format != 1 && out.audio_format != 3) return false;
    if (out.bits_per_sample != 16
        && out.bits_per_sample != 24
        && out.bits_per_sample != 32) return false;
    if (out.audio_format == 3 && out.bits_per_sample != 32) return false;

    if (out.data_offset + out.data_size > size) {
        out.data_size = size - out.data_offset;
    }

    int bytes_per_sample = out.bits_per_sample / 8;
    if (bytes_per_sample <= 0) return false;
    uint64_t frame_bytes = static_cast<uint64_t>(out.channels)
                         * static_cast<uint64_t>(bytes_per_sample);
    if (frame_bytes == 0) return false;
    out.num_frames = static_cast<uint64_t>(out.data_size) / frame_bytes;

    out.format = wav_format_from_audio_info(out.audio_format, out.bits_per_sample);
    return true;
}

void wav_write_canonical_header(uint8_t* out44, int sample_rate,
                                int channels, int bits_per_sample) {
    if (!out44) return;
    int bytes_per_sample = bits_per_sample / 8;
    uint32_t byte_rate = static_cast<uint32_t>(sample_rate)
                       * static_cast<uint32_t>(channels)
                       * static_cast<uint32_t>(bytes_per_sample);
    uint16_t block_align = static_cast<uint16_t>(channels * bytes_per_sample);
    uint16_t audio_format = (bits_per_sample == 32) ? 3 : 1;

    std::memcpy(out44, "RIFF", 4);
    wav_write_u32_le(out44 + 4, 0);
    std::memcpy(out44 + 8, "WAVE", 4);
    std::memcpy(out44 + 12, "fmt ", 4);
    wav_write_u32_le(out44 + 16, 16);
    wav_write_u16_le(out44 + 20, audio_format);
    wav_write_u16_le(out44 + 22, static_cast<uint16_t>(channels));
    wav_write_u32_le(out44 + 24, static_cast<uint32_t>(sample_rate));
    wav_write_u32_le(out44 + 28, byte_rate);
    wav_write_u16_le(out44 + 32, block_align);
    wav_write_u16_le(out44 + 34, static_cast<uint16_t>(bits_per_sample));
    std::memcpy(out44 + 36, "data", 4);
    wav_write_u32_le(out44 + 40, 0);
}

}
