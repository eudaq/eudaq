#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
#include <vector>
#include <string>

namespace eudaq {

  class StandardPlane : public Serializable {
  public:
    enum FLAGS { FLAG_ZS = 0x1, // Data are zero suppressed
                 FLAG_NEEDCDS = 0x2, // CDS needs to be calculated (data is 2 or 3 raw frames)
                 FLAG_NEGATIVE = 0x4, // Signal polarity is negative

                 FLAG_WITHPIVOT = 0x10000, // Include before/after pivot boolean per pixel
                 FLAG_WITHSUBMAT = 0x20000, // Include Submatrix ID per pixel
                 FLAG_DIFFCOORDS = 0x40000 // Each frame can have different coordinates (in ZS mode)
    };
    typedef double pixel_t;
    typedef double coord_t;
    StandardPlane(unsigned id, const std::string & type, const std::string & sensor);
    StandardPlane(Deserializer &);
    StandardPlane() : m_id(0), m_tluevent(0), m_xsize(0), m_ysize(0), m_flags(0), m_pivotpixel(0) {}
    void Serialize(Serializer &) const;
    //StandardPlane(size_t pixels = 0, size_t frames = 1);
    void SetSizeRaw(unsigned w, unsigned h, unsigned frames = 1, int flags = 0);
    void SetSizeRaw(unsigned w, unsigned h, int flags) { SetSizeRaw(w, h, 1, flags); }
    void SetSizeZS(unsigned w, unsigned h, unsigned npix, unsigned frames = 1, int flags = 0);

    void SetPixel(unsigned index, unsigned x, unsigned y, unsigned pix, bool pivot = false, unsigned frame = 0);
    void SetPixel(unsigned index, unsigned x, unsigned y, unsigned pix, unsigned frame) { SetPixel(index, x, y, pix, false, frame); }
    void PushPixel(unsigned x, unsigned y, unsigned pix, bool pivot = false, unsigned frame = 0);
    void PushPixel(unsigned x, unsigned y, unsigned pix, unsigned frame) { PushPixel(x, y, pix, false, frame); }

    double GetPixel(unsigned index, unsigned frame = 0) const;
    double GetX(unsigned index, unsigned frame = 0) const;
    double GetY(unsigned index, unsigned frame = 0) const;
    bool GetPivot(unsigned index, unsigned frame = 0) const;
    // defined for short, int, double
    template <typename T>
    std::vector<T> GetPixels() const;
    const std::vector<coord_t> & XVector(unsigned plane = 0) const { return GetPlane(m_x, plane); }
    const std::vector<coord_t> & YVector(unsigned plane = 0) const { return GetPlane(m_y, plane); }
    const std::vector<pixel_t> & PixVector(unsigned plane = 0) const { return GetPlane(m_pix, plane); }

    void SetXSize(unsigned x) { m_xsize = x; }
    void SetYSize(unsigned y) { m_ysize = y; }
    void SetTLUEvent(unsigned ev) { m_tluevent = ev; }
    void SetPivotPixel(unsigned p) { m_pivotpixel = p; }
    void SetFlags(FLAGS flags);

    unsigned ID() const { return m_id; }
    unsigned XSize() const { return m_xsize; }
    unsigned YSize() const { return m_ysize; }
    unsigned NumFrames() const { return m_pix.size(); }
    unsigned TotalPixels() const { return m_xsize * m_ysize; }
    unsigned HitPixels() const { return m_pix[0].size(); } // Doesn't properly handle ZS2 mode
    unsigned TLUEvent() const { return m_tluevent; }
    unsigned PivotPixel() const { return m_pivotpixel; }

    bool IsZS() const { return m_flags & FLAG_ZS; }
    bool NeedsCDS() const { return m_flags & FLAG_NEEDCDS; }
    int  Polarity() const { return m_flags & FLAG_NEGATIVE ? -1 : 1; }

    void Print(std::ostream &) const;
  private: // left public for the moment for compatibility
    const std::vector<pixel_t> & GetPlane(const std::vector<std::vector<pixel_t> > & v, unsigned p) const {
      if (v.size() == 1) return v[0];
      return v[p];
    }
    std::string m_type, m_sensor;
    unsigned m_id, m_tluevent;
    unsigned m_xsize, m_ysize;
    unsigned m_flags, m_pivotpixel;
    std::vector<std::vector<pixel_t> > m_pix;
    std::vector<std::vector<coord_t> > m_x, m_y;
    std::vector<std::vector<bool> > m_pivot;
    std::vector<unsigned> m_mat;
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
