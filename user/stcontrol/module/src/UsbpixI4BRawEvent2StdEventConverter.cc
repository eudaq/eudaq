#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

#include "ATLASFE4IInterpreter.hh"
#include <cstdlib>
#include <cstring>
#include <exception>

// £rawdata_path = C:\Users\testbeamadmin\Desktop
// SRAM_READOUT_AT = 60
// SkipConfiguration = no
// UseSingleBoardConfigs = no
// boards = 121
// config_file = C:\usbpix_svn_5.3\bin\workingtdac.cfg.root
// fpga_firmware = C:\usbpix_svn_5.3\config\usbpixi4.bit
// lvl1_delay = 26
// modules[121] = 1
// tlu_trigger_data_delay = 10

class UsbpixI4BRawEvent2StdEventConverter: public eudaq::StdEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("USBPIXI4B");
private:

  uint32_t getWord(const std::vector<uint8_t>& data, size_t index) const;
  eudaq::StandardPlane ConvertPlane(const std::vector<uint8_t> & data, uint32_t id, bool swap_xy) const;
  bool isEventValid(const std::vector<uint8_t> & data) const;
  bool getHitData(uint32_t &Word, bool second_hit,
		  uint32_t &Col, uint32_t &Row, uint32_t &ToT) const;
  uint32_t getTrigger(const std::vector<uint8_t> & data) const;

  static const uint32_t CHIP_MIN_COL = 1;
  static const uint32_t CHIP_MAX_COL = 80;
  static const uint32_t CHIP_MIN_ROW = 1;
  static const uint32_t CHIP_MAX_ROW = 336;
  static const uint32_t CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;
  static const uint32_t CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;
  uint32_t consecutive_lvl1 = 16;

  static ATLASFEI4Interpreter<0x00007C00, 0x000003FF> fei4b_intp;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<UsbpixI4BRawEvent2StdEventConverter>(UsbpixI4BRawEvent2StdEventConverter::m_id_factory);
}


ATLASFEI4Interpreter<0x00007C00, 0x000003FF> UsbpixI4BRawEvent2StdEventConverter::fei4b_intp;

bool UsbpixI4BRawEvent2StdEventConverter::
Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const {
  auto ev_raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  bool swap_xy = ev_raw->GetTag("SWAP_XY", 0);
  auto block_n_list = ev_raw->GetBlockNumList();
  for(auto &bn: block_n_list){
    d2->AddPlane(ConvertPlane(ev_raw->GetBlock(bn), bn+10, swap_xy));//offset 10
  }
  return true;
}

eudaq::StandardPlane UsbpixI4BRawEvent2StdEventConverter::
ConvertPlane(const std::vector<uint8_t> & data, uint32_t id, bool swap_xy) const{
  eudaq::StandardPlane plane(id, "USBPIXI4B", "USBPIXI4B");
  bool valid = isEventValid(data);
  uint32_t ToT = 0;
  uint32_t Col = 0;
  uint32_t Row = 0;
  //FE-I4: DH with lv1 before Data Record
  uint32_t lvl1 = 0;
  int colMult = 1;
  int rowMult = 1;

  // TODO: use this as a conf-parameter 
  if(swap_xy){
      plane.SetSizeZS((CHIP_MAX_ROW_NORM + 1)*rowMult, (CHIP_MAX_COL_NORM + 1)*colMult, 
              0, consecutive_lvl1,
              eudaq::StandardPlane::FLAG_DIFFCOORDS|eudaq::StandardPlane::FLAG_ACCUMULATE);
  }
  else{
      plane.SetSizeZS((CHIP_MAX_COL_NORM + 1)*colMult, (CHIP_MAX_ROW_NORM + 1)*rowMult,
		  0, consecutive_lvl1,
		  eudaq::StandardPlane::FLAG_DIFFCOORDS|eudaq::StandardPlane::FLAG_ACCUMULATE);
  }


  if(!valid){
    return plane;
  }

  //Get Events
  for(size_t i=0; i < data.size()-8; i += 4){
    uint32_t Word = getWord(data, i);
    if(fei4b_intp.is_dh(Word)){
      lvl1++;
    }
    else{
      if(swap_xy){
            //First Hit
            if(getHitData(Word, false, Col, Row, ToT)){
                   plane.PushPixel(Row, Col, ToT, false, lvl1 - 1);
            }
            //Second Hit
            if(getHitData(Word, true, Col, Row, ToT)){
                   plane.PushPixel(Row, Col, ToT, false, lvl1 - 1);
            }
      }
      else{
            //First Hit
            if(getHitData(Word, false, Col, Row, ToT)){
                   plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
            }
            //Second Hit
            if(getHitData(Word, true, Col, Row, ToT)){
                   plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
            }
      }
    }
  }
  return plane;
}


bool UsbpixI4BRawEvent2StdEventConverter::
isEventValid(const std::vector<uint8_t> & data) const{
  //ceck data consistency
  uint32_t dh_found = 0;
  for (size_t i=0; i < data.size()-8; i += 4){
    uint32_t word = getWord(data, i);
    if(fei4b_intp.is_dh(word)){
      dh_found++;
    }
  }
  if(dh_found != consecutive_lvl1){
    return false;
  }
  else{
    return true;
  }
}


bool UsbpixI4BRawEvent2StdEventConverter::
getHitData(uint32_t &Word, bool second_hit,
	   uint32_t &Col, uint32_t &Row, uint32_t &ToT) const{
  //No data record
  if( !fei4b_intp.is_dr(Word)){
    return false;
  }
  uint32_t t_Col=0;
  uint32_t t_Row=0;
  uint32_t t_ToT=15;

  if(!second_hit){
    t_ToT = fei4b_intp.get_dr_tot1(Word);
    t_Col = fei4b_intp.get_dr_col1(Word);
    t_Row = fei4b_intp.get_dr_row1(Word);
  }
  else{
    t_ToT = fei4b_intp.get_dr_tot2(Word);
    t_Col = fei4b_intp.get_dr_col2(Word);
    t_Row = fei4b_intp.get_dr_row2(Word);
  }

  //translate FE-I4 ToT code into tot
  //tot_mode = 0
  if (t_ToT==14 || t_ToT==15)
    return false;
  ToT = t_ToT + 1;

  if(t_Row > CHIP_MAX_ROW || t_Row < CHIP_MIN_ROW){
    std::cout << "Invalid row: " << t_Row << std::endl;
    return false;
  }
  if(t_Col > CHIP_MAX_COL || t_Col < CHIP_MIN_COL){
    std::cout << "Invalid col: " << t_Col << std::endl;
    return false;
  }
  //Normalize Pixelpositions
  t_Col -= CHIP_MIN_COL;
  t_Row -= CHIP_MIN_ROW;
  Col = t_Col;
  Row = t_Row;
  return true;
}

uint32_t UsbpixI4BRawEvent2StdEventConverter::
getTrigger(const std::vector<uint8_t> & data) const{
  //Get Trigger Number and check for errors
  uint32_t i = data.size() - 8; //splitted in 2x 32bit words
  uint32_t Trigger_word1 = getWord(data, i);

  if(Trigger_word1==(uint32_t) -1){
    return (uint32_t)-1;
  }
  uint32_t Trigger_word2 = getWord(data, i+4);
  uint32_t trigger_number = fei4b_intp.get_tr_no_2(Trigger_word1, Trigger_word2);
  return trigger_number;
}

uint32_t UsbpixI4BRawEvent2StdEventConverter::
getWord(const std::vector<uint8_t>& data, size_t index) const{
  return (((uint32_t)data[index + 3]) << 24) | (((uint32_t)data[index + 2]) << 16)
    | (((uint32_t)data[index + 1]) << 8) | (uint32_t)data[index];
}
