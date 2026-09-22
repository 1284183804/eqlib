#include "eqlib/eqlib.h"
#include <cstdio>
#include <cmath>
#include <cstring>

int main() {
    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) {
        std::printf("create failed\n");
        return 1;
    }

    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    int ret = dsjhq_set_num_bands(h, 93);
    std::printf("set_num_bands(93) ret=%d\n", ret);
    if (ret != eqlib_ok) {
        jhq_destroy(h);
        return 1;
    }

    int n = 0;
    dsjhq_get_num_bands(h, &n);
    std::printf("num_bands=%d\n", n);

    ret = dsjhq_auto_distribute(h, 20.0, 20000.0);
    std::printf("auto_distribute ret=%d\n", ret);

    for (int i = 0; i < 3; ++i) {
        double lo = 0.0;
        double hi = 0.0;
        double c = 0.0;
        dsjhq_set_band_range(h, i, 20.0 * std::pow(1.5, i), 20.0 * std::pow(1.5, i + 1));
        dsjhq_set_band_center(h, i, 20.0 * std::pow(1.5, i + 0.5));
        dsjhq_set_band_q(h, i, 0.707);
        dsjhq_set_band_gain(h, i, 0.0);
        dsjhq_set_band_type(h, i, eqlib_filter_peaking);
        dsjhq_set_band_enable(h, i, 1);
        (void)lo;
        (void)hi;
        (void)c;
    }

    dsjhq_set_band_target_dbfs_all(h, -18.0);
    dsjhq_set_band_dyn_range_all(h, 12.0);
    dsjhq_set_band_dyn_attack_all(h, 10.0);
    dsjhq_set_band_dyn_release_all(h, 200.0);

    const int curve_n = 20001;
    static double freqs[20001];
    static double levels[20001];
    for (int i = 0; i < curve_n; ++i) {
        freqs[i] = 20.0 + static_cast<double>(i);
        levels[i] = -50.0 + 10.0 * std::sin(freqs[i] * 0.001);
    }

    ret = dsjhq_load_measured_curve(h, freqs, levels, curve_n, -18.0);
    std::printf("load_measured_curve ret=%d\n", ret);

    int info_n = 0;
    int info_src = 0;
    int info_meas = 0;
    double info_ref = 0.0;
    dsjhq_get_curve_info(h, &info_n, &info_src, &info_meas, &info_ref);
    std::printf("curve info: points=%d source=%d measured=%d ref=%.2f\n",
                info_n, info_src, info_meas, info_ref);

    double f0 = 0.0;
    double l0 = 0.0;
    double g0 = 0.0;
    dsjhq_get_curve_point(h, 0, &f0, &l0, &g0);
    std::printf("point[0] f=%.2f level=%.2f gain=%.2f\n", f0, l0, g0);

    int idx = curve_n / 2;
    double fm = 0.0;
    double lm = 0.0;
    double gm = 0.0;
    dsjhq_get_curve_point(h, idx, &fm, &lm, &gm);
    std::printf("point[%d] f=%.2f level=%.2f gain=%.2f\n", idx, fm, lm, gm);

    const int frames = 1024;
    static float in[1024 * 2];
    static float out[1024 * 2];
    for (int i = 0; i < frames * 2; ++i) {
        in[i] = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0) * 0.5f;
    }

    ret = dsjhq_process(h, in, out, frames, 2);
    std::printf("dsjhq_process ret=%d out0=%.6f\n", ret, out[0]);

    jhq_destroy(h);
    std::printf("destroy ok\n");
    return 0;
}
