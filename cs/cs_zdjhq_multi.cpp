#include <cstdio>
#include <cmath>
#include <vector>
#include "eqlib/eqlib.h"

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    int rc = zdjhq_set_num_bands(h, 93);
    if (rc != eqlib_ok) { std::printf("FAIL: set_num_bands=%d\n", rc); jhq_destroy(h); return 1; }

    int n = 0;
    zdjhq_get_num_bands(h, &n);
    std::printf("num_bands = %d\n", n);
    if (n != 93) { std::printf("FAIL: get_num_bands\n"); jhq_destroy(h); return 1; }

    std::vector<double> freqs(93);
    for (int i = 0; i < 93; ++i) {
        double t = static_cast<double>(i) / 92.0;
        freqs[static_cast<std::size_t>(i)] = 20.0 * std::pow(1000.0, t);
    }

    rc = zdjhq_set_band_freqs(h, freqs.data(), 93);
    if (rc != eqlib_ok) { std::printf("FAIL: set_band_freqs=%d\n", rc); jhq_destroy(h); return 1; }

    double f0 = 0.0, f92 = 0.0;
    zdjhq_get_band_freq(h, 0, &f0);
    zdjhq_get_band_freq(h, 92, &f92);
    std::printf("band[0]=%.3f band[92]=%.3f\n", f0, f92);
    if (std::fabs(f0 - 20.0) > 1e-6) { std::printf("FAIL: band 0\n"); jhq_destroy(h); return 1; }
    if (std::fabs(f92 - 20000.0) > 1.0) { std::printf("FAIL: band 92\n"); jhq_destroy(h); return 1; }

    std::vector<double> ref_freqs(20001), ref_gains(20001);
    for (int i = 0; i < 20001; ++i) {
        ref_freqs[static_cast<std::size_t>(i)] = 20.0 + 20.0 * i / 20000.0 * 1000.0;
        ref_gains[static_cast<std::size_t>(i)] = 0.0;
    }
    rc = zdjhq_set_reference_curve(h, ref_freqs.data(), ref_gains.data(), 20001);
    if (rc != eqlib_ok) { std::printf("FAIL: set_reference_curve=%d\n", rc); jhq_destroy(h); return 1; }

    int cnt = 0;
    zdjhq_get_reference_point_count(h, &cnt);
    std::printf("reference points = %d\n", cnt);
    if (cnt != 20001) { std::printf("FAIL: ref count\n"); jhq_destroy(h); return 1; }

    zdjhq_start(h);
    zdjhq_feed_spectrum(h);
    zdjhq_compute_result(h);

    double gains[93] = {0};
    int out_n = 0;
    rc = zdjhq_get_result(h, gains, &out_n);
    if (rc != eqlib_ok) { std::printf("FAIL: get_result=%d\n", rc); jhq_destroy(h); return 1; }
    std::printf("result num_bands = %d\n", out_n);
    if (out_n != 93) { std::printf("FAIL: result count\n"); jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
