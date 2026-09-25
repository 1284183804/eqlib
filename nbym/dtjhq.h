#pragma once
#include "gglx.h"
#include "sejlbq.h"
#include "csdl.h"
#include "per_channel.h"

namespace eqlib {

struct MultiBandState {
    PerChannel<Biquad<double>> filter;
    BandConfig                 cfg;
    PerChannel<double>         envelope;
    double                     current_gain_db{0.0};
    double                     target_gain_db{0.0};
    double                     last_applied_gain_db{0.0};
    bool                       dirty{true};
};

class MultiBandDynEq {
    double          m_sample_rate{48000.0};
    int             m_channels{2};
    int             m_num_bands{7};
    MultiBandState  m_bands[MAX_BANDS];
    ParamQueue<256> m_param_queue;
    double          m_smoothing_coeff{0.001};
    CurveData       m_curve;

    double processSampleInternal(double input, int ch,
                                 bool use_sidechain, double sidechain_input);

public:
    MultiBandDynEq();

    void setSampleRate(double sr);
    void setChannels(int ch);
    double getSampleRate() const;
    int    getChannels() const;

    int setNumBands(int n);
    int getNumBands() const;

    int setBandConfig(int band, const BandConfig& cfg);
    int getBandConfig(int band, BandConfig& out) const;

    int setBandRange(int band, double low_hz, double high_hz);
    int setBandCenter(int band, double center_hz);
    int setBandQ(int band, double q);
    int setBandGain(int band, double gain_db);
    int setBandType(int band, int type);
    int setBandEnable(int band, int enable);

    int setBandTargetDbfs(int band, double target);
    int setBandDynRange(int band, double range_db);
    int setBandDynAttack(int band, double ms);
    int setBandDynRelease(int band, double ms);
    int setBandPercent(int band, double percent);
    int getBandPercent(int band, double& out) const;
    int setBandRatio(int band, double ratio);
    int getBandRatio(int band, double& out) const;
    int setBandMode(int band, int mode);
    int getBandMode(int band, int& out) const;

    int setBandTargetDbfsAll(double target);
    int setBandDynRangeAll(double range_db);
    int setBandDynAttackAll(double ms);
    int setBandDynReleaseAll(double ms);
    int setBandPercentAll(double percent);
    int setBandModeAll(int mode);

    int loadMeasuredCurve(const double* freqs_hz,
                          const double* levels_db,
                          int           num_points,
                          double        reference_db);

    int loadGainCurve(const double* freqs_hz,
                      const double* gains_db,
                      int           num_points,
                      int           source);

    int loadBuiltinCurve();
    int clearCurve();
    int getCurve(CurveData& out) const;
    const CurveData& getCurveRef() const;

    void reset();
    void consumeParamQueue();

    double processSample(double input, int ch);
    void   processBlock(const double* in, double* out, int frames, int channels);
    void   processBlockFloat(const float* in, float* out, int frames, int channels);

    void processBlockWithSidechain(const double* in, const double* sc,
                                   double* out, int frames, int channels);
    void processBlockWithSidechainFloat(const float* in, const float* sc,
                                        float* out, int frames, int channels);

    int autoDistributeBands(double low_hz, double high_hz);
};

}
