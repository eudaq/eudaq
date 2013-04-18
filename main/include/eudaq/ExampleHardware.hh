#ifndef EUDAQ_INCLUDED_ExampleHardware
#define EUDAQ_INCLUDED_ExampleHardware

#include "eudaq/Timer.hh"
#include <vector>

namespace eudaq {

  class ExampleHardware {
    public:
      ExampleHardware();
      void Setup(int);
      void PrepareForRun();
      bool EventsPending() const;
      unsigned NumSensors() const;
      std::vector<unsigned char> ReadSensor(int sensorid);
      void CompletedEvent();
    private:
      unsigned short m_numsensors, m_width, m_height, m_triggerid;
      eudaq::Timer m_timer;
      double m_nextevent;
  };

} // namespace eudaq

#endif // EUDAQ_INCLUDED_ExampleHardware
