#pragma once
#include "yyh.h"
#include "gglx.h"
#include <cstdint>
#include <cstddef>
#include <vector>

namespace eqlib {

class EQLIB_INTERNAL MemoryWavWriter {
    std::vector<uint8_t> m_data;
    WavHeaderInfo        m_info{};
    uint64_t             m_frames_written{0};
    bool                 m_finalized{false};
    bool                 m_opened{false};

public:
    MemoryWavWriter() = default;
    MemoryWavWriter(const MemoryWavWriter&) = delete;
    MemoryWavWriter& operator=(const MemoryWavWriter&) = delete;
    MemoryWavWriter(MemoryWavWriter&&) = delete;
    MemoryWavWriter& operator=(MemoryWavWriter&&) = delete;

    bool open(const WavHeaderInfo& info);
    bool writeFloat32(const float* data, int frames, int channels);
    bool writeDouble(const double* data, int frames, int channels);
    bool finalize();
    void clear();
    bool isOpen() const;
    const uint8_t* getData() const;
    std::size_t getSize() const;
    uint64_t getFramesWritten() const;
    WavHeaderInfo getInfo() const;
};

class EQLIB_INTERNAL MemoryWavReader {
    const uint8_t* m_data{nullptr};
    std::size_t    m_size{0};
    WavHeaderInfo  m_info{};
    std::size_t    m_data_offset{0};
    std::size_t    m_data_size{0};
    uint64_t       m_frames_read{0};
    bool           m_opened{false};

public:
    MemoryWavReader() = default;
    MemoryWavReader(const MemoryWavReader&) = delete;
    MemoryWavReader& operator=(const MemoryWavReader&) = delete;
    MemoryWavReader(MemoryWavReader&&) = delete;
    MemoryWavReader& operator=(MemoryWavReader&&) = delete;

    bool open(const uint8_t* data, std::size_t size);
    WavHeaderInfo getInfo() const;
    bool readFloat32(float* out, int frames, int channels);
    bool readDouble(double* out, int frames, int channels);
    void close();
    bool isOpen() const;
    uint64_t getFramesRead() const;
};

}
