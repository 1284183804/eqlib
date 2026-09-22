#include "eqlib/eq.hpp"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::EQ eq(48000.0, 2);

    if (!eq.isValid()) {
        std::printf("eq invalid error=%d\n", eq.getLastError());
        return 1;
    }

    std::printf("version=%s\n", eq.getVersion());

    eq.setBandType(0, eqlib_filter_peaking);
    eq.setBandFreq(0, 1000.0);
    eq.setBandGain(0, 6.0);
    eq.setBandQ(0, 1.0);
    eq.setBandEnable(0, true);

    eq.setSatType(eqlib_sat_tape);
    eq.setSatDrive(6.0);
    eq.setSatMix(0.5);

    eq.setAgcTarget(-18.0);
    eq.setAgcAttack(10.0);
    eq.setAgcRelease(200.0);

    const int frames = 1024;
    float data[frames * 2];
    for (int i = 0; i < frames * 2; ++i) {
        data[i] = std::sin(2.0 * 3.14159265358979323846 * 440.0 * i / 48000.0) * 0.5f;
    }

    eq.process(data, frames);
    std::printf("processed out0=%.6f err=%d\n", data[0], eq.getLastError());

    eq.reset();
    std::printf("reset ok\n");

    return 0;
}
