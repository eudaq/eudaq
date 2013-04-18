#ifndef EUDAQ_INCLUDED_Timer
#define EUDAQ_INCLUDED_Timer

#include "eudaq/Time.hh"

namespace eudaq {

  class Timer {
    public:
      Timer() : m_start(Time::Current()), m_stop(0) {
      }
      void Restart() {
        m_stop = Time(0);
        m_start = Time::Current();
      }
      void Stop() {
        m_stop = Time::Current();
      }
      Time Elapsed() const {
        return StopTime() - m_start;
      }
      double Seconds() const {
        return Elapsed().Seconds();
      }
      double mSeconds() const {
        return 1e3 * Seconds();
      }
      double uSeconds() const {
        return 1e6 * Seconds();
      }
      std::string Formatted(const std::string & format = Time::DEFAULT_FORMAT) const {
        return Elapsed().Formatted(format);
      }
    private:
      Time StopTime() const { return m_stop == Time(0) ? Time::Current() : m_stop; }
      Time m_start, m_stop;
  };

}

#endif // EUDAQ_INCLUDED_Timer
