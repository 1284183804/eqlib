#pragma once
#include <atomic>
#include <cstddef>
#include "gglx.h"

namespace eqlib {

template<std::size_t Capacity = 64>
class ParamQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    ParamChange m_buffer[Capacity];
    std::atomic<std::size_t> m_head{0};
    std::atomic<std::size_t> m_tail{0};

public:
    ParamQueue() = default;

    ParamQueue(const ParamQueue&) = delete;
    ParamQueue& operator=(const ParamQueue&) = delete;

    bool push(const ParamChange& change) {
        std::size_t tail = m_tail.load(std::memory_order_relaxed);
        std::size_t next_tail = (tail + 1) & (Capacity - 1);
        std::size_t head = m_head.load(std::memory_order_acquire);
        if (next_tail == head) return false;
        m_buffer[tail] = change;
        m_tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(ParamChange& out) {
        std::size_t head = m_head.load(std::memory_order_relaxed);
        std::size_t tail = m_tail.load(std::memory_order_acquire);
        if (head == tail) return false;
        out = m_buffer[head];
        m_head.store((head + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    void drain() {
        std::size_t head = m_head.load(std::memory_order_relaxed);
        std::size_t tail = m_tail.load(std::memory_order_relaxed);
        while (head != tail) {
            head = (head + 1) & (Capacity - 1);
        }
        m_head.store(head, std::memory_order_release);
    }

    bool empty() const {
        return m_head.load(std::memory_order_acquire)
            == m_tail.load(std::memory_order_acquire);
    }
};

}
