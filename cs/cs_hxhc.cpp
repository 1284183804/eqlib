#include "hxhc.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::StreamBuffer sb(64, 2);
    sb.setCapacityFrames(1024);

    const int push_frames = 100;
    float in[push_frames * 2];
    for (int i = 0; i < push_frames * 2; ++i) in[i] = 0.5f;

    std::size_t pushed = sb.pushInput(in, push_frames);
    std::printf("pushed=%zu available=%zu\n", pushed, sb.getInputAvailable());

    std::size_t processed = sb.process(
        [](const float* input, float* output, int frames, int channels) {
            for (int f = 0; f < frames; ++f) {
                for (int c = 0; c < channels; ++c) {
                    output[f * channels + c] = input[f * channels + c] * 2.0f;
                }
            }
        });
    std::printf("processed=%zu output_avail=%zu\n", processed, sb.getOutputAvailable());

    float out[push_frames * 2];
    std::size_t popped = sb.popOutput(out, push_frames);
    std::printf("popped=%zu out0=%.4f\n", popped, out[0]);

    return 0;
}
