#pragma once
#include "yyh.h"
#include "gglx.h"
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace eqlib {

class EQLIB_INTERNAL MultiChannelProcessor {
    static constexpr int MAX_WORKERS = 64;

    MultiChannelConfig m_config;
    mutable std::mutex m_mutex;

    std::vector<std::thread> m_workers;
    std::condition_variable m_start_cv;
    std::condition_variable m_done_cv;

    std::function<void(int, int)> m_job;
    int      m_job_frames{0};
    int      m_num_active_workers{0};
    int      m_completed{0};
    uint64_t m_generation{0};
    bool     m_stop{false};

    int  computeEffectiveThreadsLocked() const;
    void ensureWorkersLocked(int n);
    void workerLoop(int worker_index, uint64_t initial_generation);
    void startWorkersLocked(int n);
    void stopWorkersLocked();

public:
    MultiChannelProcessor();
    ~MultiChannelProcessor();
    MultiChannelProcessor(const MultiChannelProcessor&) = delete;
    MultiChannelProcessor& operator=(const MultiChannelProcessor&) = delete;
    MultiChannelProcessor(MultiChannelProcessor&&) = delete;
    MultiChannelProcessor& operator=(MultiChannelProcessor&&) = delete;

    void setConfig(const MultiChannelConfig& config);
    MultiChannelConfig getConfig() const;
    void setNumChannels(int ch);
    void setBlockSize(int size);
    void setNumThreads(int n);
    void setEnableMultithread(bool enable);
    int  getNumChannels() const;
    int  getBlockSize() const;
    int  getNumThreads() const;
    bool getEnableMultithread() const;

    int getEffectiveThreads() const;
    int getHardwareThreads() const;
    int getPoolSize() const;

    template<typename Func>
    void processParallel(const float* input, float* output, int frames,
                         int channels, Func&& func) {
        std::unique_lock<std::mutex> lock(m_mutex);
        int n = computeEffectiveThreadsLocked();
        if (n > static_cast<int>(m_workers.size())) {
            ensureWorkersLocked(n);
        }
        if (n > static_cast<int>(m_workers.size())) {
            n = static_cast<int>(m_workers.size());
        }
        if (n <= 1 || frames < m_config.block_size * 2) {
            lock.unlock();
            func(input, output, 0, frames, channels);
            return;
        }
        m_job = [input, output, channels, &func](int start, int end) {
            func(input, output, start, end, channels);
        };
        m_job_frames = frames;
        m_num_active_workers = n;
        m_completed = 0;
        ++m_generation;
        m_start_cv.notify_all();
        m_done_cv.wait(lock, [this] {
            return m_completed >= m_num_active_workers;
        });
        m_job = nullptr;
    }

    template<typename Func>
    void processParallelDouble(const double* input, double* output, int frames,
                               int channels, Func&& func) {
        std::unique_lock<std::mutex> lock(m_mutex);
        int n = computeEffectiveThreadsLocked();
        if (n > static_cast<int>(m_workers.size())) {
            ensureWorkersLocked(n);
        }
        if (n > static_cast<int>(m_workers.size())) {
            n = static_cast<int>(m_workers.size());
        }
        if (n <= 1 || frames < m_config.block_size * 2) {
            lock.unlock();
            func(input, output, 0, frames, channels);
            return;
        }
        m_job = [input, output, channels, &func](int start, int end) {
            func(input, output, start, end, channels);
        };
        m_job_frames = frames;
        m_num_active_workers = n;
        m_completed = 0;
        ++m_generation;
        m_start_cv.notify_all();
        m_done_cv.wait(lock, [this] {
            return m_completed >= m_num_active_workers;
        });
        m_job = nullptr;
    }

    void reset();
};

}
