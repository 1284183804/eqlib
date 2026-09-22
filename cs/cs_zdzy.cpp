#include "zdzy.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::AgcCore agc;
    agc.setSampleRate(48000.0);
    agc.setTarget(-18.0);
    agc.setAttack(10.0);
    agc.setRelease(200.0);
    agc.setMaxBoost(12.0);
    agc.setMaxCut(-12.0);
    agc.setGateEnable(true);
    agc.setGateThreshold(-60.0);

    double sum = 0.0;
    for (int i = 0; i < 96000; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 0.01;
        double y = agc.processSample(x, 0);
        sum += y * y;
    }
    std::printf("boosted rms=%.6f\n", std::sqrt(sum / 96000.0));
    std::printf("gain_db=%.2f envelope_db=%.2f\n",
                agc.getCurrentGainDb(), agc.getEnvelopeDb());

    agc.reset();
    sum = 0.0;
    for (int i = 0; i < 96000; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 0.9;
        double y = agc.processSample(x, 0);
        sum += y * y;
    }
    std::printf("cut rms=%.6f\n", std::sqrt(sum / 96000.0));
    std::printf("gain_db=%.2f envelope_db=%.2f\n",
                agc.getCurrentGainDb(), agc.getEnvelopeDb());

    return 0;
}
