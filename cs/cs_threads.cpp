#include <cstdio>
#include "eqlib/eqlib.h"

int main() {
    eqlib_handle* h = nullptr;
    jhq_create(&h);

    int hw = 0;
    dtd_get_hardware_threads(h, &hw);
    std::printf("hardware threads = %d\n", hw);
    if (hw < 1) { std::printf("FAIL: hw\n"); jhq_destroy(h); return 1; }

    int n = 0;
    dtd_get_effective_threads(h, &n);
    std::printf("default effective = %d\n", n);
    if (n < 1) { std::printf("FAIL: default\n"); jhq_destroy(h); return 1; }

    dtd_set_num_threads(h, 4);
    dtd_get_effective_threads(h, &n);
    std::printf("after set 4 = %d\n", n);
    if (n != 4) { std::printf("FAIL: set 4\n"); jhq_destroy(h); return 1; }

    dtd_set_num_threads(h, 16);
    dtd_get_effective_threads(h, &n);
    std::printf("after set 16 = %d\n", n);
    if (n != 16) { std::printf("FAIL: set 16\n"); jhq_destroy(h); return 1; }

    dtd_set_num_threads(h, 0);
    dtd_get_effective_threads(h, &n);
    std::printf("after set 0 (auto) = %d\n", n);
    if (n < 1) { std::printf("FAIL: auto\n"); jhq_destroy(h); return 1; }

    dtd_set_enable_multithread(h, 0);
    dtd_get_effective_threads(h, &n);
    std::printf("after disable = %d\n", n);
    if (n != 1) { std::printf("FAIL: disable\n"); jhq_destroy(h); return 1; }

    int rc = dtd_set_num_threads(h, 100);
    if (rc != eqlib_err_param) { std::printf("FAIL: 100 should be rejected\n"); jhq_destroy(h); return 1; }

    jhq_destroy(h);
    std::printf("PASS\n");
    return 0;
}
