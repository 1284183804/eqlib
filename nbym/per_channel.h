#pragma once
#include <vector>
#include "yyh.h"

namespace eqlib {

template<typename T>
class PerChannel {
    std::vector<T> m_units;

public:
    PerChannel() = default;

    void resize(int n) {
        if (n < 1) n = 1;
        if (static_cast<int>(m_units.size()) != n) {
            m_units.resize(static_cast<std::size_t>(n));
        }
    }

    int size() const {
        return static_cast<int>(m_units.size());
    }

    T& operator[](int ch) {
        return m_units[static_cast<std::size_t>(ch)];
    }

    const T& operator[](int ch) const {
        return m_units[static_cast<std::size_t>(ch)];
    }

    void clear() {
        m_units.clear();
    }
};

}
