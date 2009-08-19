#include "eudaq/StandardEvent.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  EUDAQ_DEFINE_EVENT(StandardEvent, str2id("_STD"));

  StandardPlane::StandardPlane(unsigned id, const std::string & type, const std::string & sensor)
    : m_type(type), m_sensor(sensor), m_id(id), m_tluevent(0), m_xsize(0), m_ysize(0), m_flags(0), m_pivotpixel(0)
  {}

  StandardPlane::StandardPlane(Deserializer & ds) {
    ds.read(m_type);
    ds.read(m_sensor);
    ds.read(m_id);
    ds.read(m_tluevent);
    ds.read(m_xsize);
    ds.read(m_ysize);
    ds.read(m_flags);
    ds.read(m_pivotpixel);
    ds.read(m_pix);
    ds.read(m_x);
    ds.read(m_y);
    ds.read(m_mat);
    ds.read(m_pivot);
  }

  void StandardPlane::Serialize(Serializer & ser) const {
    EUDAQ_THROW("StandardPlane serialization not yet implemented (wait until implementation has stabilised)");
    ser.write(m_type);
    ser.write(m_sensor);
    ser.write(m_id);
    ser.write(m_tluevent);
    ser.write(m_xsize);
    ser.write(m_ysize);
    ser.write(m_flags);
    ser.write(m_pivotpixel);
    ser.write(m_pix);
    ser.write(m_x);
    ser.write(m_y);
    ser.write(m_mat);
    ser.write(m_pivot);
  }

  // StandardPlane::StandardPlane(size_t pixels, size_t frames) 
  //   : m_pix(frames, std::vector<pixel_t>(pixels)), m_x(pixels), m_y(pixels), m_pivot(pixels)
  // {
  //   //
  // }

  void StandardPlane::Print(std::ostream & os) const {
    os << m_id << ", " << m_type << ":" << m_sensor << ", " << m_xsize << "x" << m_ysize << ", " << m_x.size();
  }

  void StandardPlane::SetSizeRaw(unsigned w, unsigned h, unsigned frames, int flags) {
    m_flags |= flags;
    SetSizeZS(w, h, w*h, frames, flags);
    m_flags &= ~FLAG_ZS;
  }

  void StandardPlane::SetSizeZS(unsigned w, unsigned h, unsigned npix, unsigned frames, int flags) {
    m_flags |= flags | FLAG_ZS;
    m_xsize = w;
    m_ysize = h;
    m_pix.resize(frames);
    m_x.resize((m_flags & FLAG_DIFFCOORDS) ? frames : 1);
    m_y.resize((m_flags & FLAG_DIFFCOORDS) ? frames : 1);
    m_pivot.resize((m_flags & FLAG_WITHPIVOT) ? ((m_flags & FLAG_DIFFCOORDS) ? frames : 1) : 0);
    for (size_t i = 0; i < frames; ++i) {
      m_pix[i].resize(npix);
      for (size_t j = 0; j < m_x.size(); ++j) {
        m_x[j].resize(npix);
        m_y[j].resize(npix);
        if (m_flags & FLAG_WITHPIVOT) m_pivot.resize(npix);
      }
    }
  }

  void StandardPlane::PushPixel(unsigned x, unsigned y, unsigned p, bool pivot, unsigned frame) {
    if (frame > m_x.size()) EUDAQ_THROW("Bad frame number " + to_string(frame) + " in PushPixel");
    m_x[frame].push_back(x);
    m_y[frame].push_back(y);
    m_pix[frame].push_back(p);
    if (m_pivot.size()) m_pivot[frame].push_back(pivot);
  }

  void StandardPlane::SetPixel(unsigned index, unsigned x, unsigned y, unsigned pix, bool pivot, unsigned frame) {
    if (frame >= m_pix.size()) EUDAQ_THROW("Bad frame number " + to_string(frame) + " in PushPixel");
    if (frame < m_x.size()) m_x[frame][index] = x;
    if (frame < m_y.size()) m_y[frame][index] = y;
    if (frame < m_pivot.size()) m_pivot[frame][index] = pivot;
    m_pix[frame][index] = pix;
  }

  void StandardPlane::SetFlags(StandardPlane::FLAGS flags) {
    m_flags |= flags;
  }

  StandardEvent::StandardEvent(unsigned run, unsigned evnum, unsigned long long timestamp)
    : Event(run, evnum, timestamp)
  {
  }

  StandardEvent::StandardEvent(Deserializer & ds)
    : Event(ds)
  {
    ds.read(m_planes);
  }

  void StandardEvent::Serialize(Serializer & ser) const {
    EUDAQ_THROW("StandardEvent serialization not yet implemented (wait until implementation has stabilised)");
    Event::Serialize(ser);
    ser.write(m_planes);
  }

  void StandardEvent::SetTimestamp(unsigned long long val) {
    m_timestamp = val;
  }

  void StandardEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_planes.size() << " planes:\n";
    for (size_t i = 0; i < m_planes.size(); ++i) {
      os << "  " << m_planes[i] << "\n";
    }
  }

  size_t StandardEvent::NumPlanes() const {
    return m_planes.size();
  }

  StandardPlane & StandardEvent::GetPlane(size_t i) {
    return m_planes[i];
  }

  const StandardPlane & StandardEvent::GetPlane(size_t i) const {
    return m_planes[i];
  }

  void StandardEvent::AddPlane(const StandardPlane & plane) {
    m_planes.push_back(plane);
  }

  // double StandardPlane::GetPixel(unsigned i) const {
  //   if (m_pix.size() < 1 || i >= m_pix[0].size()) {
  //     EUDAQ_THROW("Index out of bounds (" + to_string(i) + ")");
  //   } else if (m_pix.size() == 1 && !NeedsCDS()) {
  //     return m_pix[0][i] * Polarity();
  //   } else if (m_pix.size() == 2) {
  //     if (NeedsCDS()) {
  //       return (m_pix[1][i] - m_pix[0][i]) * Polarity();
  //     } else {
  //       return m_pix[1 - m_pivot[i]][i] * Polarity();
  //     }
  //   } else if (m_pix.size() == 3 && NeedsCDS()) {
  //     return (m_pix[0][i] * (m_pivot[i])
  //             + m_pix[1][i] * (1-2*m_pivot[i])
  //             + m_pix[2][i] * (m_pivot[i]-1)
  //       ) * -Polarity();
  //   }
  //   EUDAQ_THROW("Unrecognised pixel format (" + to_string(m_pix.size())
  //               + " frames, CDS=" + (NeedsCDS() ? "Needed" : "Done") + ")");
  // }
  double StandardPlane::GetPixel(unsigned index, unsigned frame) const {
    if (frame > m_pix.size()) EUDAQ_THROW("Frame out of range");
    return m_pix[frame][index];
  }
  double StandardPlane::GetX(unsigned index, unsigned frame) const {
    if (frame > m_x.size() && m_x.size() != 1) EUDAQ_THROW("Frame out of range");
    return m_x[m_x.size() == 1 ? 0 : frame][index];
  }
  double StandardPlane::GetY(unsigned index, unsigned frame) const {
    if (frame > m_y.size() && m_y.size() != 1) EUDAQ_THROW("Frame out of range");
    return m_y[m_y.size() == 1 ? 0 : frame][index];
  }
  bool StandardPlane::GetPivot(unsigned index, unsigned frame) const {
    if (frame > m_pivot.size() && m_pivot.size() != 1) EUDAQ_THROW("Frame out of range");
    return m_pivot[m_pivot.size() == 1 ? 0 : frame][index];
  }

  template <typename T>
  std::vector<T> StandardPlane::GetPixels() const {
    std::vector<T> result(m_pix[0].size());
    if (m_pix.size() == 1 && !NeedsCDS()) {
      for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<T>(m_pix[0][i] * Polarity());
      }
    } else if (m_pix.size() == 2) {
      if (NeedsCDS()) {
        for (size_t i = 0; i < result.size(); ++i) {
          result[i] = static_cast<T>((m_pix[1][i] - m_pix[0][i]) * Polarity());
        }
      } else {
        if (m_x.size() == 1) {
          for (size_t i = 0; i < result.size(); ++i) {
            result[i] = static_cast<T>(m_pix[1 - m_pivot[0][i]][i] * Polarity());
          }
        } else {
          EUDAQ_THROW("ZS2 not yet implemented");
        }
      }
      return result;
    } else if (m_pix.size() == 3 && NeedsCDS()) {
      std::vector<T> result(m_pix[0].size());
      for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<T>((m_pix[0][i] * (m_pivot[0][i])
                                    + m_pix[1][i] * (1-2*m_pivot[0][i])
                                    + m_pix[2][i] * (m_pivot[0][i]-1)
                                     ) * -Polarity());
      }
      return result;
    }
    EUDAQ_THROW("Unrecognised pixel format (" + to_string(m_pix.size())
                + " frames, CDS=" + (NeedsCDS() ? "Needed" : "Done") + ")");
  }

  template std::vector<short> StandardPlane::GetPixels<>() const;
  template std::vector<int> StandardPlane::GetPixels<>() const;
  template std::vector<double> StandardPlane::GetPixels<>() const;

}
