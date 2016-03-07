#ifndef EUDAQ_INCLUDED_DetectorEvent
#define EUDAQ_INCLUDED_DetectorEvent

#include <vector>
#include "eudaq/TLUEvent.hh"
#include <memory>

namespace eudaq {

  class RawDataEvent;

  class DLLEXPORT DetectorEvent : public Event {
    EUDAQ_DECLARE_EVENT(DetectorEvent);

  public:
    virtual void Serialize(Serializer &) const;
    explicit DetectorEvent(unsigned runnumber, unsigned eventnumber,
                           uint64_t timestamp)
        : Event(runnumber, eventnumber, timestamp) {}
    //     explicit DetectorEvent(const TLUEvent & tluev) :
    //       Event(tluev.GetRunNumber(), tluev.GetEventNumber(),
    //       tluev.GetTimestamp())
    //       {}
    explicit DetectorEvent(Deserializer &);
    void AddEvent(std::shared_ptr<Event> &evt);
    virtual void Print(std::ostream &) const;

    /// Return "DetectorEvent" as type.
    virtual std::string GetType() const { return "DetectorEvent"; }

    size_t NumEvents() const { return m_events.size(); }
    Event *GetEvent(size_t i) { return m_events[i].get(); }
    const Event *GetEvent(size_t i) const { return m_events[i].get(); }
    std::shared_ptr<Event> GetEventPtr(size_t i) const { return m_events[i]; }
    const RawDataEvent &GetRawSubEvent(const std::string &subtype,
                                       int n = 0) const;
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
  };
}

#endif // EUDAQ_INCLUDED_TLUEvent
