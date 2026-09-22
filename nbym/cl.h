#pragma once
#include "yyh.h"
#include "gglx.h"
#include "per_channel.h"

namespace eqlib {

class EQLIB_INTERNAL SidechainProcessor {
    double             m_sample_rate{48000.0};
    int                m_channels{2};
    PerChannel<double> m_envelope;
    double             m_attack_coeff{0.0};
    double             m_release_coeff{0.0};
    double             m_amount{1.0};
    double             m_target_db{-18.0};
    double             m_range_db{12.0};
    bool               m_enable{false};

public:
    SidechainProcessor();
    void setSampleRate(double sr);
    void setChannels(int ch);
    int  getChannels() const;
    void setAttack(double ms);
    void setRelease(double ms);
    void setAmount(double amount);
    void setTarget(double target_db);
    void setRange(double range_db);
    void setEnable(bool enable);
    void reset();
    double process(double sidechain_input, int ch);
    double getEnvelope() const;
    double getEnable() const;
    double getAmount() const;
    double getAttack() const;
    double getRelease() const;
    double getTarget() const;
    double getRange() const;
};

}
