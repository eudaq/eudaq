#include "eudaq/Mutex.hh"
#include "eudaq/Exception.hh"

#include <mutex>

namespace eudaq {

  class Mutex::Impl {
  public:
    Impl() {}
    ~Impl() {}

    std::recursive_mutex m_mutex;
  };

  Mutex::Mutex() : m_impl(new Mutex::Impl) {}

  Mutex::~Mutex() { delete m_impl; }

  int Mutex::Lock() {
    m_impl->m_mutex.lock();
    return 0;
  }

  int Mutex::TryLock() {
    if (m_impl->m_mutex.try_lock()) {
      return 0;
    }
    return 1;
  }

  int Mutex::UnLock() {
    m_impl->m_mutex.unlock();
    return 0;
  }

  MutexLock::MutexLock(Mutex &m, bool lock) : m_mutex(m), m_locked(lock) {
    if (lock && m_mutex.TryLock()) {
      EUDAQ_THROW("Unable to lock mutex");
    }
  }

  void MutexLock::Lock() {
    if (m_mutex.TryLock()) {
      EUDAQ_THROW("Unable to lock mutex");
    }
    m_locked = true;
  }

  void MutexLock::Release() {
    if (m_locked)
      m_mutex.UnLock();
    m_locked = false;
  }

  MutexLock::~MutexLock() { Release(); }

  //   MutexTryLock::MutexTryLock(Mutex & m) : m_mutex(m), m_locked(true) {
  //     if (m_mutex.TryLock()) {
  //       m_locked = false;
  //     }
  //   }

  //   void MutexTryLock::Release() {
  //     m_mutex.UnLock();
  //     m_locked = false;
  //   }

  //   MutexTryLock::~MutexTryLock() {
  //     if (m_locked) Release();
  //   }

  //   MutexTryLock::operator bool () const {
  //     return m_locked;
  //   }
}
