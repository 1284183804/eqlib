#include "zc.h"
#include <cmath>

namespace eqlib {

MidSideProcessor::MidSideProcessor() = default;

void MidSideProcessor::setMode(MidSideMode mode) {
    m_mode = mode;
}

void MidSideProcessor::setMidGain(double db) {
    m_mid_gain_db = db;
    m_mid_gain_linear = std::pow(10.0, db / 20.0);
}

void MidSideProcessor::setSideGain(double db) {
    m_side_gain_db = db;
    m_side_gain_linear = std::pow(10.0, db / 20.0);
}

void MidSideProcessor::reset() {
    m_mid_gain_linear = std::pow(10.0, m_mid_gain_db / 20.0);
    m_side_gain_linear = std::pow(10.0, m_side_gain_db / 20.0);
}

void MidSideProcessor::process(double in_l, double in_r, double& out_l, double& out_r) {
    switch (m_mode) {
    case MidSideMode::Stereo:
        out_l = in_l;
        out_r = in_r;
        break;
    case MidSideMode::MidOnly: {
        double mid = (in_l + in_r) * 0.5;
        out_l = mid;
        out_r = mid;
        break;
    }
    case MidSideMode::SideOnly: {
        double side = (in_l - in_r) * 0.5;
        out_l = side;
        out_r = -side;
        break;
    }
    case MidSideMode::MidSide: {
        double mid = (in_l + in_r) * 0.5;
        double side = (in_l - in_r) * 0.5;
        mid *= m_mid_gain_linear;
        side *= m_side_gain_linear;
        out_l = mid + side;
        out_r = mid - side;
        break;
    }
    }
}

MidSideMode MidSideProcessor::getMode() const { return m_mode; }
double MidSideProcessor::getMidGain() const { return m_mid_gain_db; }
double MidSideProcessor::getSideGain() const { return m_side_gain_db; }

}
