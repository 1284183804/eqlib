#include <cstdio>
#include <cmath>
#include <vector>
#include "eqlib/eqlib.h"

static int test_format(eqlib_handle* h, int format, double tolerance, const char* label) {
    const char* path = "cs_roundtrip_tmp.wav";
    int sr = 48000;
    int ch = 2;
    int frames = 1000;

    std::vector<float> in(static_cast<std::size_t>(frames) * ch);
    for (int i = 0; i < frames; ++i) {
        in[2 * i]     = 0.5f * (float)std::sin(2.0 * 3.14159265 * 440.0 * i / sr);
        in[2 * i + 1] = 0.5f * (float)std::sin(2.0 * 3.14159265 * 880.0 * i / sr);
    }

    int rc = wjdx_wav_open_write(h, path, sr, ch, format);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] open_write=%d\n", label, rc); return 1; }

    rc = wjdx_wav_write_float(h, in.data(), frames);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] write=%d\n", label, rc); return 1; }

    rc = wjdx_wav_finalize(h);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] finalize=%d\n", label, rc); return 1; }

    wjdx_wav_close(h);

    rc = wjdx_wav_open_read(h, path);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] open_read=%d\n", label, rc); return 1; }

    int r_sr = 0, r_ch = 0, r_bits = 0, r_fmt = 0;
    uint64_t r_frames = 0;
    rc = wjdx_wav_get_info(h, &r_sr, &r_ch, &r_bits, &r_fmt, &r_frames);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] get_info=%d\n", label, rc); return 1; }

    std::printf("  read: sr=%d ch=%d bits=%d frames=%llu\n",
                r_sr, r_ch, r_bits, (unsigned long long)r_frames);

    if (r_sr != sr || r_ch != ch || r_frames != static_cast<uint64_t>(frames)) {
        std::printf("FAIL[%s] metadata mismatch\n", label);
        return 1;
    }

    std::vector<float> out(static_cast<std::size_t>(frames) * ch, 0.0f);
    rc = wjdx_wav_read_float(h, out.data(), frames);
    if (rc != eqlib_ok) { std::printf("FAIL[%s] read=%d\n", label, rc); return 1; }
    wjdx_wav_read_close(h);

    double max_err = 0.0;
    for (std::size_t i = 0; i < out.size(); ++i) {
        double err = std::fabs(static_cast<double>(out[i]) - static_cast<double>(in[i]));
        if (err > max_err) max_err = err;
    }
    std::printf("  max error = %.6f (tolerance %.6f)\n", max_err, tolerance);

    if (max_err > tolerance) {
        std::printf("FAIL[%s] error too large\n", label);
        return 1;
    }
    return 0;
}

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);

    std::printf("float32:\n");
    if (test_format(h, eqlib_wav_float32, 1e-6, "float32") != 0) { jhq_destroy(h); return 1; }

    std::printf("int24:\n");
    if (test_format(h, eqlib_wav_int24, 1e-4, "int24") != 0) { jhq_destroy(h); return 1; }

    std::printf("int16:\n");
    if (test_format(h, eqlib_wav_int16, 1e-3, "int16") != 0) { jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
