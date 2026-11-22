#pragma once
/**
 * @brief Measures the duration of a code section via RAII.
 *
 * On destruction (scope exit), it adds the elapsed duration
 * to the accumulator passed to the constructor.
 *
 * Typical usage:
 *   { ScopedTimer t(my_accumulator); do_something(); }
 *
 */

#include "DurationAccumulator.hh"

#include <chrono>

class ScopedTimer {
public:
    explicit ScopedTimer(DurationAccumulator& acc)
        : m_acc(acc)
        , m_start(std::chrono::steady_clock::now())
    {}

    ~ScopedTimer() {
        m_acc.Add(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - m_start));
    }

    ScopedTimer(const ScopedTimer&)            = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&)                 = delete;
    ScopedTimer& operator=(ScopedTimer&&)      = delete;

private:
    DurationAccumulator&                  m_acc;
    std::chrono::steady_clock::time_point m_start;
};
