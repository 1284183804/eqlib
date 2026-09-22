#include "eqlib/eqlib.h"
#include <cstdio>
#include <cmath>
#include <vector>

int main() {
    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) return 1;
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    int rc = jhq_set_num_bands(h, 93);
    std::printf("set 93 bands rc=%d\n", rc);
    int nb = 0;
    jhq_get_num_bands(h, &nb);
    std::printf("num_bands=%d\n", nb);

    double f_low = 20.0;
    for (int i = 0; i < nb; ++i) {
        double f_high = f_low * std::pow(20000.0 / 20.0, 1.0 / nb);
        if (i == nb - 1) f_high = 20000.0;
        jhq_set_band_range(h, i, f_low, f_high);
        jhq_set_band_gain(h, i, (i % 2 == 0) ? 3.0 : -3.0);
        f_low = f_high;
    }

    int count = 0;
    jhq_get_builtin_curve_count(h, &count);
    std::printf("builtin curves=%d\n", count);
    for (int i = 0; i < count; ++i) {
        char name[64];
        jhq_get_builtin_curve_name(h, i, name, 64);
        std::printf("curve[%d]=%s\n", i, name);
    }

    jhq_set_builtin_curve(h, eqlib_curve_lossless);
    jhq_apply_builtin_curve(h);
    std::printf("applied lossless curve\n");

    const int frames = 96000;
    std::vector<float> in(frames * 2);
    std::vector<float> out(frames * 2);
    for (int i = 0; i < frames; ++i) {
        double t = static_cast<double>(i) / 48000.0;
        in[i * 2] = static_cast<float>(std::sin(2.0 * 3.14159265 * 440.0 * t) * 0.5);
        in[i * 2 + 1] = in[i * 2];
    }
    jhq_process(h, in.data(), out.data(), frames, 2);

    double rms_in = 0.0, rms_out = 0.0;
    for (int i = 0; i < frames * 2; ++i) {
        rms_in += in[i] * in[i];
        rms_out += out[i] * out[i];
    }
    rms_in = std::sqrt(rms_in / (frames * 2));
    rms_out = std::sqrt(rms_out / (frames * 2));
    std::printf("rms_in=%.6f rms_out=%.6f\n", rms_in, rms_out);

    jhq_set_global_gain(h, 6.0);
    jhq_process(h, in.data(), out.data(), frames, 2);
    double rms_boost = 0.0;
    for (int i = 0; i < frames * 2; ++i) rms_boost += out[i] * out[i];
    rms_boost = std::sqrt(rms_boost / (frames * 2));
    std::printf("rms_boost_6db=%.6f ratio=%.3f\n", rms_boost, rms_boost / rms_out);

    jhq_destroy(h);
    return 0;
}
