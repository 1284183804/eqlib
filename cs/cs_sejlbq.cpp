#include "sejlbq.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::Biquad<double> bq;
    bq.setSampleRate(48000.0);
    bq.setParams(eqlib::FilterType::Peaking, 1000.0, 6.0, 1.0);

    double sum = 0.0;
    for (int i = 0; i < 1024; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = bq.processSample(x);
        sum += std::abs(y);
    }
    std::printf("peaking avg=%.6f\n", sum / 1024.0);

    bq.reset();
    bq.setParams(eqlib::FilterType::LowPass, 2000.0, 0.0, 0.707);
    sum = 0.0;
    for (int i = 0; i < 1024; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 500.0 * i / 48000.0);
        double y = bq.processSample(x);
        sum += std::abs(y);
    }
    std::printf("lowpass avg=%.6f\n", sum / 1024.0);

    bq.reset();
    bq.setParams(eqlib::FilterType::HighPass, 2000.0, 0.0, 0.707);
    sum = 0.0;
    for (int i = 0; i < 1024; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 500.0 * i / 48000.0);
        double y = bq.processSample(x);
        sum += std::abs(y);
    }
    std::printf("highpass avg=%.6f\n", sum / 1024.0);

    bq.reset();
    bq.setParams(eqlib::FilterType::Notch, 1000.0, 0.0, 2.0);
    sum = 0.0;
    for (int i = 0; i < 1024; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = bq.processSample(x);
        sum += std::abs(y);
    }
    std::printf("notch avg=%.6f\n", sum / 1024.0);

    bq.reset();
    bq.setParams(eqlib::FilterType::AllPass, 1000.0, 0.0, 0.707);
    sum = 0.0;
    for (int i = 0; i < 1024; ++i) {
        double x = std::sin(2.0 * 3.14159265358979323846 * 1000.0 * i / 48000.0);
        double y = bq.processSample(x);
        sum += std::abs(y);
    }
    std::printf("allpass avg=%.6f\n", sum / 1024.0);

    return 0;
}
