#include <cstdio>
#include <cmath>
#include <vector>
#include "eqlib/eqlib.h"

static int test_percent(double percent, double expected_ratio) {
    eqlib_handle* h = nullptr;
    jhq_create(&h);
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 1);

    dsjhq_set_num_bands(h, 1);
    dsjhq_set_band_center(h, 0, 1000.0);
    dsjhq_set_band_q(h, 0, 1.0);
    dsjhq_set_band_gain(h, 0, 0.0);
    dsjhq_set_band_target_dbfs(h, 0, -20.0);
    dsjhq_set_band_dyn_range(h, 0, 80.0);
    dsjhq_set_band_dyn_attack(h, 0, 1.0);
    dsjhq_set_band_dyn_release(h, 0, 50.0);
    dtjhq_set_band_percent(h, 0, percent);
    dtjhq_set_band_mode(h, 0, 0);

    double got_percent = 0.0;
    dtjhq_get_band_percent(h, 0, &got_percent);

    double got_ratio = 0.0;
    dtjhq_get_band_ratio(h, 0, &got_ratio);

    std::printf("percent=%.1f -> ratio=%.3f (expect ~%.3f)\n",
                got_percent, got_ratio, expected_ratio);

    if (std::fabs(got_percent - percent) > 1e-6) {
        std::printf("FAIL: percent mismatch\n");
        jhq_destroy(h);
        return 1;
    }
    if (std::fabs(got_ratio - expected_ratio) > 0.01) {
        std::printf("FAIL: ratio mismatch\n");
        jhq_destroy(h);
        return 1;
    }

    jhq_destroy(h);
    return 0;
}

int main() {
    std::printf("--- percent -> ratio ---\n");
    if (test_percent(0.0, 1.0) != 0) return 1;
    if (test_percent(50.0, 2.0) != 0) return 1;
    if (test_percent(75.0, 4.0) != 0) return 1;

    std::printf("--- ratio -> percent ---\n");
    eqlib_handle* h = nullptr;
    jhq_create(&h);
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 1);
    dsjhq_set_num_bands(h, 1);

    dtjhq_set_band_ratio(h, 0, 4.0);
    double p = 0.0;
    dtjhq_get_band_percent(h, 0, &p);
    std::printf("ratio 4.0 -> percent %.1f (expect 75.0)\n", p);
    if (std::fabs(p - 75.0) > 0.01) { std::printf("FAIL\n"); jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
