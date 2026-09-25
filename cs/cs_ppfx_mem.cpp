#include <cstdio>
#include <cmath>
#include "eqlib/eqlib.h"

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);
    jhq_set_sample_rate(h, 48000.0);
    jhq_set_channels(h, 2);

    int rc = ppfx_init(h, 2048, eqlib_window_hann, 0.75);
    if (rc != eqlib_ok) { std::printf("FAIL: init 2048\n"); jhq_destroy(h); return 1; }

    int fft = 0;
    ppfx_get_fft_size(h, &fft);
    std::printf("fft size = %d\n", fft);
    if (fft != 2048) { std::printf("FAIL: fft\n"); jhq_destroy(h); return 1; }

    float mag[4097] = {0};
    float freq[4097] = {0};
    int bins = 0;

    rc = ppfx_get_spectrum(h, mag, freq, &bins);
    std::printf("before feed: rc=%d bins=%d\n", rc, bins);
    if (rc != eqlib_ok) { std::printf("FAIL: expected ok\n"); jhq_destroy(h); return 1; }
    if (bins != 1025) { std::printf("FAIL: bins before feed (expect 1025)\n"); jhq_destroy(h); return 1; }

    double sum_before = 0.0;
    for (int i = 0; i < bins; ++i) sum_before += mag[i];
    std::printf("sum before feed = %.9f (expect 0)\n", sum_before);
    if (sum_before > 1e-9) { std::printf("FAIL: nonzero before feed\n"); jhq_destroy(h); return 1; }

    const int N = 4096;
    float in[4096 * 2] = {0};
    for (int i = 0; i < N; ++i) {
        in[2 * i] = 0.5f * (float)std::sin(2.0 * 3.14159265 * 1000.0 * i / 48000.0);
    }
    ppfx_process(h, in, N, 2);

    rc = ppfx_get_spectrum(h, mag, freq, &bins);
    std::printf("after feed: rc=%d bins=%d\n", rc, bins);
    if (rc != eqlib_ok) { std::printf("FAIL: get_spectrum after feed\n"); jhq_destroy(h); return 1; }
    if (bins != 1025) { std::printf("FAIL: bins after feed\n"); jhq_destroy(h); return 1; }

    double sum_after = 0.0;
    for (int i = 0; i < bins; ++i) sum_after += mag[i];
    std::printf("sum after feed = %.6f (expect > 0)\n", sum_after);
    if (sum_after < 1e-6) { std::printf("FAIL: zero after feed\n"); jhq_destroy(h); return 1; }

    ppfx_init(h, 8192, eqlib_window_blackman, 0.5);
    ppfx_get_fft_size(h, &fft);
    std::printf("after re-init: fft = %d\n", fft);
    if (fft != 8192) { std::printf("FAIL: re-init\n"); jhq_destroy(h); return 1; }

    ppfx_get_spectrum(h, mag, freq, &bins);
    if (bins != 4097) { std::printf("FAIL: bins after re-init (expect 4097)\n"); jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
