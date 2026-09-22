#include "wjdx.h"
#include <cstdio>
#include <cmath>

int main() {
    const int sr = 48000;
    const int ch = 2;
    const int frames = 1024;

    eqlib::WavHeaderInfo info{};
    info.sample_rate = sr;
    info.channels = ch;
    info.format = eqlib::WavFormat::Float32;
    info.num_frames = 0;

    eqlib::WavWriter writer;
    if (!writer.open("cs_wjdx_test.wav", info)) {
        std::printf("open_write failed\n");
        return 1;
    }

    float data[frames * ch];
    for (int i = 0; i < frames * ch; ++i) {
        data[i] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / sr) * 0.5f;
    }

    writer.writeFloat32(data, frames, ch);
    writer.finalize();
    writer.close();
    std::printf("written=%llu\n", static_cast<unsigned long long>(writer.getFramesWritten()));

    eqlib::WavReader reader;
    if (!reader.open("cs_wjdx_test.wav")) {
        std::printf("open_read failed\n");
        return 1;
    }

    eqlib::WavHeaderInfo rinfo = reader.getInfo();
    std::printf("sr=%d ch=%d bits=%d format=%d\n",
                rinfo.sample_rate, rinfo.channels,
                rinfo.bits_per_sample, static_cast<int>(rinfo.format));

    float read_data[frames * ch];
    reader.readFloat32(read_data, frames, ch);
    reader.close();
    std::printf("read0=%.6f read_last=%.6f\n",
                read_data[0], read_data[frames * ch - 1]);

    return 0;
}
