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

//   EUDRBDecoder::EUDRBDecoder(EUDRBDecoder::E_DET det, EUDRBDecoder::E_MODE mode)
//     : m_det(det), m_mode(mode)
//   {
//     Check();
//   }

  EUDRBDecoder::EUDRBDecoder(const DetectorEvent & bore) {
    for (size_t i = 0; i < bore.NumEvents(); ++i) {
      const EUDRBEvent * ev = dynamic_cast<const EUDRBEvent *>(bore.GetEvent(i));
      //std::cout << "SubEv " << i << (ev ? " is EUDRB" : "") << std::endl;
      if (ev) {
        unsigned nboards = from_string(ev->GetTag("BOARDS"), 0);
        for (size_t j = 0; j < nboards; ++j) {
          //std::cout << "Board " << j << std::endl;
          unsigned id = from_string(ev->GetTag("ID") + to_string(j), j);
          //std::cout << "ID = " << id << std::endl;
          m_info[id] = BoardInfo(*ev, j);
          //std::cout << "ok" << std::endl;
        }
      }
    }
  }

  EUDRBDecoder::BoardInfo::BoardInfo()
    : m_det(DET_MIMOTEL), m_mode(MODE_RAW3), m_rows(0), m_cols(0), m_mats(0)
  {
  }

  EUDRBDecoder::BoardInfo::BoardInfo(const EUDRBEvent & ev, unsigned brd)
    : m_rows(0), m_cols(0), m_mats(0)
  {
    std::string det = ev.GetTag("DET" + to_string(brd));
    if (det == "") det = ev.GetTag("DET", "MIMOTEL");

    if (det == "MIMOTEL") m_det = DET_MIMOTEL;
    else if (det == "MIMOSTAR2") m_det = DET_MIMOSTAR2;
    else if (det == "MIMOSA18") m_det = DET_MIMOSA18;
    else EUDAQ_THROW("Unknown detector in EUDRBDecoder: " + det);

    std::string mode = ev.GetTag("MODE" + to_string(brd));
    if (mode == "") mode = ev.GetTag("MODE", "RAW3");

    if (mode == "ZS") m_mode = MODE_ZS;
    else if (mode == "RAW2") m_mode = MODE_RAW2;
    else if (mode == "RAW3") m_mode = MODE_RAW3;
    else if (mode == "Mixed") {
      /// TODO: remove this 'else if' block it is just a hack for testing
      m_mode = MODE_RAW3;
      if (brd % 2) m_mode = MODE_ZS;
    }
    else EUDAQ_THROW("Unknown mode in EUDRBDecoder: " + mode);

    Check();
  }

  void EUDRBDecoder::BoardInfo::Check() {
    if (m_det == DET_MIMOTEL || m_det == DET_MIMOTEL_NEWORDER) {
      if (!m_rows) m_rows = 256;
      if (!m_cols) m_cols = 66;
      if (!m_mats) m_mats = 4;
      if (m_det == DET_MIMOTEL) {
        m_order = order_mimotel_old;
      } else {
        m_order = order_mimotel_new;
      }
    } else if (m_det == DET_MIMOSTAR2) {
      if (!m_rows) m_rows = 128;
      if (!m_cols) m_cols = 66;
      if (!m_mats) m_cols = 4;
    } else if (m_det == DET_MIMOSA18) {
      if (!m_rows) m_rows = 256;
      if (!m_cols) m_cols = 256;
      if (!m_mats) m_cols = 4;
    }else {
      EUDAQ_THROW("Unknown detector in EUDRBDecoder");
    }
    if (m_mode != MODE_RAW3 && m_mode != MODE_RAW2 && m_mode != MODE_ZS)
      EUDAQ_THROW("Unknown mode in EUDRBDecoder");
  }

  const EUDRBDecoder::BoardInfo & EUDRBDecoder::GetInfo(const EUDRBBoard & brd) const {
    std::map<unsigned, BoardInfo>::const_iterator it = m_info.find(brd.GetID());
    if (it == m_info.end()) EUDAQ_THROW("Unrecognised board ID: " + to_string(brd.GetID()) + ", cannot decode event");
    return it->second;
  }

  unsigned EUDRBDecoder::NumFrames(const EUDRBBoard & brd) const {
    return 3 - GetInfo(brd).m_mode; // RAW3=0 -> 3, RAW2=1 -> 2, ZS=2 -> 1
  }

  unsigned EUDRBDecoder::NumPixels(const EUDRBBoard & brd) const {
    const BoardInfo & b = GetInfo(brd);
    if (b.m_mode != MODE_ZS) return b.m_rows * b.m_cols * b.m_mats;
    return brd.DataSize() / 4;
    //EUDAQ_THROW("ZS mode not yet implemented");
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysRaw(const EUDRBBoard & brd) const {
    const BoardInfo & b = GetInfo(brd);
    const size_t datasize = 2 * b.m_rows * b.m_cols * b.m_mats * NumFrames(brd);
    if (brd.DataSize() != datasize) {
      EUDAQ_THROW("EUDRB data size mismatch " + to_string(brd.DataSize()) +
                  ", expecting " + to_string(datasize));
    }
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(NumPixels(brd), NumFrames(brd));
    const unsigned char * data = brd.GetData();
    unsigned pivot = brd.PivotPixel();
    //size_t i = 0;
    for (size_t row = 0; row < b.m_rows; ++row) {
      for (size_t col = 0; col < b.m_cols; ++col) {
        //size_t saved_i = i;
        for (size_t mat = 0; mat < b.m_mats; ++mat) {
          size_t i = col + b.m_order[mat]*b.m_cols + row*b.m_mats*b.m_cols;
          result.m_x[i] = col + b.m_order[mat]*b.m_cols;
          result.m_y[i] = row;
          result.m_pivot[i] = (row<<7 | col) >= pivot;
          //i++;
        }
        for (size_t frame = 0; frame < NumFrames(brd); ++frame) {
          //i = saved_i;
          for (size_t mat = 0; mat < b.m_mats; ++mat) {
            size_t i = col + b.m_order[mat]*b.m_cols + row*b.m_mats*b.m_cols;
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

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysZS(const EUDRBBoard & brd) const {
    const BoardInfo & b = GetInfo(brd);
    unsigned pixels = NumPixels(brd);
    //std::cout << "board datasize = " << brd.DataSize() << ", pixels = " << pixels << ", frames = " << NumFrames(brd) << std::endl;
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(pixels, NumFrames(brd));
    const unsigned char * data = brd.GetData();
    for (unsigned i = 0; i < pixels; ++i) {
      int mat = 3 - (data[4*i] >> 6);
      int col = ((data[4*i+1] & 0x7) << 4) | (data[4*i+2] >> 4);
      result.m_x[i] = col + b.m_order[mat]*b.m_cols;
      result.m_y[i] = ((data[4*i] & 0x7) << 5) | (data[4*i+1] >> 3);
      result.m_adc[0][i] = ((data[4*i+2] & 0xf) << 8) | data[4*i+3];
      result.m_pivot[i] = false;
    }
    return result;
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysRawM18(const EUDRBBoard & brd) const {
    EUDAQ_THROW("Not implemented");
    (void)brd; // to remove warning
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysZSM18(const EUDRBBoard & brd) const {
    EUDAQ_THROW("Not implemented");
    (void)brd; // to remove warning
  }

  template
  EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysRaw<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysRaw<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysZS<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysZS<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysRawM18<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysRawM18<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysZSM18<>(const EUDRBBoard & brd) const;

  template
  EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysZSM18<>(const EUDRBBoard & brd) const;

}
