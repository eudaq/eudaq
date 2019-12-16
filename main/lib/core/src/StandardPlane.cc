#include "eudaq/StandardPlane.hh"

namespace eudaq{
  StandardPlane::StandardPlane()
    : m_id(0), m_xsize(0), m_ysize(0), m_flags(0),
      m_pivotpixel(0), m_result_pix(0), m_result_x(0), m_result_y(0) {}

  StandardPlane::StandardPlane(uint32_t id, const std::string &type,
			       const std::string &sensor)
    : m_type(type), m_sensor(sensor), m_id(id), m_xsize(0),
      m_ysize(0), m_flags(0), m_pivotpixel(0), m_result_pix(0), m_result_x(0),
      m_result_y(0) {}

  StandardPlane::StandardPlane(Deserializer &ds)
    : m_result_pix(0), m_result_x(0), m_result_y(0) {
    ds.read(m_type);
    ds.read(m_sensor);
    ds.read(m_id);
    ds.read(m_xsize);
    ds.read(m_ysize);
    ds.read(m_flags);
    ds.read(m_pivotpixel);
    ds.read(m_pix);
    ds.read(m_x);
    ds.read(m_y);
    ds.read(m_pivot);
    ds.read(m_mat);
    ds.read(m_time);
  }

  void StandardPlane::Serialize(Serializer &ser) const {
    ser.write(m_type);
    ser.write(m_sensor);
    ser.write(m_id);
    ser.write(m_xsize);
    ser.write(m_ysize);
    ser.write(m_flags);
    ser.write(m_pivotpixel);
    ser.write(m_pix);
    ser.write(m_x);
    ser.write(m_y);
    ser.write(m_pivot);
    ser.write(m_mat);
    ser.write(m_time);
  }


  void StandardPlane::Print(std::ostream & os) const {
    Print(os, 0);
  }

  void StandardPlane::Print(std::ostream &os, size_t offset) const  {
    os << std::string(offset, ' ') << m_id << ", " << m_type << ":" << m_sensor << ", "
       << m_xsize << "x" << m_ysize << "x" << m_pix.size()
       << " (" << (m_pix.size() ? m_pix[0].size() : 0) << "), pivot=" << m_pivotpixel
       << " timestamps:" << m_time[0].size()<< std::endl; }
  void StandardPlane::SetSizeRaw(uint32_t w, uint32_t h, uint32_t frames,
				 int flags) {

    m_flags = flags;
    SetSizeZS(w, h, w * h, frames, flags);
    m_flags &= ~FLAG_ZS;
  }

  void StandardPlane::SetSizeZS(uint32_t w, uint32_t h, uint32_t npix,
				uint32_t frames, int flags) {
    m_flags = flags | FLAG_ZS;
    // std::cout << "DBG flags " << hexdec(m_flags) << std::endl;
    m_xsize = w;
    m_ysize = h;
    m_pix.resize(frames);
    m_time.resize(GetFlags(FLAG_DIFFCOORDS) ? frames : 1);
    m_x.resize(GetFlags(FLAG_DIFFCOORDS) ? frames : 1);
    m_y.resize(GetFlags(FLAG_DIFFCOORDS) ? frames : 1);
    m_pivot.resize(GetFlags(FLAG_WITHPIVOT)
		   ? (GetFlags(FLAG_DIFFCOORDS) ? frames : 1)
		   : 0);
    for (size_t i = 0; i < frames; ++i) {
      m_pix[i].resize(npix);
    }
    for (size_t i = 0; i < m_x.size(); ++i) {
      m_x[i].resize(npix);
      m_y[i].resize(npix);
      m_time[i].resize(npix);
      if (m_pivot.size())
	m_pivot[i].resize(npix);
    }
  }

  void StandardPlane::PushPixelHelper(uint32_t x, uint32_t y, double p, uint64_t time_ps,
				      bool pivot, uint32_t frame) {
    if (frame > m_x.size())
      EUDAQ_THROW("Bad frame number " + to_string(frame) + " in PushPixel");
    m_x[frame].push_back(x);
    m_y[frame].push_back(y);
    m_pix[frame].push_back(p);
    m_time[frame].push_back(time_ps);
    if (m_pivot.size())
      m_pivot[frame].push_back(pivot);
    // std::cout << "DBG: " << frame << ", " << x << ", " << y << ", " << p <<
    // ";" << m_pix[0].size() << ", " << m_pivot.size() << std::endl;
  }

  void StandardPlane::SetPixelHelper(uint32_t index, uint32_t x, uint32_t y,
				     double pix, uint64_t time_ps, bool pivot, uint32_t frame) {
    if (frame >= m_pix.size())
      EUDAQ_THROW("Bad frame number " + to_string(frame) + " in SetPixel");
    if (frame < m_x.size()) {
      m_x.at(frame).at(index) = x;
    }
    if (frame < m_y.size()) {
      m_y.at(frame).at(index) = y;
    }
    if (frame < m_pivot.size()) {
      m_pivot.at(frame).at(index) = pivot;
    }
    if (frame < m_pix.size()) {
      m_pix.at(frame).at(index) = pix;
    }
    if (frame < m_time.size()) {
      m_time.at(frame).at(index) = time_ps;
    }
  }

  void StandardPlane::SetFlags(StandardPlane::FLAGS flags) { m_flags |= flags; }

  double StandardPlane::GetPixel(uint32_t index, uint32_t frame) const {
    return m_pix.at(frame).at(index);
  }
  double StandardPlane::GetPixel(uint32_t index) const {
    SetupResult();
    return m_result_pix->at(index);
  }
  uint64_t StandardPlane::GetTimestamp(uint32_t index, uint32_t frame) const {
      if (!GetFlags(FLAG_DIFFCOORDS))
        frame = 0;
      return m_time.at(frame).at(index);
  }
  uint64_t StandardPlane::GetTimestamp(uint32_t index) const {
    SetupResult();
    return m_result_time->at(index);
  }
  double StandardPlane::GetX(uint32_t index, uint32_t frame) const {
    if (!GetFlags(FLAG_DIFFCOORDS))
      frame = 0;
    return m_x.at(frame).at(index);
  }
  double StandardPlane::GetX(uint32_t index) const {
    SetupResult();
    return m_result_x->at(index);
  }
  double StandardPlane::GetY(uint32_t index, uint32_t frame) const {
    if (!GetFlags(FLAG_DIFFCOORDS))
      frame = 0;
    return m_y.at(frame).at(index);
  }
  double StandardPlane::GetY(uint32_t index) const {
    SetupResult();
    return m_result_y->at(index);
  }
  bool StandardPlane::GetPivot(uint32_t index, uint32_t frame) const {
    if (!GetFlags(FLAG_DIFFCOORDS))
      frame = 0;
    return m_pivot.at(frame).at(index);
  }

  void StandardPlane::SetPivot(uint32_t index, uint32_t frame, bool PivotFlag) {
    m_pivot.at(frame).at(index) = PivotFlag;
  }

  const std::vector<StandardPlane::coord_t> &
  StandardPlane::XVector(uint32_t frame) const {
    return GetFrame(m_x, frame);
  }

  const std::vector<StandardPlane::coord_t> &StandardPlane::XVector() const {
    SetupResult();
    return *m_result_x;
  }

  const std::vector<StandardPlane::coord_t> &
  StandardPlane::YVector(uint32_t frame) const {
    return GetFrame(m_y, frame);
  }

  const std::vector<StandardPlane::coord_t> &StandardPlane::YVector() const {
    SetupResult();
    return *m_result_y;
  }

  const std::vector<StandardPlane::pixel_t> &
  StandardPlane::PixVector(uint32_t frame) const {
    return GetFrame(m_pix, frame);
  }

  const std::vector<StandardPlane::pixel_t> &StandardPlane::PixVector() const {
    SetupResult();
    return *m_result_pix;
  }

  void StandardPlane::SetXSize(uint32_t x) { m_xsize = x; }

  void StandardPlane::SetYSize(uint32_t y) { m_ysize = y; }

  void StandardPlane::SetPivotPixel(uint32_t p) { m_pivotpixel = p; }

  uint32_t StandardPlane::ID() const { return m_id; }

  const std::string &StandardPlane::Type() const { return m_type; }

  const std::string &StandardPlane::Sensor() const { return m_sensor; }

  uint32_t StandardPlane::XSize() const { return m_xsize; }

  uint32_t StandardPlane::YSize() const { return m_ysize; }

  uint32_t StandardPlane::NumFrames() const { return m_pix.size(); }

  uint32_t StandardPlane::TotalPixels() const { return m_xsize * m_ysize; }

  uint32_t StandardPlane::HitPixels(uint32_t frame) const {
    return GetFrame(m_pix, frame).size();
  }

  uint32_t StandardPlane::HitPixels() const {
    SetupResult();
    return m_result_pix->size();
  }

  uint32_t StandardPlane::PivotPixel() const { return m_pivotpixel; }

  int StandardPlane::GetFlags(int f) const { return m_flags & f; }

  bool StandardPlane::NeedsCDS() const { return GetFlags(FLAG_NEEDCDS) != 0; }

  int StandardPlane::Polarity() const {
    return GetFlags(FLAG_NEGATIVE) ? -1 : 1;
  }

  const std::vector<StandardPlane::pixel_t> &
  StandardPlane::GetFrame(const std::vector<std::vector<pixel_t>> &v,
			  uint32_t f) const {
    return v.at(f);
  }

  void StandardPlane::SetupResult() const {
    if (m_result_pix)
      return;
    m_result_x = &m_x[0];
    m_result_y = &m_y[0];
    m_result_time = &m_time[0];
    if (GetFlags(FLAG_ACCUMULATE)) {
      m_temp_pix.resize(0);
      m_temp_x.resize(0);
      m_temp_y.resize(0);
      m_temp_time.resize(0);
      for (size_t f = 0; f < m_pix.size(); ++f) {
        for (size_t p = 0; p < m_pix[f].size(); ++p) {
          m_temp_x.push_back(GetX(p, f));
          m_temp_y.push_back(GetY(p, f));
          m_temp_pix.push_back(GetPixel(p, f));
          m_temp_time.push_back(GetTimestamp(p,f));
        }
      }
      m_result_x = &m_temp_x;
      m_result_y = &m_temp_y;
      m_result_pix = &m_temp_pix;
      m_result_time = & m_temp_time;
    } else if (m_pix.size() == 1 && !GetFlags(FLAG_NEEDCDS)) {
      m_result_pix = &m_pix[0];
    } else if (m_pix.size() == 2) {
      if (GetFlags(FLAG_NEEDCDS)) {
        m_temp_pix.resize(m_pix[0].size());
        for (size_t i = 0; i < m_temp_pix.size(); ++i) {
          m_temp_pix[i] = m_pix[1][i] - m_pix[0][i];
        }
        m_result_pix = &m_temp_pix;
      } else {
        if (m_x.size() == 1) {
          m_temp_pix.resize(m_pix[0].size());
          for (size_t i = 0; i < m_temp_pix.size(); ++i) {
            m_temp_pix[i] = m_pix[1 - m_pivot[0][i]][i];
          }
        } else {
          m_temp_x.resize(0);
          m_temp_y.resize(0);
          m_temp_pix.resize(0);
          m_temp_time.resize(0);
          const bool inverse = false;
          size_t i;
          for (i = 0; i < m_pix[1 - inverse].size(); ++i) {
            if (m_pivot[1][i])
            break;
            m_temp_x.push_back(m_x[1 - inverse][i]);
            m_temp_y.push_back(m_y[1 - inverse][i]);
            m_temp_pix.push_back(m_pix[1 - inverse][i]);
            m_temp_time.push_back(m_time[1-inverse][i]);
          }
          for (i = 0; i < m_pix[0 + inverse].size(); ++i) {
            if (m_pivot[0 + inverse][i])
            break;
          }
          for (/**/; i < m_pix[0 + inverse].size(); ++i) {
            m_temp_x.push_back(m_x[0 + inverse][i]);
            m_temp_y.push_back(m_y[0 + inverse][i]);
            m_temp_pix.push_back(m_pix[0 + inverse][i]);
            m_temp_time.push_back(m_time[0 + inverse][i]);
          }
          m_result_x = &m_temp_x;
          m_result_y = &m_temp_y;
          m_result_time = &m_temp_time;
        }
        m_result_pix = &m_temp_pix;
      }
    } else if (m_pix.size() == 3 && GetFlags(FLAG_NEEDCDS)) {
      m_temp_pix.resize(m_pix[0].size());
      for (size_t i = 0; i < m_temp_pix.size(); ++i) {
        m_temp_pix[i] = m_pix[0][i] * (m_pivot[0][i] - 1) +
        m_pix[1][i] * (2 * m_pivot[0][i] - 1) +
        m_pix[2][i] * (m_pivot[0][i]);
      }
      m_result_pix = &m_temp_pix;
    } else {
      EUDAQ_THROW("Unrecognised pixel format (" + to_string(m_pix.size()) +
      " frames, CDS=" +
      (GetFlags(FLAG_NEEDCDS) ? "Needed" : "Done") + ")");
    }
  }

  template std::vector<short> StandardPlane::GetPixels<>() const;
  template std::vector<int> StandardPlane::GetPixels<>() const;
  template std::vector<double> StandardPlane::GetPixels<>() const;
}
