#pragma once
#include "gglx.h"
#include <cstdint>
#include <cstddef>

namespace eqlib {

struct WavParseResult {
    int         sample_rate{0};
    int         channels{0};
    int         bits_per_sample{0};
    int         audio_format{0};
    std::size_t data_offset{0};
    std::size_t data_size{0};
    uint64_t    num_frames{0};
    WavFormat   format{WavFormat::Float32};
};

uint16_t wav_read_u16_le(const uint8_t* p);
uint32_t wav_read_u32_le(const uint8_t* p);
void     wav_write_u16_le(uint8_t* p, uint16_t v);
void     wav_write_u32_le(uint8_t* p, uint32_t v);
bool     wav_match_fourcc(const uint8_t* p, const char* s);
bool     wav_parse_header(const uint8_t* data, std::size_t size, WavParseResult& out);
void     wav_write_canonical_header(uint8_t* out44, int sample_rate,
                                    int channels, int bits_per_sample);
WavFormat wav_format_from_audio_info(int audio_format, int bits);

}
