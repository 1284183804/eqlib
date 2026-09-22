#include <cstdio>
#include <cmath>
#include <vector>
#include "eqlib/eqlib.h"

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);

    cy_set_channels(h, 1);
    cy_set_input_rate(h, 44100.0);
    cy_set_output_rate(h, 48000.0);
    cy_set_quality(h, 64);

    const int N = 44100;
    std::vector<float> in(N);
    for (int i = 0; i < N; ++i) {
        in[i] = 0.5f * (float)std::sin(2.0 * 3.14159265 * 1000.0 * i / 44100.0);
    }

    int expected = 0;
    cy_get_expected_output(h, N, &expected);

    std::vector<float> out(static_cast<std::size_t>(expected) + 4096, 0.0f);
    int got = 0;
    int rc = cy_process(h, in.data(), N, 1, out.data(),
                        static_cast<int>(out.size()), &got);
    if (rc != eqlib_ok) {
        std::printf("FAIL: cy_process = %d\n", rc);
        jhq_destroy(h);
        return 1;
    }

    int expected_out = static_cast<int>(std::round(N * (48000.0 / 44100.0)));
    std::printf("expected frames ~%d, got %d\n", expected_out, got);

    if (std::abs(got - expected_out) > 200) {
        std::printf("FAIL: frame count out of range\n");
        jhq_destroy(h);
        return 1;
    }

    double rms = 0.0;
    int valid = 0;
    for (int i = 200; i < got - 200; ++i) {
        rms += static_cast<double>(out[i]) * out[i];
        ++valid;
    }
    if (valid <= 0) {
        std::printf("FAIL: not enough output\n");
        jhq_destroy(h);
        return 1;
    }
    rms = std::sqrt(rms / valid);
    std::printf("output RMS = %.6f (expect ~0.3536)\n", rms);

    if (rms < 0.30 || rms > 0.40) {
        std::printf("FAIL: RMS out of range\n");
        jhq_destroy(h);
        return 1;
    }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
