#pragma once
#include "yyh.h"
#include "gglx.h"

namespace eqlib {

class EQLIB_INTERNAL MidSideProcessor {
    MidSideMode m_mode{MidSideMode::Stereo};
    double m_mid_gain_db{0.0};
    double m_side_gain_db{0.0};
    double m_mid_gain_linear{1.0};
    double m_side_gain_linear{1.0};

public:
    MidSideProcessor();
    void setMode(MidSideMode mode);
    void setMidGain(double db);
    void setSideGain(double db);
    void reset();
    void process(double in_l, double in_r, double& out_l, double& out_r);
    MidSideMode getMode() const;
    double getMidGain() const;
    double getSideGain() const;
};

}
