#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

  namespace {
    static const int temp_order_0[] = { 0, 1, 2, 3 };
    static const std::vector<int> order_default(temp_order_0, temp_order_0+4);

    static const int temp_order_1[] = { 3, 1, 2, 0 };
    static const std::vector<int> order_swap03(temp_order_1, temp_order_1+4);
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
    : m_det(DET_MIMOTEL), m_mode(MODE_RAW3), m_rows(0), m_cols(0), m_mats(0), m_order(order_swap03)
  {
  }

  EUDRBDecoder::BoardInfo::BoardInfo(const EUDRBEvent & ev, unsigned brd)
    : m_rows(0), m_cols(0), m_mats(0), m_order(order_swap03)
  {
    std::string det = ev.GetTag("DET" + to_string(brd));
    if (det == "") det = ev.GetTag("DET", "MIMOTEL");

    if (det == "MIMOTEL") m_det = DET_MIMOTEL;
    else if (det == "MIMOSTAR2") m_det = DET_MIMOSTAR2;
    else if (det == "MIMOSA18") m_det = DET_MIMOSA18;
    else if (det == "MIMOSA1") m_det = DET_MIMOSA18; // DEBUGGING (can be removed)
    else EUDAQ_THROW("Unknown detector in EUDRBDecoder: " + det);

    std::string mode = ev.GetTag("MODE" + to_string(brd));
    if (mode == "") mode = ev.GetTag("MODE", "RAW3");

    if (mode == "ZS") m_mode = MODE_ZS;
    else if (mode == "RAW2") m_mode = MODE_RAW2;
    else if (mode == "RAW3") m_mode = MODE_RAW3;
    else if (mode == "Mixed") {
      /// TODO: remove this 'else if' block it is just a hack for testing
      m_mode = (brd % 2) ? MODE_ZS : MODE_RAW3;
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
        m_order = order_swap03;
      } else {
        m_order = order_default;
      }
    } else if (m_det == DET_MIMOSTAR2) {
      if (!m_rows) m_rows = 128;
      if (!m_cols) m_cols = 66;
      if (!m_mats) m_mats = 4;
      m_order = order_swap03; // ???
    } else if (m_det == DET_MIMOSA18) {
      if (!m_rows) m_rows = 256;
      if (!m_cols) m_cols = 256;
      if (!m_mats) m_mats = 4;
      m_order = order_swap03;
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
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysRawMTEL(const EUDRBBoard & brd) const {
    const BoardInfo & b = GetInfo(brd);
    const size_t datasize = 2 * (b.m_rows * b.m_cols - 1) * b.m_mats * NumFrames(brd);
    if (brd.DataSize() != datasize) {
      EUDAQ_THROW("EUDRB data size mismatch " + to_string(brd.DataSize()) +
                  ", expecting " + to_string(datasize));
    }
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(NumPixels(brd), NumFrames(brd));
    const unsigned char * data = brd.GetData();
    unsigned pivot = brd.PivotPixel();

    unsigned nxpixel = b.m_cols*b.m_mats; //number of pixels in x direction
    //unsigned nypixel = b.m_rows;          //number of pixels in y direction

    for (size_t row = 0; row < b.m_rows; ++row) {
      for (size_t col = 0; col < b.m_cols; ++col) {
        if (row == b.m_rows-1 && col == b.m_cols-1) break; // last pixel is not transferred
        for (size_t frame = 0; frame < NumFrames(brd); ++frame) {
          for (size_t mat = 0; mat < b.m_mats; ++mat) {
            unsigned x = col + b.m_order[mat]*b.m_cols;
            unsigned y = row;
            size_t i = x + y*nxpixel;
            if (frame == 0) {
              result.m_x[i] = x;
              result.m_y[i] = y;
              result.m_pivot[i] = (row<<7 | col) >= pivot;
            }
            short pix = *data++ << 8;
            pix |= *data++;
            pix &= 0xfff;
            result.m_adc[frame][i] = pix;
          }
        }
      }
    }
    return result;
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysZSMTEL(const EUDRBBoard & brd) const {
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
    const BoardInfo & b = GetInfo(brd);
    const size_t datasize = 2 * (b.m_rows * b.m_cols - 1) * b.m_mats * NumFrames(brd);
    if (brd.DataSize() != datasize) {
      EUDAQ_THROW("EUDRB data size mismatch " + to_string(brd.DataSize()) +
                  ", expecting " + to_string(datasize));
    }
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(NumPixels(brd), NumFrames(brd));
    const unsigned char * data = brd.GetData();
    unsigned pivot = brd.PivotPixel();

    unsigned nxpixel = b.m_cols*2; //number of pixels in x direction
    unsigned nypixel = b.m_rows*2; //number of pixels in y direction

    for (size_t row = 0; row < b.m_rows; ++row) {
      for (size_t col = 0; col < b.m_cols; ++col) {
        if (row == b.m_rows-1 && col == b.m_cols-1) break; // last pixel is not transferred
        for (size_t frame = 0; frame < NumFrames(brd); ++frame) {
          for (size_t mat = 0; mat < b.m_mats; ++mat) {
            unsigned x = 0;
            unsigned y = 0;
            unsigned coltmp = col, rowtmp = row;
            if (NumFrames(brd) == 2) { // rolling frames
              coltmp = (col + (pivot & 0x1f)) % b.m_cols;
              rowtmp = (row + (pivot >> 9)) % b.m_rows;
            }
            switch (b.m_order[mat]) {
            case 0:
              x = rowtmp;
              y = coltmp;
              break;
            case 1:
              x = rowtmp;
              y = nypixel - 1 - rowtmp;
              break;
            case 2:
              x = nxpixel - 1 - coltmp;
              y = nypixel - 1 - rowtmp;
              break;
            case 3:
              x = nxpixel - 1 - coltmp;
              y = coltmp;
              break;
            }
            size_t i = x + y*nxpixel;
            if (frame == 0) {
              result.m_x[i] = x;
              result.m_y[i] = y;
              result.m_pivot[i] = (row<<7 | col) >= pivot;
            }
            short pix = *data++ << 8;
            pix |= *data++;
            pix &= 0xfff;
            result.m_adc[frame][i] = pix;
          }
        }
      }
    }
    return result;
  }

  template <typename T_coord, typename T_adc>
  EUDRBDecoder::arrays_t<T_coord, T_adc> EUDRBDecoder::GetArraysZSM18(const EUDRBBoard & brd) const {
    const BoardInfo & b = GetInfo(brd);
    unsigned pixels = NumPixels(brd);
    EUDRBDecoder::arrays_t<T_coord, T_adc> result(pixels, NumFrames(brd));
    const unsigned char * data = brd.GetData();
    for (unsigned i = 0; i < pixels; ++i) {
      int mat = 3 - (data[4*i] >> 6);
      int col = ((data[4*i+1] & 0x1F) << 4) | (data[4*i+2] >> 4);
      int row = ((data[4*i] & 0x3F) << 3) |  (data[4*i+1] >> 5);
      result.m_adc[0][i] = ((data[4*i+2] & 0x0F) << 8) | (data[4*i+3]);
      result.m_pivot[i] = false;
      switch (b.m_order[mat]) {
      case 0:
        result.m_x[i] = col;
        result.m_y[i] = row;
        break;
      case 1:
        result.m_x[i] = col;
        result.m_y[i] = 2 * b.m_rows - 1 - row ;
        break;
      case 2:
        result.m_x[i] = 2 * b.m_cols - 1 - col;
        result.m_y[i] = 2 * b.m_rows - 1 - row;
        break;
      case 3:
        result.m_x[i] = 2 * b.m_cols - 1 - col;
        result.m_y[i] = row;
        break;
      }
    }
    return result;
  }

  template EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysRawMTEL<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysZSMTEL<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysRawM18<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<double, double> EUDRBDecoder::GetArraysZSM18<>(const EUDRBBoard & brd) const;

  template EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysRawMTEL<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysZSMTEL<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysRawM18<>(const EUDRBBoard & brd) const;
  template EUDRBDecoder::arrays_t<short, short> EUDRBDecoder::GetArraysZSM18<>(const EUDRBBoard & brd) const;

}
