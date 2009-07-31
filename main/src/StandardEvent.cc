#include "eudaq/StandardEvent.hh"

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

  StandardPlane::StandardPlane(size_t pixels, size_t frames) 
    : m_pix(frames, std::vector<pixel_t>(pixels)), m_x(pixels), m_y(pixels), m_pivot(pixels)
  {
    //
  }

  void StandardPlane::Print(std::ostream & os) const {
    os << m_id << ", " << m_type << ":" << m_sensor << ", " << m_xsize << "x" << m_ysize << ", " << m_x.size();
  }

  void StandardPlane::SetSizeRaw(unsigned w, unsigned h, unsigned frames, bool withpivot) {
    SetSizeZS(w, h, w*h, frames, withpivot);
    m_flags &= ~FLAG_ZS;
  }

  void StandardPlane::SetSizeZS(unsigned w, unsigned h, unsigned npix, unsigned frames, bool withpivot) {
    m_flags |= FLAG_ZS;
    m_xsize = w;
    m_ysize = h;
    m_x.resize(npix);
    m_y.resize(npix);
    m_pivot.resize(npix*withpivot);
    m_pix.resize(frames);
    for (size_t i = 0; i < frames; ++i) {
      m_pix[i].resize(npix);
    }
  }

  void StandardPlane::PushPixel(unsigned x, unsigned y, unsigned p) {
    m_x.push_back(x);
    m_y.push_back(y);
    m_pix[0].push_back(p);
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

  double StandardPlane::GetPixel(size_t i) const {
    if (m_pix.size() < 1 || i >= m_pix[0].size()) {
      EUDAQ_THROW("Index out of bounds (" + to_string(i) + ")");
    } else if (m_pix.size() == 1 && !NeedsCDS()) {
      return m_pix[0][i] * Polarity();
    } else if (m_pix.size() == 2) {
      if (NeedsCDS()) {
        return (m_pix[1][i] - m_pix[0][i]) * Polarity();
      } else {
        return m_pix[1 - m_pivot[i]][i] * Polarity();
      }
    } else if (m_pix.size() == 3 && NeedsCDS()) {
      return (m_pix[0][i] * (m_pivot[i])
              + m_pix[1][i] * (1-2*m_pivot[i])
              + m_pix[2][i] * (m_pivot[i]-1)
        ) * -Polarity();
    }
    EUDAQ_THROW("Unrecognised pixel format (" + to_string(m_pix.size())
                + " frames, CDS=" + (NeedsCDS() ? "Needed" : "Done") + ")");
  }

  template <typename T>
  std::vector<T> StandardPlane::GetPixels() const {
    std::vector<T> result(m_pix[0].size());
    if (m_pix.size() == 1 && !NeedsCDS()) {
      for (size_t i = 0; i < result.size(); ++i) {
        result[i] = m_pix[0][i] * Polarity();
      }
    } else if (m_pix.size() == 2) {
      if (NeedsCDS()) {
        for (size_t i = 0; i < result.size(); ++i) {
          result[i] = (m_pix[1][i] - m_pix[0][i]) * Polarity();
        }
      } else {
        for (size_t i = 0; i < result.size(); ++i) {
          result[i] = m_pix[1 - m_pivot[i]][i] * Polarity();
        }
      }
      return result;
    } else if (m_pix.size() == 3 && NeedsCDS()) {
      std::vector<T> result(m_pix[0].size());
      for (size_t i = 0; i < result.size(); ++i) {
        result[i] = (m_pix[0][i] * (m_pivot[i])
                     + m_pix[1][i] * (1-2*m_pivot[i])
                     + m_pix[2][i] * (m_pivot[i]-1)
          ) * -Polarity();
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
