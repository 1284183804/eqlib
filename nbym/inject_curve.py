#!/usr/bin/env python3
import json
import struct
import sys
import ctypes
import os


def load_json(json_path):
    with open(json_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    freqs = []
    levels = []
    for item in data.get("spectrum", []):
        freqs.append(float(item["f"]))
        levels.append(float(item["p"]))
    return freqs, levels


def write_bin(freqs, levels, bin_path):
    n = len(freqs)
    with open(bin_path, "wb") as f:
        f.write(struct.pack("<i", n))
        for i in range(n):
            f.write(struct.pack("<d", freqs[i]))
            f.write(struct.pack("<d", levels[i]))
    return n


def read_bin(bin_path):
    with open(bin_path, "rb") as f:
        raw = f.read(4)
        n = struct.unpack("<i", raw)[0]
        freqs = []
        levels = []
        for i in range(n):
            freqs.append(struct.unpack("<d", f.read(8))[0])
            levels.append(struct.unpack("<d", f.read(8))[0])
    return freqs, levels


def inject_direct(lib_path, bin_path, reference_db=-18.0):
    freqs, levels = read_bin(bin_path)
    n = len(freqs)
    lib = ctypes.CDLL(lib_path)

    h = ctypes.c_void_p()
    ret = lib.jhq_create(ctypes.byref(h))
    if ret != 0:
        print(f"jhq_create failed ret={ret}")
        return ret

    lib.jhq_set_sample_rate(h, ctypes.c_double(48000.0))
    lib.jhq_set_channels(h, 2)

    arr_f = (ctypes.c_double * n)(*freqs)
    arr_l = (ctypes.c_double * n)(*levels)

    ret = lib.dsjhq_set_num_bands(h, 93)
    print(f"set_num_bands ret={ret}")

    ret = lib.dsjhq_auto_distribute(h, ctypes.c_double(20.0), ctypes.c_double(20000.0))
    print(f"auto_distribute ret={ret}")

    ret = lib.dsjhq_load_measured_curve(h, arr_f, arr_l, n, ctypes.c_double(reference_db))
    print(f"load_measured_curve ret={ret} n={n} ref={reference_db}")

    info_np = ctypes.c_int()
    info_src = ctypes.c_int()
    info_meas = ctypes.c_int()
    info_ref = ctypes.c_double()
    ret = lib.dsjhq_get_curve_info(h, ctypes.byref(info_np), ctypes.byref(info_src),
                                   ctypes.byref(info_meas), ctypes.byref(info_ref))
    print(f"curve info ret={ret} points={info_np.value} source={info_src.value} "
          f"measured={info_meas.value} ref={info_ref.value:.2f}")

    f0 = ctypes.c_double()
    l0 = ctypes.c_double()
    g0 = ctypes.c_double()
    lib.dsjhq_get_curve_point(h, 0, ctypes.byref(f0), ctypes.byref(l0), ctypes.byref(g0))
    print(f"point[0] f={f0.value:.2f} level={l0.value:.2f} gain={g0.value:.2f}")

    idx_mid = n // 2
    fm = ctypes.c_double()
    lm = ctypes.c_double()
    gm = ctypes.c_double()
    lib.dsjhq_get_curve_point(h, idx_mid, ctypes.byref(fm), ctypes.byref(lm), ctypes.byref(gm))
    print(f"point[{idx_mid}] f={fm.value:.2f} level={lm.value:.2f} gain={gm.value:.2f}")

    lib.jhq_destroy(h)
    return ret


def inject_and_process(lib_path, input_wav, output_wav, bin_path, reference_db=-18.0):
    freqs, levels = read_bin(bin_path)
    n = len(freqs)
    lib = ctypes.CDLL(lib_path)

    h = ctypes.c_void_p()
    ret = lib.jhq_create(ctypes.byref(h))
    if ret != 0:
        print(f"jhq_create failed ret={ret}")
        return ret

    lib.jhq_set_sample_rate(h, ctypes.c_double(48000.0))
    lib.jhq_set_channels(h, 2)

    arr_f = (ctypes.c_double * n)(*freqs)
    arr_l = (ctypes.c_double * n)(*levels)

    lib.dsjhq_set_num_bands(h, 93)
    lib.dsjhq_auto_distribute(h, ctypes.c_double(20.0), ctypes.c_double(20000.0))
    lib.dsjhq_load_measured_curve(h, arr_f, arr_l, n, ctypes.c_double(reference_db))
    lib.dsjhq_set_band_target_dbfs_all(h, ctypes.c_double(reference_db))
    lib.dsjhq_set_band_dyn_range_all(h, ctypes.c_double(12.0))
    lib.dsjhq_set_band_dyn_attack_all(h, ctypes.c_double(10.0))
    lib.dsjhq_set_band_dyn_release_all(h, ctypes.c_double(200.0))

    inp = input_wav.encode("utf-8")
    outp = output_wav.encode("utf-8")
    ret = lib.ypdc_process_and_save(h, inp, outp, 0)
    print(f"ypdc_process_and_save ret={ret}")

    lib.jhq_destroy(h)
    return ret


def main():
    if len(sys.argv) < 3:
        print("usage:")
        print("  inject_curve.py <json> <bin>                       # 只转换")
        print("  inject_curve.py <json> <bin> <lib.so>              # 转换 + 注入")
        print("  inject_curve.py <json> <bin> <lib.so> <in.wav> <out.wav>  # 转换 + 注入 + 处理")
        return 0

    json_path = sys.argv[1]
    bin_path = sys.argv[2]

    freqs, levels = load_json(json_path)
    n = write_bin(freqs, levels, bin_path)
    print(f"wrote {n} points to {bin_path}")
    print(f"freq range: {freqs[0]:.2f} .. {freqs[-1]:.2f} Hz")
    print(f"level range: {min(levels):.2f} .. {max(levels):.2f} dBFS")

    if len(sys.argv) >= 4:
        lib_path = sys.argv[3]
        if len(sys.argv) >= 6:
            inject_and_process(lib_path, sys.argv[4], sys.argv[5], bin_path)
        else:
            inject_direct(lib_path, bin_path)

    return 0


if __name__ == "__main__":
    sys.exit(main())
