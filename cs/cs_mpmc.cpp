#include "mpmc.h"
#include <cstdio>
#include <thread>
#include <vector>

int main() {
    eqlib::MpmcQueue<int> q(1024);

    const int producers = 4;
    const int consumers = 4;
    const int per_producer = 1000;

    std::vector<std::thread> threads;
    std::atomic<int> consumed{0};

    for (int p = 0; p < producers; ++p) {
        threads.emplace_back([&q, p, per_producer]() {
            for (int i = 0; i < per_producer; ++i) {
                while (!q.push(p * per_producer + i)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (int c = 0; c < consumers; ++c) {
        threads.emplace_back([&q, &consumed]() {
            int v = 0;
            while (consumed.load() < 4000) {
                if (q.pop(v)) {
                    consumed.fetch_add(1);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : threads) t.join();
    std::printf("consumed=%d size=%zu\n", consumed.load(), q.getSize());

    return 0;
}
