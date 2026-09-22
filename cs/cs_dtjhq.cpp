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

    dsjhq_set_num_bands(h, 7);
    dsjhq_auto_distribute(h, 20.0, 20000.0);

    dtjhq_set_band_threshold(h, 0, -20.0);
    dtjhq_set_band_ratio(h, 0, 4.0);
    dtjhq_set_band_attack(h, 0, 5.0);
    dtjhq_set_band_release(h, 0, 100.0);
    dtjhq_set_band_range(h, 0, 12.0);
    dtjhq_set_band_mode(h, 0, eqlib_dyn_downward);
    dtjhq_set_band_type(h, 0, eqlib_filter_peaking);
    dtjhq_set_band_freq(h, 0, 3000.0);
    dtjhq_set_band_gain(h, 0, 0.0);
    dtjhq_set_band_q(h, 0, 1.5);
    dtjhq_set_band_enable(h, 0, 1);

    double g = 0.0;
    double thr = 0.0;
    double atk = 0.0;
    double rel = 0.0;
    double rng = 0.0;
    int mode = 0;
    dtjhq_get_band_gain(h, 0, &g);
    dtjhq_get_band_threshold(h, 0, &thr);
    dtjhq_get_band_attack(h, 0, &atk);
    dtjhq_get_band_release(h, 0, &rel);
    dtjhq_get_band_range(h, 0, &rng);
    dtjhq_get_band_mode(h, 0, &mode);
    std::printf("band0 gain=%.2f threshold=%.2f attack=%.2f release=%.2f range=%.2f mode=%d\n",
                g, thr, atk, rel, rng, mode);

    const int frames = 4096;
    static float in[4096 * 2];
    static float out[4096 * 2];
    for (int i = 0; i < frames * 2; ++i) {
        in[i] = std::sin(2.0 * 3.14159265358979323846 * 3000.0 * i / 48000.0) * 0.9f;
    }

    int ret = dtjhq_process(h, in, out, frames, 2);
    std::printf("dtjhq_process ret=%d out0=%.6f\n", ret, out[0]);

    jhq_destroy(h);
    std::printf("destroy ok\n");
    return 0;
}
