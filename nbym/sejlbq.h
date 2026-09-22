#pragma once
#include <cmath>
#include "gglx.h"

namespace eqlib {

template<typename T>
class Biquad {
    T m_b0{1}, m_b1{0}, m_b2{0}, m_a1{0}, m_a2{0};
    T m_x1{0}, m_x2{0}, m_y1{0}, m_y2{0};
    double m_sample_rate{48000.0};
    FilterType m_type{FilterType::Peaking};
    double m_freq_hz{1000.0};
    double m_gain_db{0.0};
    double m_q{0.707};

public:
    Biquad() = default;

    void setSampleRate(double sr) {
        m_sample_rate = sr;
    }

    void setParams(FilterType type, double freq_hz, double gain_db, double q) {
        m_type = type;
        m_freq_hz = freq_hz;
        m_gain_db = gain_db;
        m_q = q;
        calcCoeffs();
    }

    void reset() {
        m_x1 = m_x2 = m_y1 = m_y2 = T(0);
    }

    T processSample(T input) {
        T output = m_b0 * input + m_b1 * m_x1 + m_b2 * m_x2 - m_a1 * m_y1 - m_a2 * m_y2;
        m_x2 = m_x1;
        m_x1 = input;
        m_y2 = m_y1;
        m_y1 = output;
        return output;
    }

    void calcCoeffs() {
        const double pi = 3.14159265358979323846;
        const double w0 = 2.0 * pi * m_freq_hz / m_sample_rate;
        const double cos_w0 = std::cos(w0);
        const double sin_w0 = std::sin(w0);
        const double alpha = sin_w0 / (2.0 * m_q);
        const double A = std::pow(10.0, m_gain_db / 40.0);

        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

        switch (m_type) {
        case FilterType::Peaking:
            b0 = 1.0 + alpha * A;
            b1 = -2.0 * cos_w0;
            b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha / A;
            break;
        case FilterType::LowShelf: {
            const double sqrtA = std::sqrt(A);
            const double two_sqrtA_alpha = 2.0 * sqrtA * alpha;
            b0 = A * ((A + 1.0) - (A - 1.0) * cos_w0 + two_sqrtA_alpha);
            b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cos_w0);
            b2 = A * ((A + 1.0) - (A - 1.0) * cos_w0 - two_sqrtA_alpha);
            a0 = (A + 1.0) + (A - 1.0) * cos_w0 + two_sqrtA_alpha;
            a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cos_w0);
            a2 = (A + 1.0) + (A - 1.0) * cos_w0 - two_sqrtA_alpha;
            break;
        }
        case FilterType::HighShelf: {
            const double sqrtA = std::sqrt(A);
            const double two_sqrtA_alpha = 2.0 * sqrtA * alpha;
            b0 = A * ((A + 1.0) + (A - 1.0) * cos_w0 + two_sqrtA_alpha);
            b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cos_w0);
            b2 = A * ((A + 1.0) + (A - 1.0) * cos_w0 - two_sqrtA_alpha);
            a0 = (A + 1.0) - (A - 1.0) * cos_w0 + two_sqrtA_alpha;
            a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cos_w0);
            a2 = (A + 1.0) - (A - 1.0) * cos_w0 - two_sqrtA_alpha;
            break;
        }
        case FilterType::LowPass:
            b0 = (1.0 - cos_w0) / 2.0;
            b1 = 1.0 - cos_w0;
            b2 = (1.0 - cos_w0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::HighPass:
            b0 = (1.0 + cos_w0) / 2.0;
            b1 = -(1.0 + cos_w0);
            b2 = (1.0 + cos_w0) / 2.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::BandPass:
            b0 = alpha;
            b1 = 0.0;
            b2 = -alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::Notch:
            b0 = 1.0;
            b1 = -2.0 * cos_w0;
            b2 = 1.0;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha;
            break;
        case FilterType::AllPass:
            b0 = 1.0 - alpha;
            b1 = -2.0 * cos_w0;
            b2 = 1.0 + alpha;
            a0 = 1.0 + alpha;
            a1 = -2.0 * cos_w0;
            a2 = 1.0 - alpha;
            break;
        }

        m_b0 = static_cast<T>(b0 / a0);
        m_b1 = static_cast<T>(b1 / a0);
        m_b2 = static_cast<T>(b2 / a0);
        m_a1 = static_cast<T>(a1 / a0);
        m_a2 = static_cast<T>(a2 / a0);
    }
};

}
