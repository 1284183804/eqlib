#pragma once
#include "yyh.h"
#include "gglx.h"

namespace eqlib {

class EQLIB_INTERNAL SaturationCore {
    SaturationParams m_params;
    double m_drive_linear{1.0};
    double m_output_linear{1.0};
    double m_mix{1.0};

public:
    SaturationCore();
    void setParams(const SaturationParams& params);
    SaturationParams getParams() const;
    void setType(SaturationType type);
    void setDrive(double drive_db);
    void setMix(double mix);
    void setOutput(double output_db);
    void reset();
    double processSample(double input) const;
    void processBlock(const double* input, double* output, int frames, int channels) const;
    void processBlockFloat(const float* input, float* output, int frames, int channels) const;
    SaturationType getType() const;
    double getDrive() const;
    double getMix() const;
    double getOutput() const;
};

}
