#include "dtd.h"

namespace eqlib {

MultiChannelProcessor::MultiChannelProcessor() {
    m_config.num_threads = 0;
    startWorkersLocked();
}

MultiChannelProcessor::~MultiChannelProcessor() {
    stopWorkersLocked();
}

void MultiChannelProcessor::startWorkersLocked() {
    unsigned int hw = std::thread::hardware_concurrency();
    int n = (hw == 0) ? 1 : static_cast<int>(hw);
    if (n > 8) n = 8;
    m_workers.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        m_workers.emplace_back([this, i]() { workerLoop(i); });
    }
}

void MultiChannelProcessor::stopWorkersLocked() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
        m_start_cv.notify_all();
    }
    for (auto& th : m_workers) {
        if (th.joinable()) th.join();
    }
    m_workers.clear();
}

void MultiChannelProcessor::workerLoop(int worker_index) {
    uint64_t last_gen = 0;
    while (true) {
        std::function<void(int, int)> job;
        int start = 0;
        int end = 0;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_start_cv.wait(lock, [this, &last_gen] {
                return m_stop || m_generation != last_gen;
            });
            if (m_stop) return;
            last_gen = m_generation;
            if (worker_index >= m_num_active_workers) continue;
            int chunk = m_job_frames / m_num_active_workers;
            if (chunk < 1) chunk = 1;
            start = worker_index * chunk;
            end = (worker_index == m_num_active_workers - 1)
                ? m_job_frames
                : (start + chunk);
            job = m_job;
        }
        if (job) job(start, end);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            ++m_completed;
            if (m_completed >= m_num_active_workers) {
                m_done_cv.notify_one();
            }
        }
    }
}

int MultiChannelProcessor::computeEffectiveThreadsLocked() const {
    if (!m_config.enable_multithread) return 1;
    if (m_config.num_threads > 0) return m_config.num_threads;
    unsigned int hw = std::thread::hardware_concurrency();
    if (hw == 0) return 1;
    int n = static_cast<int>(hw);
    if (n > 8) n = 8;
    return n;
}

void MultiChannelProcessor::setConfig(const MultiChannelConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

MultiChannelConfig MultiChannelProcessor::getConfig() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

void MultiChannelProcessor::setNumChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > WAV_MAX_CHANNELS) ch = WAV_MAX_CHANNELS;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.num_channels = ch;
}

void MultiChannelProcessor::setBlockSize(int size) {
    if (size < 1) size = 1;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.block_size = size;
}

void MultiChannelProcessor::setNumThreads(int n) {
    if (n < 0) n = 0;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.num_threads = n;
}

void MultiChannelProcessor::setEnableMultithread(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config.enable_multithread = enable;
}

int MultiChannelProcessor::getNumChannels() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.num_channels;
}

int MultiChannelProcessor::getBlockSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.block_size;
}

int MultiChannelProcessor::getNumThreads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.num_threads;
}

bool MultiChannelProcessor::getEnableMultithread() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config.enable_multithread;
}

int MultiChannelProcessor::getEffectiveThreads() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return computeEffectiveThreadsLocked();
}

void MultiChannelProcessor::reset() {
}

}
