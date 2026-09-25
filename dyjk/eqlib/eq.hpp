#pragma once
#include "eqlib.h"
#include <memory>

namespace eqlib {

class EQLIB_API EQ {
public:
    EQ();
    explicit EQ(double sample_rate, int channels = 2);
    ~EQ();
    EQ(const EQ&) = delete;
    EQ& operator=(const EQ&) = delete;
    EQ(EQ&&) noexcept;
    EQ& operator=(EQ&&) noexcept;

    bool isValid() const;
    int getLastError() const;

    void setSampleRate(double sr);
    double getSampleRate() const;
    void setChannels(int ch);
    int getChannels() const;
    void reset();
    void clearAll();

    void setNumBands(int n);
    int getNumBands() const;
    void setBandType(int band, int type);
    void setBandFreq(int band, double freq_hz);
    void setBandFreqs(const double* freqs_hz, int n);
    void setBandGain(int band, double gain_db);
    void setBandQ(int band, double q);
    void setBandEnable(int band, bool enable);
    int getBandType(int band) const;
    double getBandFreq(int band) const;
    double getBandGain(int band) const;
    double getBandQ(int band) const;
    bool getBandEnable(int band) const;

    void setDynThreshold(int band, double threshold_db);
    void setDynRatio(int band, double ratio);
    void setDynPercent(int band, double percent);
    void setDynAttack(int band, double attack_ms);
    void setDynRelease(int band, double release_ms);
    void setDynRange(int band, double range_db);
    void setDynMode(int band, int mode);

    void setSatType(int type);
    void setSatDrive(double drive_db);
    void setSatMix(double mix);
    void setSatOutput(double output_db);

    void setAgcTarget(double target_dbfs);
    void setAgcAttack(double attack_ms);
    void setAgcRelease(double release_ms);
    void setAgcMaxBoost(double max_boost_db);
    void setAgcMaxCut(double max_cut_db);

    void setLinearKernelSize(int size);
    int getLinearKernelSize() const;
    int getLatency() const;
    void setBandPhaseMode(int band, int mode);

    void setMultiBandNumBands(int n);
    int getMultiBandNumBands() const;
    void autoDistributeBands(double low_hz, double high_hz);
    void setMultiBandRange(int band, double low_hz, double high_hz);
    void setMultiBandCenter(int band, double center_hz);
    void setMultiBandGain(int band, double gain_db);
    void setMultiBandQ(int band, double q);
    void setMultiBandType(int band, int type);
    void setMultiBandEnable(int band, bool enable);
    void setMultiBandTargetDbfsAll(double target);
    void setMultiBandDynRangeAll(double range_db);
    void setMultiBandDynAttackAll(double ms);
    void setMultiBandDynReleaseAll(double ms);
    int loadMeasuredCurve(const double* freqs_hz, const double* levels_db,
                          int num_points, double reference_db);
    int loadGainCurve(const double* freqs_hz, const double* gains_db,
                      int num_points, int source);
    int loadBuiltinCurve();
    int clearCurve();
    int getCurveInfo(int* num_points, int* source, int* is_measured, double* reference_db) const;
    int getCurvePoint(int index, double* freq_hz, double* level_db, double* gain_db) const;

    void setMidSideMode(int mode);
    void setMidGain(double db);
    void setSideGain(double db);

    void setSidechainEnable(bool enable);
    void setSidechainAttack(double ms);
    void setSidechainRelease(double ms);
    void setSidechainAmount(double amount);
    void setSidechainTarget(double db);
    void setSidechainRange(double db);

    void setResampleChannels(int ch);
    void setResampleInputRate(double rate);
    void setResampleOutputRate(double rate);
    void setResampleQuality(int taps);
    int getResampleLatency() const;

    void process(float* data, int frames);
    void processDouble(double* data, int frames);
    void processFull(float* data, int frames);
    void processFullDouble(double* data, int frames);

    int processResample(const float* in, int in_frames,
                        float* out, int out_capacity, int* out_frames);
    int processResampleDouble(const double* in, int in_frames,
                              double* out, int out_capacity, int* out_frames);

    void processFile(const char* input_path, const char* output_path, int output_format);

    const char* getVersion() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

}
