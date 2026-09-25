#pragma once
#include "yyh.h"
#include "gglx.h"
#include "ppfx.h"
#include <vector>

namespace eqlib {

class EQLIB_INTERNAL AutoEqCore {
    double m_sample_rate{48000.0};
    int    m_fft_size{2048};
    WindowType m_window{WindowType::Hann};
    double m_pct{100.0};

    int    m_num_bands{DEFAULT_BANDS};
    std::vector<double> m_band_freqs;

    std::vector<double> m_reference_freqs_hz;
    std::vector<double> m_reference_gains_db;
    int    m_reference_points{0};
    bool   m_reference_set{false};

    std::vector<double> m_measured_freqs_hz;
    std::vector<double> m_measured_db;
    int    m_measured_points{0};

    AutoEqOutput m_last_output{};
    bool m_running{false};
    bool m_has_result{false};

public:
    AutoEqCore();
    void setSampleRate(double sr);
    void setFftSize(int size);
    void setWindow(WindowType type);
    void setPct(double pct);

    int  setNumBands(int n);
    int  getNumBands() const;
    int  setBandFreq(int band, double freq_hz);
    int  setBandFreqs(const double* freqs_hz, int n);
    bool getBandFreq(int band, double& out_freq_hz) const;

    bool setReferenceCurve(const double* freqs_hz, const double* gains_db, int num_points);
    void clearReferenceCurve();
    bool hasReferenceCurve() const;
    int  getReferencePointCount() const;
    bool getReferencePoint(int index, double& freq_hz, double& gain_db) const;
    void start();
    void stop();
    bool isRunning() const;
    bool hasResult() const;
    void feedSpectrum(const SpectrumFrame& frame);
    bool computeResult();
    AutoEqOutput getResult() const;
    void reset();
};

}
