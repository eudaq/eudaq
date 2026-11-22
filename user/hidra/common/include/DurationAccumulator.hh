#pragma once
/**
 * @brief Accumulates durations and computes the average at the end of a run.
 *
 * Design:
 * - Completely independent from the rest of the system: no dependency on
 *   EUDAQ, ROOT, or any other project class. Usable anywhere.
 * - Thread safety: Add() is not thread-safe. Callers invoking Add() from
 *   multiple threads must synchronize externally. In the current setup each
 *   accumulator is written by a single thread (DoReceive or the pump), so no
 *   additional synchronization is needed.
 * - The name is part of the accumulator to simplify logging: callers do not
 *   need to maintain a separate name-to-accumulator map.
 *
 * Disabling telemetry:
 * - Comment out ScopedTimer lines in the classes that use them. This header
 *   and DurationAccumulator can remain included with no runtime overhead
 *   (no threads, no allocations, no locks).
 */

#include <chrono>
#include <string>
#include <string_view>
#include <cstdint>

class DurationAccumulator {
public:
    explicit DurationAccumulator(std::string name) : m_name(std::move(name)) {}

    /**
     * @brief Adds a duration sample to the accumulator.
     *
     * Not thread-safe: call from a single thread per accumulator, or
     * synchronize externally.
     */
    void Add(std::chrono::nanoseconds duration) {
        m_total_ns += duration.count();
        ++m_count;
    }

    /** @brief Returns the number of accumulated samples. */
    uint64_t Count() const { return m_count; }

    /** @brief Returns the average in milliseconds, or 0.0 if no samples were added. */
    double MeanMs() const {
        if (m_count == 0) return 0.0;
        return static_cast<double>(m_total_ns) / static_cast<double>(m_count) * 1e-6;
    }

    /** @brief Returns the descriptive name used in logs. */
    std::string_view Name() const { return m_name; }

    /** @brief Resets the accumulator state (useful for mid-run resets). */
    void Reset() {
        m_total_ns = 0;
        m_count    = 0;
    }

    /** @brief Returns a log-ready summary string, e.g. "decode_xdc: 0.042 ms (1234 samples)". */
    std::string Summary() const {
        return m_name + ": "
            + std::to_string(MeanMs()) + " ms avg"
            + " (" + std::to_string(m_count) + " samples)";
    }

private:
    std::string m_name;
    uint64_t    m_total_ns{0};
    uint64_t    m_count{0};
};
