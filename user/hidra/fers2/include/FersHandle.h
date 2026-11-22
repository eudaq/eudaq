// RAII wrapper for FERS device handles
#ifndef HIDRA_FERS2_FERSHANDLE_H_
#define HIDRA_FERS2_FERSHANDLE_H_

#include <string>

#include "FersException.h"
#include "FERSlib.h"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
 
#include <chrono>
#include <thread>
#include <cstdio>

namespace hidra {
namespace fers2 {

class FersHandle {
 public:
  FersHandle() noexcept : m_handle(-1) {}
  explicit FersHandle(int h) noexcept : m_handle(h) {}

  // Open device by path; throws FersError on failure.
  explicit FersHandle(const std::string& path) {
    int h = -1;
    int ret = FERS_OpenDevice(const_cast<char*>(path.c_str()), &h);
    if (ret != 0) {
      throw FersError("FERS_OpenDevice failed for '" + path + "'", ret);
    }
    m_handle = h;
  }

  // Non-copyable, movable
  FersHandle(const FersHandle&) = delete;
  FersHandle& operator=(const FersHandle&) = delete;

  FersHandle(FersHandle&& other) noexcept : m_handle(other.m_handle) { other.m_handle = -1; }
  FersHandle& operator=(FersHandle&& other) noexcept {
    reset();
    m_handle = other.m_handle;
    other.m_handle = -1;
    return *this;
  }

  ~FersHandle() noexcept {
    reset();
  }

  int get() const noexcept { return m_handle; }
  explicit operator bool() const noexcept { return m_handle >= 0; }

  void reset() noexcept {
    if (m_handle >= 0) {
      const int max_attempts = 3;
      for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        int ret = FERS_CloseDevice(m_handle);
        if (ret == 0) {
          break;
        }
        FERS_LibMsg(const_cast<char*>("[WARNING][CNC %02d] FERS_CloseDevice attempt %d failed (ret=%d)\n"),
                   FERS_INDEX(m_handle), attempt, ret);
        if (attempt < max_attempts) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      }
      m_handle = -1;
    }
  }

 private:
  int m_handle = -1;
};

}  // namespace fers2
}  // namespace hidra

#endif  // HIDRA_FERS2_FERSHANDLE_H_
