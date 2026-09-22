#include "nccwj.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::MemoryWavWriter writer;
    eqlib::WavHeaderInfo info{};
    info.sample_rate = 48000;
    info.channels = 2;
    info.format = eqlib::WavFormat::Float32;
    info.num_frames = 0;

    if (!writer.open(info)) {
        std::printf("open_write failed\n");
        return 1;
    }

    const int frames = 1024;
    float data[frames * 2];
    for (int i = 0; i < frames * 2; ++i) {
        data[i] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 0.5f;
    }

    writer.writeFloat32(data, frames, 2);
    writer.finalize();
    std::printf("size=%zu frames=%llu\n",
                writer.getSize(),
                static_cast<unsigned long long>(writer.getFramesWritten()));

    eqlib::MemoryWavReader reader;
    if (!reader.open(writer.getData(), writer.getSize())) {
        std::printf("open_read failed\n");
        return 1;
    }

    eqlib::WavHeaderInfo rinfo = reader.getInfo();
    std::printf("sr=%d ch=%d bits=%d\n", rinfo.sample_rate, rinfo.channels, rinfo.bits_per_sample);

    float read_data[frames * 2];
    reader.readFloat32(read_data, frames, 2);
    reader.close();
    std::printf("read0=%.6f read_last=%.6f\n", read_data[0], read_data[frames * 2 - 1]);

    return 0;
}
