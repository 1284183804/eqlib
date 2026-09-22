#include "ppfx.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::SpectrumAnalyzer sa;
    eqlib::SpectrumConfig cfg;
    cfg.fft_size = 2048;
    cfg.window = eqlib::WindowType::Hann;
    cfg.overlap = 0.75;

    bool init_ok = sa.init(48000.0, 2, cfg);
    std::printf("init=%d fft=%d hop=%d\n",
                init_ok ? 1 : 0, sa.getFftSize(), sa.getHopSize());

    sa.setSmoothing(0.5);

    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        sa.processSample(x, 0);
    }

    eqlib::SpectrumFrame frame;
    bool got = sa.getFrame(frame);
    std::printf("got_frame=%d num_bins=%d\n", got ? 1 : 0, frame.num_bins);

    if (got) {
        for (int i = 0; i < 5; ++i) {
            std::printf("bin=%d freq=%.2f mag=%.6f\n",
                        i, frame.freqs_hz[i], frame.magnitudes[i]);
        }
    }

    sa.reset();
    std::printf("reset ok\n");

    return 0;
}
