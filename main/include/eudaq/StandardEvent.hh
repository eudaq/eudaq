#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
#include <vector>
#include <string>

namespace eudaq {

  class StandardPlane : public Serializable {
  public:
    enum FLAGS { FLAG_ZS = 1, // Data are zero suppressed
                 FLAG_NEEDCDS = 2, // CDS needs to be calculated
                 FLAG_NEGATIVE = 4 // Signal polarity is negative
    };
    StandardPlane(unsigned id, const std::string & type, const std::string & sensor);
    StandardPlane(Deserializer &);
    bool IsZS() const { return m_flags & FLAG_ZS; }
    bool NeedsCDS() const { return m_flags & FLAG_NEEDCDS; }
    int Polarity() const { return m_flags & FLAG_NEGATIVE ? -1 : 1; }
    void Serialize(Serializer &) const;
    typedef double pixel_t;
    typedef double coord_t;
    StandardPlane(size_t pixels = 0, size_t frames = 1);
    double GetPixel(size_t i) const;
    // defined for short, int, double
    template <typename T>
    std::vector<T> GetPixels() const;
    void SetSizeRaw(unsigned w, unsigned h, unsigned frames = 1, bool withpivot = false);
    void SetSizeRaw(unsigned w, unsigned h, bool withpivot) { SetSizeRaw(w, h, 1, withpivot); }
    void SetSizeZS(unsigned w, unsigned h, unsigned npix, unsigned frames = 1, bool withpivot = false);
    void PushPixel(unsigned x, unsigned y, unsigned p);
    void Print(std::ostream &) const;

    std::string m_type, m_sensor;
    unsigned m_id, m_tluevent;
    unsigned m_xsize, m_ysize;
    unsigned m_flags, m_pivotpixel;
    std::vector<std::vector<pixel_t> > m_pix;
    std::vector<coord_t> m_x, m_y;
    std::vector<unsigned> m_mat;
    std::vector<bool> m_pivot;
  };

  class StandardEvent : public Event {
    EUDAQ_DECLARE_EVENT(StandardEvent);
  public:
    StandardEvent(unsigned run = 0, unsigned evnum = 0, unsigned long long timestamp = NOTIMESTAMP);
    StandardEvent(Deserializer &);
    void SetTimestamp(unsigned long long);
    void AddPlane(const StandardPlane &);
    size_t NumPlanes() const;
    const StandardPlane & GetPlane(size_t i) const;
    StandardPlane & GetPlane(size_t i);
    virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream &) const;
  private:
    std::vector<StandardPlane> m_planes;
  };

  inline std::ostream & operator << (std::ostream & os, const StandardPlane & pl) {
    pl.Print(os);
    return os;
  }
  inline std::ostream & operator << (std::ostream & os, const StandardEvent & ev) {
    ev.Print(os);
    return os;
  }

}

#endif // EUDAQ_INCLUDED_StandardEvent
