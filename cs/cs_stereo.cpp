#include <cstdio>
#include <cmath>
#include "eqlib/eqlib.h"

static int g_failed = 0;

static void check(bool cond, const char* msg) {
    if (!cond) {
        std::printf("FAIL: %s\n", msg);
        ++g_failed;
    }
}

int main() {
    eqlib_handle* h = nullptr;
    check(jhq_create(&h) == eqlib_ok, "create");
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    jhq_set_band_type(h, 0, eqlib_filter_peaking);
    jhq_set_band_freq(h, 0, 1000.0);
    jhq_set_band_gain(h, 0, 12.0);
    jhq_set_band_q(h, 0, 1.0);

    const int N = 4800;
    float in[2 * N]  = {0};
    float out[2 * N] = {0};

    for (int i = 0; i < N; ++i) {
        in[2 * i]     = 0.5f * (float)std::sin(2.0 * 3.14159265 * 1000.0 * i / 48000.0);
        in[2 * i + 1] = 0.0f;
    }

    check(jhq_process(h, in, out, N, 2) == eqlib_ok, "process");

    double l = 0.0, r = 0.0;
    for (int i = 0; i < N; ++i) {
        l += (double)out[2 * i] * out[2 * i];
        r += (double)out[2 * i + 1] * out[2 * i + 1];
    }
    l = std::sqrt(l / N);
    r = std::sqrt(r / N);

    std::printf("L RMS = %.9f\n", l);
    std::printf("R RMS = %.9f\n", r);

    check(l > 0.5, "left channel has signal");
    check(r < 1e-9, "right channel is silent (no crosstalk)");

    jhq_destroy(h);

    if (g_failed > 0) {
        std::printf("%d check(s) failed\n", g_failed);
        return 1;
    }
    std::printf("all checks passed\n");
    return 0;
}
