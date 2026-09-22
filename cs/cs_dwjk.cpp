#include "eqlib/eqlib.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) {
        std::printf("create failed\n");
        return 1;
    }

    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    jhq_set_band_type(h, 0, eqlib_filter_peaking);
    jhq_set_band_freq(h, 0, 1000.0);
    jhq_set_band_q(h, 0, 1.0);
    jhq_set_band_gain(h, 0, 6.0);
    jhq_set_band_enable(h, 0, 1);

    double g = 0.0;
    int t = 0;
    jhq_get_band_gain(h, 0, &g);
    jhq_get_band_type(h, 0, &t);
    std::printf("band0 type=%d gain=%.2f\n", t, g);

    const int frames = 1024;
    const int ch = 2;
    float in[frames * ch];
    float out[frames * ch];
    for (int i = 0; i < frames * ch; ++i) {
        in[i] = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0) * 0.5f;
    }

    int ret = jhq_process(h, in, out, frames, ch);
    std::printf("process ret=%d out0=%.6f\n", ret, out[0]);

    dtjhq_set_band_threshold(h, 0, -20.0);
    dtjhq_set_band_ratio(h, 0, 4.0);
    dtjhq_set_band_attack(h, 0, 5.0);
    dtjhq_set_band_release(h, 0, 100.0);
    dtjhq_set_band_range(h, 0, 12.0);
    dtjhq_set_band_mode(h, 0, eqlib_dyn_downward);
    dtjhq_process(h, in, out, frames, ch);

    bh_set_type(h, eqlib_sat_tape);
    bh_set_drive(h, 6.0);
    bh_set_mix(h, 0.5);
    bh_process(h, out, out, frames, ch);

    zdzy_set_target(h, -18.0);
    zdzy_set_attack(h, 10.0);
    zdzy_set_release(h, 200.0);
    zdzy_process(h, out, out, frames, ch);

    zc_set_mode(h, eqlib_ms_mid_side);
    zc_set_mid_gain(h, 0.0);
    zc_set_side_gain(h, 3.0);
    zc_process(h, out, out, out, out, frames);

    char ver[64];
    jhq_get_version(ver, 64);
    std::printf("version=%s\n", ver);

    jhq_destroy(h);
    std::printf("destroy ok\n");

    return 0;
}
