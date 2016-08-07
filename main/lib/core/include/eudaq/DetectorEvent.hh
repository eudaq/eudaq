#ifndef EUDAQ_INCLUDED_DetectorEvent
#define EUDAQ_INCLUDED_DetectorEvent

#include <vector>
#include "eudaq/TLUEvent.hh"
#include <memory>

namespace eudaq {


  class RawDataEvent;

  class DLLEXPORT DetectorEvent : public Event {
  public:
    virtual void Serialize(Serializer &) const;
    explicit DetectorEvent(unsigned runnumber, unsigned eventnumber,
                           uint64_t timestamp);

    explicit DetectorEvent(Deserializer &);
    void AddEvent(std::shared_ptr<Event> evt);
    virtual void Print(std::ostream &) const;
    virtual void Print(std::ostream &os,size_t offset) const;
    static event_sp ShallowCopy(const DetectorEvent& det);


    virtual std::string GetType() const;
    

    size_t NumEvents() const;
    Event *GetEvent(size_t i);
    const Event *GetEvent(size_t i) const;
    std::shared_ptr<Event> GetEventPtr(size_t i) const;
    const RawDataEvent &GetRawSubEvent(const std::string &subtype,
                                       int n = 0) const;

    void clearEvents();

    template <typename T> const T *GetSubEvent(int n = 0) const {
      for (size_t i = 0; i < NumEvents(); i++) {
        const T *sev = dynamic_cast<const T *>(GetEvent(i));
        if (sev) {
          if (n > 0) {
            --n;
          } else {
            return sev;
          }
        }
      }
      return 0;
    }


  private:
    std::vector<std::shared_ptr<Event>> m_events;
    DetectorEvent(const DetectorEvent& det);
  };
  using ContainerEvent = DetectorEvent;
  
}

#endif // EUDAQ_INCLUDED_TLUEvent
