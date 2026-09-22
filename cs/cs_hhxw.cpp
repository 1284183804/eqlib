#include "eqlib/eqlib.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) return 1;

    hhxw_set_sample_rate(h, 48000.0);
    hhxw_set_channels(h, 1);
    hhxw_set_linear_kernel_size(h, eqlib_lp_kernel_512);

    hhxw_set_band_phase_mode(h, 0, eqlib_phase_minimum);
    hhxw_set_band_type(h, 0, eqlib_filter_peaking);
    hhxw_set_band_freq(h, 0, 1000.0);
    hhxw_set_band_gain(h, 0, 6.0);

    hhxw_set_band_phase_mode(h, 1, eqlib_phase_linear);
    hhxw_set_band_type(h, 1, eqlib_filter_peaking);
    hhxw_set_band_freq(h, 1, 3000.0);
    hhxw_set_band_gain(h, 1, -6.0);

    for (int i = 2; i < eqlib_num_bands; ++i) {
        hhxw_set_band_enable(h, i, 0);
    }

    int latency = 0;
    hhxw_get_latency(h, &latency);
    int kernel = 0;
    hhxw_get_linear_kernel_size(h, &kernel);
    std::printf("latency=%d kernel=%d\n", latency, kernel);

    const int frames = 8192;
    float in[frames];
    float out[frames];
    for (int i = 0; i < frames; ++i) {
        in[i] = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0) * 0.5f;
    }

    hhxw_process(h, in, out, frames, 1);

    double sum = 0.0;
    for (int i = 0; i < frames; ++i) sum += out[i] * out[i];
    std::printf("rms=%.6f\n", std::sqrt(sum / frames));

    hhxw_reset(h);
    jhq_destroy(h);
    return 0;
}
