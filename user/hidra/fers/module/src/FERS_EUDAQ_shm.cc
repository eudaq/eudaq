#include "FERS_EUDAQ_shm.h"

std::chrono::time_point<std::chrono::high_resolution_clock> get_midnight_today() {
    // Get the current time using system_clock
    auto now = std::chrono::system_clock::now();

    // Convert to time_t to manipulate the time
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Extract the date part
    std::tm* now_tm = std::localtime(&now_c);
    now_tm->tm_hour = 0;
    now_tm->tm_min = 0;
    now_tm->tm_sec = 0;

    // Convert back to time_t
    std::time_t midnight_c = std::mktime(now_tm);

    // Convert back to std::chrono::system_clock::time_point
    auto midnight = std::chrono::system_clock::from_time_t(midnight_c);

    // Convert to high_resolution_clock::time_point
    auto midnight_high_res = std::chrono::time_point_cast<std::chrono::high_resolution_clock::duration>(midnight);

    return midnight_high_res;
}


