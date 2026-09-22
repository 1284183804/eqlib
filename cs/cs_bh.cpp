#include "bh.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::SaturationCore sat;
    sat.setType(eqlib::SaturationType::SoftClip);
    sat.setDrive(12.0);
    sat.setMix(1.0);
    sat.setOutput(-6.0);

    double sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 1.5;
        double y = sat.processSample(x);
        sum += y * y;
    }
    std::printf("softclip rms=%.6f\n", std::sqrt(sum / 4096.0));

    sat.setType(eqlib::SaturationType::Tape);
    sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 1.5;
        double y = sat.processSample(x);
        sum += y * y;
    }
    std::printf("tape rms=%.6f\n", std::sqrt(sum / 4096.0));

    sat.setType(eqlib::SaturationType::Tube);
    sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 1.5;
        double y = sat.processSample(x);
        sum += y * y;
    }
    std::printf("tube rms=%.6f\n", std::sqrt(sum / 4096.0));

    sat.setType(eqlib::SaturationType::Diode);
    sum = 0.0;
    for (int i = 0; i < 4096; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 1.5;
        double y = sat.processSample(x);
        sum += y * y;
    }
    std::printf("diode rms=%.6f\n", std::sqrt(sum / 4096.0));

    std::printf("type=%d drive=%.2f mix=%.2f output=%.2f\n",
                static_cast<int>(sat.getType()), sat.getDrive(),
                sat.getMix(), sat.getOutput());

    return 0;
}
