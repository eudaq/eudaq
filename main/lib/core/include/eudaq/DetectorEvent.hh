#ifndef EUDAQ_INCLUDED_DetectorEvent
#define EUDAQ_INCLUDED_DetectorEvent

#include <vector>
#include <memory>

#include <Event.hh>

namespace eudaq {
  class RawDataEvent;

  class DLLEXPORT DetectorEvent:public Event {
  public:
    virtual void Serialize(Serializer &) const;
    explicit DetectorEvent(unsigned runnumber, unsigned eventnumber,
                           uint64_t timestamp);

    explicit DetectorEvent(Deserializer &);

    void AddEvent(EventSP evt);
    virtual void Print(std::ostream &os, size_t offset = 0) const;

    size_t NumEvents() const;
    const Event *GetEvent(size_t i) const;
    EventSP GetEventPtr(size_t i) const;
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
    std::vector<EventSP> m_events;
  };
}

#endif
