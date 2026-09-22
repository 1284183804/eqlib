#include "eqlib/eqlib.h"
#include <cstdio>
#include <cstdlib>

static int test_load(const char* path, int format) {
    eqlib_handle* h = nullptr;
    if (jhq_create(&h) != eqlib_ok) return 1;

    int ret = ypdr_load(h, path, format);
    if (ret != eqlib_ok) {
        jhq_destroy(h);
        return 1;
    }

    int sr = 0, ch = 0, fmt = 0;
    uint64_t frames = 0;
    ypdr_get_info(h, &sr, &ch, &frames, &fmt);
    std::printf("format=%d sr=%d ch=%d frames=%llu\n",
                fmt, sr, ch, static_cast<unsigned long long>(frames));

    ypdr_clear(h);
    jhq_destroy(h);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: cs_ypdr <file> <format: 1=wav 2=mp3 3=flac>\n");
        return 0;
    }
    int fmt = std::atoi(argv[2]);
    return test_load(argv[1], fmt);
}
