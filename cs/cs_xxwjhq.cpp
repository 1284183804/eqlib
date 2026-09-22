#include "xxwjhq.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::LinearPhaseEq lpeq;
    lpeq.setSampleRate(48000.0);
    lpeq.setChannels(1);
    lpeq.setKernelSize(512);

    std::printf("kernel=%d latency=%d\n",
                lpeq.getKernelSize(), lpeq.getLatency());

    lpeq.setBandType(0, eqlib::FilterType::Peaking);
    lpeq.setBandFreq(0, 1000.0);
    lpeq.setBandQ(0, 1.0);
    lpeq.setBandGain(0, 6.0);
    lpeq.setBandEnable(0, true);

    for (int i = 1; i < eqlib::NUM_BANDS; ++i) {
        lpeq.setBandEnable(i, false);
    }

    lpeq.rebuild();

    double sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = lpeq.processSample(0, x);
        sum += y * y;
    }
    std::printf("rms=%.6f\n", std::sqrt(sum / 4096.0));

    lpeq.reset();
    std::printf("reset ok\n");

    lpeq.setBandGain(0, -6.0);
    lpeq.rebuild();
    sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = lpeq.processSample(0, x);
        sum += y * y;
    }
    std::printf("rms=%.6f\n", std::sqrt(sum / 4096.0));

    return 0;
}
