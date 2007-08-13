#ifndef EUDAQ_INCLUDED_pixel_SparseEvent
#define EUDAQ_INCLUDED_pixel_SparseEvent

#include <vector>
#include "eudaq/Event.hh"

namespace eudaq {

  struct SparseHit {
    unsigned short x, y, d;
  };

  class SparseEvent : public Event {
    EUDAQ_DECLARE_EVENT(SparseEvent);
  public:
    virtual void Serialize(Serializer &) const;
    SparseEvent(unsigned run, unsigned event, const std::vector<SparseHit> hits) :
      Event(run, event),
      m_hits(hits) {}
    SparseEvent(unsigned run, unsigned event,
                const std::vector<unsigned short> x,
                const std::vector<unsigned short> y,
                const std::vector<unsigned short> d) :
      Event(run, event),
      m_hits(make_hits(x, y, d)) {}
    static std::vector<SparseHit> make_hits(const std::vector<unsigned short> x,
                                            const std::vector<unsigned short> y,
                                            const std::vector<unsigned short> d);
    virtual void Print(std::ostream &) const;
  private:
    std::vector<SparseHit> m_hits;
  };

}

#endif // EUDAQ_INCLUDED_pixel_SparseEvent
