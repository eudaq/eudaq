#ifndef EUDAQ_INCLUDED_StandardPlane
#define EUDAQ_INCLUDED_StandardPlane

#include "eudaq/Serializable.hh"
#include "eudaq/Serializer.hh"
#include "eudaq/Deserializer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"

#include <vector>
#include <string>

namespace eudaq {

  class DLLEXPORT StandardPlane : public Serializable {
  public:
    enum FLAGS {
      FLAG_ZS = 0x1, // Data are zero suppressed
      FLAG_NEEDCDS = 0x2, // CDS needs to be calculated (data is 2 or 3 raw frames)
      FLAG_NEGATIVE = 0x4, // Signal polarity is negative
      FLAG_ACCUMULATE = 0x8, // Multiple frames should be accumulated for output
      FLAG_WITHPIVOT = 0x10000, // Include before/after pivot boolean per pixel
      FLAG_WITHSUBMAT = 0x20000, // Include Submatrix ID per pixel
      FLAG_DIFFCOORDS = 0x40000 // Each frame can have different coordinates (in ZS mode)
    };
    typedef double pixel_t;
    typedef double coord_t;
    StandardPlane(uint32_t id, const std::string &type,
                  const std::string &sensor = "");
    StandardPlane(Deserializer &);
    StandardPlane();
    void Serialize(Serializer &) const;
    void SetSizeRaw(uint32_t w, uint32_t h, uint32_t frames = 1, int flags = 0);
    void SetSizeZS(uint32_t w, uint32_t h, uint32_t npix, uint32_t frames = 1,
                   int flags = 0);

    template <typename T>
      void SetPixel(uint32_t index, uint32_t x, uint32_t y, T pix,
		    bool pivot = false, uint32_t frame = 0) {
      SetPixelHelper(index, x, y, (double)pix, pivot, frame);
    }
    template <typename T>
      void SetPixel(uint32_t index, uint32_t x, uint32_t y, T pix,
		    uint32_t frame) {
      SetPixelHelper(index, x, y, (double)pix, false, frame);
    }
    template <typename T>
      void PushPixel(uint32_t x, uint32_t y, T pix, bool pivot = false,
		     uint32_t frame = 0) {
      PushPixelHelper(x, y, (double)pix, pivot, frame);
    }
    template <typename T>
      void PushPixel(uint32_t x, uint32_t y, T pix, uint32_t frame) {
      PushPixelHelper(x, y, (double)pix, false, frame);
    }

    void SetPixelHelper(uint32_t index, uint32_t x, uint32_t y, double pix,
                        bool pivot, uint32_t frame);
    void PushPixelHelper(uint32_t x, uint32_t y, double pix, bool pivot,
                         uint32_t frame);
    double GetPixel(uint32_t index, uint32_t frame) const;
    double GetPixel(uint32_t index) const;
    double GetX(uint32_t index, uint32_t frame) const;
    double GetX(uint32_t index) const;
    double GetY(uint32_t index, uint32_t frame) const;
    double GetY(uint32_t index) const;
    bool GetPivot(uint32_t index, uint32_t frame = 0) const;
    void SetPivot(uint32_t index, uint32_t frame, bool PivotFlag);
    // defined for short, int, double
    template <typename T> std::vector<T> GetPixels() const {
      SetupResult();
      std::vector<T> result(m_result_pix->size());
      for (size_t i = 0; i < result.size(); ++i) {
	result[i] = static_cast<T>((*m_result_pix)[i] * Polarity());
      }
      return result;
    }
    const std::vector<coord_t> &XVector(uint32_t frame) const;
    const std::vector<coord_t> &XVector() const;
    const std::vector<coord_t> &YVector(uint32_t frame) const;
    const std::vector<coord_t> &YVector() const;
    const std::vector<pixel_t> &PixVector(uint32_t frame) const;
    const std::vector<pixel_t> &PixVector() const;

    void SetXSize(uint32_t x);
    void SetYSize(uint32_t y);
    void SetPivotPixel(uint32_t p);
    void SetFlags(FLAGS flags);

    uint32_t ID() const;
    const std::string &Type() const;
    const std::string &Sensor() const;
    uint32_t XSize() const;
    uint32_t YSize() const;
    uint32_t NumFrames() const;
    uint32_t TotalPixels() const;
    uint32_t HitPixels(uint32_t frame) const;
    uint32_t HitPixels() const;
    uint32_t PivotPixel() const;

    int GetFlags(int f) const;
    bool NeedsCDS() const;
    int  Polarity() const;

    void Print(std::ostream &) const;
    void Print(std::ostream &os ,size_t offset) const;
  private:
    const std::vector<pixel_t> &
      GetFrame(const std::vector<std::vector<pixel_t>> &v, uint32_t f) const;
    void SetupResult() const;

    std::string m_type;
    std::string m_sensor;
    uint32_t m_id;
    uint32_t m_xsize;
    uint32_t m_ysize;
    uint32_t m_flags;
    uint32_t m_pivotpixel;
    std::vector<std::vector<pixel_t>> m_pix;
    std::vector<std::vector<coord_t>> m_x, m_y;
    std::vector<std::vector<bool>> m_pivot;
    std::vector<uint32_t> m_mat;

    mutable const std::vector<pixel_t> *m_result_pix;
    mutable const std::vector<coord_t> *m_result_x, *m_result_y;

    mutable std::vector<pixel_t> m_temp_pix;
    mutable std::vector<coord_t> m_temp_x, m_temp_y;
  };
  inline std::ostream &operator<<(std::ostream &os, const StandardPlane &pl) {
    pl.Print(os);
    return os;
  }

  inline bool operator==(StandardPlane const &a, StandardPlane const &b) {
    return (a.Sensor() == b.Sensor() && a.ID() == b.ID());
  }

  inline bool operator<(StandardPlane const &a, StandardPlane const &b) {
    return (a.Sensor() < b.Sensor() ||
	    (a.Sensor() == b.Sensor() && a.ID() < b.ID()));
  }

} // namespace eudaq

#endif // EUDAQ_INCLUDED_StandardPlane
