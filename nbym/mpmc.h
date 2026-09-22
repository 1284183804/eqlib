#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <vector>

namespace eqlib {

template<typename T>
class MpmcQueue {
public:
    explicit MpmcQueue(std::size_t capacity = 1024) {
        setCapacity(capacity);
    }

    ~MpmcQueue() = default;
    MpmcQueue(const MpmcQueue&) = delete;
    MpmcQueue& operator=(const MpmcQueue&) = delete;
    MpmcQueue(MpmcQueue&&) = delete;
    MpmcQueue& operator=(MpmcQueue&&) = delete;

    void setCapacity(std::size_t capacity) {
        if (capacity < 2) capacity = 2;
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.resize(capacity);
        m_capacity = capacity;
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

    std::size_t getCapacity() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_capacity;
    }

    std::size_t getSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size == 0;
    }

    bool push(const T& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_size >= m_capacity) return false;
            m_buffer[m_tail] = value;
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
        }
        m_cv.notify_one();
        return true;
    }

    bool push(T&& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_size >= m_capacity) return false;
            m_buffer[m_tail] = std::move(value);
            m_tail = (m_tail + 1) % m_capacity;
            ++m_size;
        }
        m_cv.notify_one();
        return true;
    }

    bool pop(T& out) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_size == 0) return false;
        out = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) % m_capacity;
        --m_size;
        return true;
    }

    bool waitPop(T& out) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() { return m_size > 0; });
        out = std::move(m_buffer[m_head]);
        m_head = (m_head + 1) % m_capacity;
        --m_size;
        return true;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

private:
    std::vector<T> m_buffer;
    std::size_t m_capacity{0};
    std::size_t m_head{0};
    std::size_t m_tail{0};
    std::size_t m_size{0};
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
};

}
