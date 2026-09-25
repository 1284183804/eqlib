#include "zdjhq.h"
#include "ppfx.h"
#include <cstdio>
#include <cmath>

int main() {
    eqlib::AutoEqCore aeq;
    aeq.setSampleRate(48000.0);
    aeq.setFftSize(2048);
    aeq.setPct(100.0);

    double ref_freqs[7] = {80.0, 200.0, 500.0, 1000.0, 2500.0, 6000.0, 12000.0};
    double ref_gains[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    bool set_ok = aeq.setReferenceCurve(ref_freqs, ref_gains, 7);
    std::printf("set_reference=%d has=%d count=%d\n",
                set_ok ? 1 : 0, aeq.hasReferenceCurve() ? 1 : 0,
                aeq.getReferencePointCount());

    aeq.start();
    std::printf("running=%d\n", aeq.isRunning() ? 1 : 0);

    eqlib::SpectrumFrame frame;
    frame.sample_rate = 48000.0;
    frame.resize(1025);
    for (int i = 0; i < frame.num_bins; ++i) {
        frame.freqs_hz[static_cast<std::size_t>(i)] =
            static_cast<double>(i) * 48000.0 / 2048.0;
        frame.magnitudes[static_cast<std::size_t>(i)] = 0.01;
    }
    aeq.feedSpectrum(frame);
    bool computed = aeq.computeResult();
    std::printf("computed=%d has_result=%d\n",
                computed ? 1 : 0, aeq.hasResult() ? 1 : 0);

    eqlib::AutoEqOutput out = aeq.getResult();
    for (int i = 0; i < eqlib::NUM_BANDS; ++i) {
        std::printf("band=%d gain=%.2f\n", i, out.gains_db[i]);
    }
    std::printf("rms_error=%.4f max_error=%.4f\n",
                out.rms_error_db, out.max_error_db);

    aeq.stop();
    aeq.reset();
    std::printf("stopped reset\n");

    return 0;
}
