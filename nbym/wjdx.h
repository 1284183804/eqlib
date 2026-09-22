#pragma once
#include "yyh.h"
#include "gglx.h"
#include <cstdio>
#include <string>

namespace eqlib {

class EQLIB_INTERNAL WavWriter {
    std::FILE*    m_file{nullptr};
    WavHeaderInfo m_info{};
    uint64_t      m_frames_written{0};
    bool          m_finalized{false};

public:
    WavWriter();
    ~WavWriter();
    WavWriter(const WavWriter&) = delete;
    WavWriter& operator=(const WavWriter&) = delete;
    WavWriter(WavWriter&&) = delete;
    WavWriter& operator=(WavWriter&&) = delete;

    bool open(const std::string& path, const WavHeaderInfo& info);
    bool writeFloat32(const float* data, int frames, int channels);
    bool writeDouble(const double* data, int frames, int channels);
    bool finalize();
    void close();
    bool isOpen() const;
    uint64_t getFramesWritten() const;
    WavHeaderInfo getInfo() const;
};

class EQLIB_INTERNAL WavReader {
    std::FILE*    m_file{nullptr};
    WavHeaderInfo m_info{};
    uint64_t      m_frames_read{0};
    std::size_t   m_data_offset{0};
    std::size_t   m_data_size{0};

public:
    WavReader();
    ~WavReader();
    WavReader(const WavReader&) = delete;
    WavReader& operator=(const WavReader&) = delete;
    WavReader(WavReader&&) = delete;
    WavReader& operator=(WavReader&&) = delete;

    bool open(const std::string& path);
    WavHeaderInfo getInfo() const;
    bool readFloat32(float* out, int frames, int channels);
    bool readDouble(double* out, int frames, int channels);
    void close();
    bool isOpen() const;
    uint64_t getFramesRead() const;
};

}
