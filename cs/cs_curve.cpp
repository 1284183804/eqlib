#include <cstdio>
#include <cmath>
#include <vector>
#include "eqlib/eqlib.h"

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);

    const int N = 101;
    std::vector<double> freqs(N), gains(N);
    for (int i = 0; i < N; ++i) {
        freqs[i] = 20.0 + i * 198.0;
        gains[i] = 0.0;
        if (freqs[i] >= 990.0 && freqs[i] <= 1010.0) gains[i] = 6.0;
    }

    int rc = dsjhq_load_gain_curve(h, freqs.data(), gains.data(), N,
                                   eqlib_curve_injected);
    if (rc != eqlib_ok) {
        std::printf("FAIL: load_gain_curve=%d\n", rc);
        jhq_destroy(h);
        return 1;
    }

    int num_points = 0, source = 0, is_measured = 0;
    double ref_db = 0.0;
    rc = dsjhq_get_curve_info(h, &num_points, &source, &is_measured, &ref_db);
    if (rc != eqlib_ok) {
        std::printf("FAIL: get_curve_info=%d\n", rc);
        jhq_destroy(h);
        return 1;
    }
    std::printf("num_points=%d source=%d is_measured=%d ref=%.3f\n",
                num_points, source, is_measured, ref_db);

    if (num_points != N) {
        std::printf("FAIL: num_points\n");
        jhq_destroy(h);
        return 1;
    }
    if (source != eqlib_curve_injected) {
        std::printf("FAIL: source\n");
        jhq_destroy(h);
        return 1;
    }

    int idx_1k = -1;
    double best = 1e9;
    for (int i = 0; i < num_points; ++i) {
        double f = 0, l = 0, g = 0;
        dsjhq_get_curve_point(h, i, &f, &l, &g);
        double d = std::fabs(f - 1000.0);
        if (d < best) { best = d; idx_1k = i; }
    }
    if (idx_1k < 0) {
        std::printf("FAIL: no point near 1k\n");
        jhq_destroy(h);
        return 1;
    }

    double f = 0, l = 0, g = 0;
    dsjhq_get_curve_point(h, idx_1k, &f, &l, &g);
    std::printf("at f=%.1f: gain=%.4f (expect 6.0)\n", f, g);

    if (std::fabs(g - 6.0) > 1e-6) {
        std::printf("FAIL: gain mismatch\n");
        jhq_destroy(h);
        return 1;
    }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
