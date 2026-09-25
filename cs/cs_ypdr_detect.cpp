#include <cstdio>
#include <cstring>
#include "eqlib/eqlib.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("SKIP: no file provided (run with audio path)\n");
        return 0;
    }

    eqlib_handle* h = nullptr;
    jhq_create(&h);

    int rc = ypdr_load(h, argv[1], eqlib_file_unknown);
    std::printf("load rc = %d\n", rc);
    if (rc != eqlib_ok) { jhq_destroy(h); return 1; }

    int sr = 0, ch = 0, fmt = 0;
    uint64_t frames = 0;
    ypdr_get_info(h, &sr, &ch, &frames, &fmt);

    const char* name = "unknown";
    if (fmt == eqlib_file_wav)  name = "WAV";
    if (fmt == eqlib_file_mp3)  name = "MP3";
    if (fmt == eqlib_file_flac) name = "FLAC";
    std::printf("detected: %s, sr=%d ch=%d frames=%llu\n",
                name, sr, ch, (unsigned long long)frames);

    if (fmt == eqlib_file_unknown) { std::printf("FAIL\n"); jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
