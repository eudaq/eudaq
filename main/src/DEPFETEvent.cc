#include "eudaq/DEPFETEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#include <ostream>
#include <iostream>

namespace eudaq {

  EUDAQ_DEFINE_EVENT(DEPFETEvent, str2id("_DEP"));

  DEPFETBoard::DEPFETBoard(Deserializer & ds) {
    ds.read(m_id);
    ds.read(m_data);
  }

  void DEPFETBoard::Serialize(Serializer & ser) const {
    ser.write(m_id);
    ser.write(m_data);
  }


  void DEPFETBoard::Print(std::ostream & os) const {
    os << "  ID            = " << m_id << "\n";
    //        << "  DataSize      = " << DataSize() << "\n";
  }



  DEPFETEvent::DEPFETEvent(Deserializer & ds) :
    Event(ds)
  {
    ds.read(m_boards);
  }

  void DEPFETEvent::Print(std::ostream & os) const {
    Event::Print(os);
    os << ", " << m_boards.size() << " boards";
  }

  void DEPFETEvent::Debug() {
    for (unsigned i = 0; i < NumBoards(); ++i) {
      std::cout << GetBoard(i) << std::endl;
    }
  }

  void DEPFETEvent::Serialize(Serializer & ser) const {
    Event::Serialize(ser);
    ser.write(m_boards);
  }
  //   size_t DEPFETBoard::DataSize() const {
  //     return m_data.size();
  //   }

  //   template <typename T_coord, typename T_adc>
  //   DEPFETDecoder::arrays_t<T_coord, T_adc> DEPFETDecoder::GetArrays(const DEPFETBoard & brd) const {
  //       unsigned pixels = 8192; // datasize 8195!!! 
  //       DEPFETDecoder::arrays_t<T_coord, T_adc> result(pixels, 1);
  //       const unsigned  * data0 = (unsigned *) brd.GetData();
  //       const unsigned  * data = &data0[3];

  //       //printf("eudaq::DEPFET data0=0x%x Trigger=0x%x data2= 0x%x  brdsize=%d\n",
  //       //       data0[0],data0[1],data0[2],(int)brd.DataSize());

  //       for (unsigned i = 0; i < pixels; ++i) {
  //         int col = (data[i]>>16)&0x3F;
  //         int row = (data[i]>>22)&0x7F;

  //         result.m_adc[0][i] = (data[i]& 0xffff);
  //         result.m_pivot[i] = false;
  //         result.m_x[i] = col;
  //         result.m_y[i] = row;
  //         //printf("eudaq::DEPFET ipix=%d col= %d row= %d data=%d brdsize=%d \n",
  //         //       i,col,row,data[i]&0xffff,(int)brd.DataSize());
  //       }

  //       return result;
  //   }

  //   DEPFETDecoder::DEPFETDecoder(const DetectorEvent & /*bore*/) {
  // //     for (size_t i = 0; i < bore.NumEvents(); ++i) {
  // //       const DEPFETEvent * ev = dynamic_cast<const DEPFETEvent *>(bore.GetEvent(i));
  // //       //std::cout << "SubEv " << i << (ev ? " is DEPFET" : "") << std::endl;
  // //       if (ev) {
  // //         unsigned nboards = from_string(ev->GetTag("BOARDS"), 0);
  // //         for (size_t j = 0; j < nboards; ++j) {
  // //           //std::cout << "Board " << j << std::endl;
  // //           unsigned id = from_string(ev->GetTag("ID") + to_string(j), j);
  // //           //std::cout << "ID = " << id << std::endl;
  // //           m_info[id] = BoardInfo(*ev, j);
  // //           // std::cout << "ok" << std::endl;
  // //         }
  // //       }
  // //     }
  //   }


  // #define MAKE_DECODER_TYPE_D(TCOORD, TADC)
  //   template DEPFETDecoder::arrays_t<TCOORD, TADC> DEPFETDecoder::GetArrays<>(const DEPFETBoard & brd) const

  //   MAKE_DECODER_TYPE_D(short, short);
  //   MAKE_DECODER_TYPE_D(double, double);

  // #undef MAKE_DECODER_TYPE_D



}
