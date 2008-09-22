#ifndef EUDAQ_INCLUDED_Guard
#define EUDAQ_INCLUDED_Guard

#include <pthread.h>
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  class GuardedBase {
  public:
    GuardedBase() {
      //pthread_mutexattr attr;
      pthread_mutex_init(&m_mutex, 0);
    }

    void Lock() {
      int err = pthread_mutex_lock(&m_mutex);
      if (err) EUDAQ_THROW("Error " + to_string(err) + " locking mutex");
    }
    void Unlock() {
      int err = pthread_mutex_unlock(&m_mutex);
      if (err) EUDAQ_THROW("Error " + to_string(err) + " unlocking mutex");
    }

    class Access {
    public:
      Access(GuardedBase & g) : m_guard(g), m_locked(false) {
        m_guard.Lock();
        m_locked = true;
      }
      void Release() {
        if (m_locked) {
          m_guard.Unlock();
          m_locked = false;
        }
      }
      virtual ~Access() { Release(); }
    protected:
      GuardedBase & m_guard;
      bool m_locked;
    };

  protected:
    pthread_mutex_t m_mutex;
  };


  template <typename T>
  class Guarded : public GuardedBase {
  public:
    Guarded(const T & val) : m_var(val) {
    }

    class Access : public GuardedBase::Access {
    public:
      Access(Guarded & g) : GuardedBase::Access(g), m_var(g.m_var) {
      }
      operator T & () { return m_var; }
      operator const T & () const { return m_var; }
    private:
      T & m_var;
    };
    friend class Access;

    Access get() { return Access(*this); }

  private:
    T m_var;
  };

  template <>
  class Guarded<void> : public GuardedBase {
  public:
    Guarded() {}

    class Access : public GuardedBase::Access {
    public:
      Access(Guarded & g) : GuardedBase::Access(g) {}
    };
    friend class Access;
  };

}

#endif // EUDAQ_INCLUDED_Guard
