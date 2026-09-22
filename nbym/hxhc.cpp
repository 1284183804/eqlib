#include "hxhc.h"
#include <algorithm>
#include <cstring>

namespace eqlib {

template<typename T>
RingBuffer<T>::RingBuffer(std::size_t capacity) {
    setCapacity(capacity);
}

template<typename T>
RingBuffer<T>::~RingBuffer() {
    delete[] m_buffer;
}

template<typename T>
RingBuffer<T>::RingBuffer(RingBuffer&& other) noexcept
    : m_buffer(other.m_buffer),
      m_capacity(other.m_capacity),
      m_read(other.m_read),
      m_write(other.m_write),
      m_available(other.m_available) {
    other.m_buffer = nullptr;
    other.m_capacity = 0;
    other.m_read = 0;
    other.m_write = 0;
    other.m_available = 0;
}

template<typename T>
RingBuffer<T>& RingBuffer<T>::operator=(RingBuffer&& other) noexcept {
    if (this != &other) {
        delete[] m_buffer;
        m_buffer = other.m_buffer;
        m_capacity = other.m_capacity;
        m_read = other.m_read;
        m_write = other.m_write;
        m_available = other.m_available;
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_read = 0;
        other.m_write = 0;
        other.m_available = 0;
    }
    return *this;
}

template<typename T>
void RingBuffer<T>::setCapacity(std::size_t capacity) {
    if (capacity < 2) capacity = 2;
    std::size_t p = 1;
    while (p < capacity) p <<= 1;
    capacity = p;
    if (capacity == m_capacity && m_buffer) {
        clear();
        return;
    }
    delete[] m_buffer;
    m_buffer = new T[capacity];
    m_capacity = capacity;
    m_read = 0;
    m_write = 0;
    m_available = 0;
}

template<typename T>
std::size_t RingBuffer<T>::getCapacity() const {
    return m_capacity;
}

template<typename T>
std::size_t RingBuffer<T>::getReadable() const {
    return m_available;
}

template<typename T>
std::size_t RingBuffer<T>::getWritable() const {
    return m_capacity - m_available;
}

template<typename T>
void RingBuffer<T>::clear() {
    m_read = 0;
    m_write = 0;
    m_available = 0;
}

template<typename T>
std::size_t RingBuffer<T>::write(const T* data, std::size_t count) {
    if (!data || count == 0 || m_capacity == 0) return 0;
    std::size_t writable = m_capacity - m_available;
    if (count > writable) count = writable;
    if (count == 0) return 0;

    std::size_t first = std::min(count, m_capacity - m_write);
    std::memcpy(m_buffer + m_write, data, first * sizeof(T));
    std::size_t second = count - first;
    if (second > 0) {
        std::memcpy(m_buffer, data + first, second * sizeof(T));
    }
    m_write = (m_write + count) & (m_capacity - 1);
    m_available += count;
    return count;
}

template<typename T>
std::size_t RingBuffer<T>::read(T* out, std::size_t count) {
    if (!out || count == 0 || m_available == 0) return 0;
    if (count > m_available) count = m_available;

    std::size_t first = std::min(count, m_capacity - m_read);
    std::memcpy(out, m_buffer + m_read, first * sizeof(T));
    std::size_t second = count - first;
    if (second > 0) {
        std::memcpy(out + first, m_buffer, second * sizeof(T));
    }
    m_read = (m_read + count) & (m_capacity - 1);
    m_available -= count;
    return count;
}

template<typename T>
std::size_t RingBuffer<T>::peek(T* out, std::size_t count) const {
    if (!out || count == 0 || m_available == 0) return 0;
    if (count > m_available) count = m_available;

    std::size_t first = std::min(count, m_capacity - m_read);
    std::memcpy(out, m_buffer + m_read, first * sizeof(T));
    std::size_t second = count - first;
    if (second > 0) {
        std::memcpy(out + first, m_buffer, second * sizeof(T));
    }
    return count;
}

template<typename T>
std::size_t RingBuffer<T>::skip(std::size_t count) {
    if (count > m_available) count = m_available;
    m_read = (m_read + count) & (m_capacity - 1);
    m_available -= count;
    return count;
}

template class RingBuffer<float>;
template class RingBuffer<double>;

StreamBuffer::StreamBuffer() {
    setCapacityFrames(m_capacity_frames);
    setBlockSize(m_block_size);
    setChannels(m_channels);
}

StreamBuffer::StreamBuffer(int block_size, int channels) {
    setCapacityFrames(m_capacity_frames);
    setBlockSize(block_size);
    setChannels(channels);
}

StreamBuffer::~StreamBuffer() = default;

void StreamBuffer::setBlockSize(int size) {
    if (size < 1) size = 1;
    m_block_size = size;
    m_block.assign(static_cast<std::size_t>(m_block_size) * m_channels, 0.0f);
    m_block_d.assign(static_cast<std::size_t>(m_block_size) * m_channels, 0.0);
}

void StreamBuffer::setChannels(int ch) {
    if (ch < 1) ch = 1;
    if (ch > 64) ch = 64;
    if (ch == m_channels) return;
    m_channels = ch;
    m_block.assign(static_cast<std::size_t>(m_block_size) * m_channels, 0.0f);
    m_block_d.assign(static_cast<std::size_t>(m_block_size) * m_channels, 0.0);
    setCapacityFrames(m_capacity_frames);
}

int StreamBuffer::getBlockSize() const {
    return m_block_size;
}

int StreamBuffer::getChannels() const {
    return m_channels;
}

void StreamBuffer::setCapacityFrames(int frames) {
    if (frames < 64) frames = 64;
    m_capacity_frames = frames;
    std::size_t cap = static_cast<std::size_t>(frames)
                    * static_cast<std::size_t>(m_channels);
    m_input.setCapacity(cap);
    m_output.setCapacity(cap);
    m_input_d.setCapacity(cap);
    m_output_d.setCapacity(cap);
}

int StreamBuffer::getCapacityFrames() const {
    return m_capacity_frames;
}

void StreamBuffer::clear() {
    m_input.clear();
    m_output.clear();
    m_input_d.clear();
    m_output_d.clear();
}

std::size_t StreamBuffer::pushInput(const float* data, std::size_t frames) {
    if (!data) return 0;
    std::size_t count = frames * static_cast<std::size_t>(m_channels);
    std::size_t wrote = m_input.write(data, count);
    return wrote / static_cast<std::size_t>(m_channels);
}

std::size_t StreamBuffer::pushInputDouble(const double* data, std::size_t frames) {
    if (!data) return 0;
    std::size_t count = frames * static_cast<std::size_t>(m_channels);
    std::size_t wrote = m_input_d.write(data, count);
    return wrote / static_cast<std::size_t>(m_channels);
}

std::size_t StreamBuffer::popOutput(float* data, std::size_t frames) {
    if (!data) return 0;
    std::size_t count = frames * static_cast<std::size_t>(m_channels);
    std::size_t got = m_output.read(data, count);
    return got / static_cast<std::size_t>(m_channels);
}

std::size_t StreamBuffer::popOutputDouble(double* data, std::size_t frames) {
    if (!data) return 0;
    std::size_t count = frames * static_cast<std::size_t>(m_channels);
    std::size_t got = m_output_d.read(data, count);
    return got / static_cast<std::size_t>(m_channels);
}

std::size_t StreamBuffer::getInputAvailable() const {
    return m_input.getReadable() / static_cast<std::size_t>(m_channels);
}

std::size_t StreamBuffer::getOutputAvailable() const {
    return m_output.getReadable() / static_cast<std::size_t>(m_channels);
}

}
