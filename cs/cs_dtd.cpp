#include "dtd.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::MultiChannelProcessor mc;
    mc.setNumChannels(2);
    mc.setBlockSize(512);
    mc.setEnableMultithread(true);
    mc.setNumThreads(0);

    std::printf("channels=%d block=%d threads=%d multithread=%d effective=%d\n",
                mc.getNumChannels(), mc.getBlockSize(),
                mc.getNumThreads(), mc.getEnableMultithread() ? 1 : 0,
                mc.getEffectiveThreads());

    const int frames = 4096;
    const int channels = 2;
    float in[frames * channels];
    float out[frames * channels];
    for (int i = 0; i < frames * channels; ++i) {
        in[i] = 0.1f;
        out[i] = 0.0f;
    }

    mc.processParallel(in, out, frames, channels,
        [](const float* input, float* output, int start, int end, int ch) {
            for (int f = start; f < end; ++f) {
                for (int c = 0; c < ch; ++c) {
                    output[f * ch + c] = input[f * ch + c] * 2.0f;
                }
            }
        });

    std::printf("out0=%.4f out_last=%.4f\n", out[0], out[frames * channels - 1]);

    mc.reset();
    std::printf("reset ok\n");

    return 0;
}
