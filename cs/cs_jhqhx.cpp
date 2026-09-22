#include "jhqhx.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::EqCore core;
    core.setSampleRate(48000.0);
    core.setChannels(1);

    core.setBandType(0, eqlib::FilterType::Peaking);
    core.setBandFreq(0, 1000.0);
    core.setBandQ(0, 1.0);
    core.setBandGain(0, 6.0);
    core.setBandEnable(0, true);

    for (int i = 1; i < eqlib::NUM_BANDS; ++i) {
        core.setBandEnable(i, false);
    }

    for (int b = 0; b < eqlib::NUM_BANDS; ++b) {
        double g = core.getBandGain(b);
        int t = static_cast<int>(core.getBandType(b));
        double f = core.getBandFreq(b);
        double q = core.getBandQ(b);
        bool e = core.getBandEnable(b);
        std::printf("band=%d type=%d freq=%.2f q=%.4f gain=%.2f enable=%d\n",
                    b, t, f, q, g, e ? 1 : 0);
    }

    double sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = core.processSample(x, 0);
        sum += y * y;
    }
    std::printf("rms=%.6f\n", std::sqrt(sum / 4096.0));

    core.reset();
    std::printf("reset ok\n");

    core.setBandGain(0, -12.0);
    core.consumeParamQueue();
    std::printf("after change gain=%.2f\n", core.getBandGain(0));

    return 0;
}
