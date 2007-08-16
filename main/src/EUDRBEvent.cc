#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

  namespace {
    static const int temp1[] = { 3, 1, 2, 0 };
    static const std::vector<int> order_mimotel_old(temp1, temp1+4);

    static const int temp2[] = { 0, 1, 2, 3 };
    static const std::vector<int> order_mimotel_new(temp2, temp2+4);
  }

  EUDAQ_DEFINE_EVENT(EUDRBEvent, str2id("_DRB"));

  EUDRBBoard::EUDRBBoard(Deserializer & ds) {
    ds.read(m_id);
    ds.read(m_data);
  }

  void EUDRBBoard::Serialize(Serializer & ser) const {
    ser.write(m_id);
    ser.write(m_data);
  }

  unsigned EUDRBBoard::LocalEventNumber() const {
    return (GetByte(1) << 8) | GetByte(2);
  }

  unsigned EUDRBBoard::TLUEventNumber() const {
    return (GetByte(m_data.size()-7) << 8) | GetByte(m_data.size()-6);
  }

  unsigned EUDRBBoard::FrameNumber() const {
    return GetByte(3);
  }

  unsigned EUDRBBoard::PivotPixel() const {
    return ((GetByte(5) & 0x3) << 16) | (GetByte(6) << 8) | GetByte(7);
  }

  unsigned EUDRBBoard::WordCount() const {
    return ((GetByte(m_data.size()-3) & 0x3) << 16) |
      (GetByte(m_data.size()-2) << 8) |
      GetByte(m_data.size()-1);
  }

  size_t EUDRBBoard::DataSize() const {
    return m_data.size() - 16;
  }

  void EUDRBBoard::Print(std::ostream & os) const {
    os << "  ID            = " << m_id << "\n"
       << "  LocalEventNum = " << LocalEventNumber() << "\n"
       << "  TLUEventNum   = " << TLUEventNumber() << "\n"
       << "  FrameNum      = " << FrameNumber() << "\n"
       << "  PivotPixel    = " << PivotPixel() << "\n"
       << "  WordCount     = " << WordCount() << "\n"
       << "  DataSize      = " << DataSize() << "\n";
  }

  EUDRBEvent::EUDRBEvent(Deserializer & ds) :
    Event(ds)
  {
    ds.read(m_boards);
  }

  void EUDRBEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_boards.size() << " boards";
  }

  void EUDRBEvent::Debug() {
    for (unsigned i = 0; i < NumBoards(); ++i) {
      std::cout << GetBoard(i) << std::endl;
    }
  }

  void EUDRBEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write(m_boards);
  }

  EUDRBDecoder::EUDRBDecoder(EUDRBDecoder::E_DET det, EUDRBDecoder::E_MODE mode)
    : m_det(det), m_mode(mode)
  {
    Check();
  }

  EUDRBDecoder::EUDRBDecoder(const EUDRBEvent & bore)
    : m_rows(0),
      m_cols(0),
      m_mats(0)
  {
    std::string det = bore.GetTag("DET", "MIMOTEL");
    if (det == "MIMOTEL") m_det = DET_MIMOTEL;
    else if (det == "MIMOSTAR2") m_det = DET_MIMOSTAR2;
    else EUDAQ_THROW("Unknown detector in EUDRBDecoder: " + det);
    std::string mode = bore.GetTag("MODE", "RAW3");
    if (mode == "ZS") m_mode = MODE_ZS;
    else if (mode == "RAW2") m_mode = MODE_RAW2;
    else if (mode == "RAW3") m_mode = MODE_RAW3;
    else EUDAQ_THROW("Unknown mode in EUDRBDecoder: " + mode);
    Check();
  }

  void EUDRBDecoder::Check() {
    if (m_det == DET_MIMOTEL || m_det == DET_MIMOTEL_NEWORDER) {
      if (!m_rows) m_rows = 256;
      if (!m_cols) m_cols = 66;
      if (!m_mats) m_mats = 4;
      if (m_det == DET_MIMOTEL)
        m_order = order_mimotel_old;
      else
        m_order = order_mimotel_new;

    } else if (m_det == DET_MIMOSTAR2) {
      if (!m_rows) m_rows = 128;
      if (!m_cols) m_cols = 66;
      if (!m_mats) m_cols = 4;
    } else {
      EUDAQ_THROW("Unknown detector in EUDRBDecoder");
    }
    if (m_mode != MODE_RAW3 && m_mode != MODE_RAW2 && m_mode != MODE_ZS)
      EUDAQ_THROW("Unknown mode in EUDRBDecoder");
  }

  unsigned EUDRBDecoder::NumFrames() const {
    return 3 - m_mode; // RAW3=0 -> 3, RAW2=1 -> 2, ZS=2 -> 1
  }

  unsigned EUDRBDecoder::NumPixels(const EUDRBBoard & /*brd*/) const {
    if (m_mode != MODE_ZS) return m_rows * m_cols * m_mats;
    EUDAQ_THROW("ZS mode not yet implemented");
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArrays(const EUDRBBoard & brd) const {
    const size_t datasize = 2 * m_rows * m_cols * m_mats * NumFrames();
    if (brd.DataSize() < datasize) {
      EUDAQ_ERROR("EUDRB data size too small: " +
                  to_string(brd.DataSize()) + " < " + to_string(datasize) +
                  ", event = " + to_string(brd.LocalEventNumber()) +
                  " (local) " + to_string(brd.TLUEventNumber()) + " (TLU)");
    } else if (brd.DataSize() > datasize) {
      EUDAQ_WARN("EUDRB data size larger than expected: " +
                 to_string(brd.DataSize()) + " > " + to_string(datasize) +
                  ", event = " + to_string(brd.LocalEventNumber()) +
                  " (local) " + to_string(brd.TLUEventNumber()) + " (TLU)");
    }
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(NumPixels(brd), NumFrames());
    const unsigned char * data = brd.GetData();
    unsigned pivot = brd.PivotPixel();
    //size_t i = 0;
    for (size_t row = 0; row < m_rows; ++row) {
      for (size_t col = 0; col < m_cols; ++col) {
        //size_t saved_i = i;
        for (size_t mat = 0; mat < m_mats; ++mat) {
          size_t i = col + m_order[mat]*m_cols + row*m_mats*m_cols;
          result.m_x[i] = col + m_order[mat]*m_cols;
          result.m_y[i] = row;
          result.m_pivot[i] = (row<<7 | col) >= pivot;
          //i++;
        }
        for (size_t frame = 0; frame < NumFrames(); ++frame) {
          //i = saved_i;
          for (size_t mat = 0; mat < m_mats; ++mat) {
            size_t i = col + m_order[mat]*m_cols + row*m_mats*m_cols;
            short pix = *data++ << 8;
            pix |= *data++;
            pix &= 0xfff;
            result.m_adc[frame][i] = pix;
            //i++;
          }
        }
      }
    }
    return result;
  }

  template
  EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArrays<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArrays<>(const EUDRBBoard & brd) const;

//   template<>
//   EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArrays<short, short>(const EUDRBBoard &) const;
//   template<>
//   EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArrays<double, double>(const EUDRBBoard &) const;

//   EUDRBDecoder::arrays_t EUDRBDecoder::GetArrays(const EUDRBBoard & brd) {
//     //static bool debugged = false;
//     arrays_t result(3*NumFrames(), std::vector<double>(NumPixels(brd), 0.0));
//     const unsigned char * data = brd.GetData();
//     //size_t i = 0, saved_i = 0;
//     for (int frame = 0; frame < NumFrames(); ++frame) {
//       size_t i = 0;
//       for (int row = 0; row < m_rows; ++row) {
//         for (int col = 0; col < m_cols; ++col) {
//           //i = saved_i;
//           for (int mat = 0; mat < m_mats; ++mat) {
//             short pix = *data++ << 8;
//             pix |= *data++;
//             result[3*frame+0][i] = col + mat*m_cols;
//             result[3*frame+1][i] = row;
//             result[3*frame+2][i] = pix & 0xfff;
//             //if (!debugged) std::cout << "DBG: submat=" << mat << ", data=0x" << to_hex(pix, 4) << std::endl;
//             i++;
//           }
//         }
//       }
//     }
//     //debugged = true;
//     return result;
//   }

}
