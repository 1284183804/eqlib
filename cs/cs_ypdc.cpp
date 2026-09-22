#include "eqlib/eqlib.h"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: cs_ypdc <input> <output>\n");
        return 0;
    }

    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) return 1;

    jhq_set_band_type(h, 0, eqlib_filter_peaking);
    jhq_set_band_freq(h, 0, 3000.0);
    jhq_set_band_q(h, 0, 1.5);
    jhq_set_band_gain(h, 0, -4.0);

    bh_set_type(h, eqlib_sat_tape);
    bh_set_drive(h, 6.0);
    bh_set_mix(h, 0.5);

    zdzy_set_target(h, -18.0);
    zdzy_set_attack(h, 10.0);
    zdzy_set_release(h, 200.0);

    int ret = ypdc_process_and_save(h, argv[1], argv[2], eqlib_wav_float32);
    std::printf("result=%d\n", ret);

    jhq_destroy(h);
    return ret == eqlib_ok ? 0 : 1;
}
