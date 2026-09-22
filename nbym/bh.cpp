#include "bh.h"
#include <cmath>

namespace eqlib {

namespace {

double softClip(double x) {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x - (x * x * x) / 3.0;
}

double tapeSat(double x) {
    return std::tanh(x);
}

double tubeSat(double x) {
    if (x >= 0.0) {
        return 1.0 - std::exp(-x);
    } else {
        return -(1.0 - std::exp(x * 0.5)) * 0.7;
    }
}

double diodeSat(double x) {
    const double k = 2.0;
    if (x >= 0.0) {
        return std::log(1.0 + k * x) / std::log(1.0 + k);
    } else {
        return -std::log(1.0 - k * x) / std::log(1.0 + k) * 0.5;
    }
}

double applySat(SaturationType type, double x) {
    switch (type) {
    case SaturationType::SoftClip: return softClip(x);
    case SaturationType::Tape:     return tapeSat(x);
    case SaturationType::Tube:     return tubeSat(x);
    case SaturationType::Diode:    return diodeSat(x);
    }
    return x;
}

}

SaturationCore::SaturationCore() {
    setDrive(0.0);
    setOutput(0.0);
    setMix(1.0);
}

void SaturationCore::setParams(const SaturationParams& params) {
    m_params = params;
    m_drive_linear = std::pow(10.0, params.drive_db / 20.0);
    m_output_linear = std::pow(10.0, params.output_db / 20.0);
    m_mix = params.mix;
}

SaturationParams SaturationCore::getParams() const {
    return m_params;
}

void SaturationCore::setType(SaturationType type) {
    m_params.type = type;
}

void SaturationCore::setDrive(double drive_db) {
    m_params.drive_db = drive_db;
    m_drive_linear = std::pow(10.0, drive_db / 20.0);
}

void SaturationCore::setMix(double mix) {
    m_params.mix = mix;
    m_mix = mix;
}

void SaturationCore::setOutput(double output_db) {
    m_params.output_db = output_db;
    m_output_linear = std::pow(10.0, output_db / 20.0);
}

void SaturationCore::reset() {
    m_drive_linear = std::pow(10.0, m_params.drive_db / 20.0);
    m_output_linear = std::pow(10.0, m_params.output_db / 20.0);
    m_mix = m_params.mix;
}

double SaturationCore::processSample(double input) const {
    double driven = input * m_drive_linear;
    double saturated = applySat(m_params.type, driven);
    double mixed = input * (1.0 - m_mix) + saturated * m_mix;
    return mixed * m_output_linear;
}

void SaturationCore::processBlock(const double* input, double* output, int frames, int channels) const {
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            output[f * channels + c] = processSample(input[f * channels + c]);
        }
    }
}

void SaturationCore::processBlockFloat(const float* input, float* output, int frames, int channels) const {
    for (int f = 0; f < frames; ++f) {
        for (int c = 0; c < channels; ++c) {
            output[f * channels + c] = static_cast<float>(
                processSample(static_cast<double>(input[f * channels + c])));
        }
    }
}

SaturationType SaturationCore::getType() const { return m_params.type; }
double SaturationCore::getDrive() const { return m_params.drive_db; }
double SaturationCore::getMix() const { return m_params.mix; }
double SaturationCore::getOutput() const { return m_params.output_db; }

}
