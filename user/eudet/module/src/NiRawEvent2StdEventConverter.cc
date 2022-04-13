#define NOMINMAX
#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

#define PIVOTPIXELOFFSET 64

class NiRawEvent2StdEventConverter: public eudaq::StdEventConverter{
  typedef std::vector<uint8_t>::const_iterator datait;
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
  void DecodeFrame(eudaq::StandardPlane& plane, const uint32_t fm_n,
           const uint8_t *const d, const size_t l32, bool fix_pivot = false) const;
  static const uint32_t m_id_factory = eudaq::cstr2hash("NiRawDataEvent");
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<NiRawEvent2StdEventConverter>(NiRawEvent2StdEventConverter::m_id_factory);
}

bool NiRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{

  static const std::vector<uint32_t> m_ids = {0, 1, 2, 3, 4, 5};
  //TODO: number of telescope plane may be less than 6. Decode additional tags
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  if(!ev)
    return false;

    // Identify the detetor type
  d2->SetDetectorType("MIMOSA26");

  if(!d2->IsFlagPacket()){
    d2->SetFlag(d1->GetFlag());
    d2->SetRunN(d1->GetRunN());
    d2->SetEventN(d1->GetEventN());
    d2->SetStreamN(d1->GetStreamN());
    d2->SetTriggerN(d1->GetTriggerN(), d1->IsFlagTrigger());
    d2->SetTimestamp(d1->GetTimestampBegin(), d1->GetTimestampEnd(), d1->IsFlagTimestamp());
  }

  auto &rawev = *ev;
  if (rawev.NumBlocks() < 2 || rawev.GetBlock(0).size() < 20 ||
      rawev.GetBlock(1).size() < 20) {
    EUDAQ_WARN("Ignoring bad event " + std::to_string(rawev.GetEventNumber()));
    return false;
  }
  auto use_all_hits = (conf != nullptr ? bool(conf->Get("use_all_hits",0)) : false);

  const std::vector<uint8_t> &data0 = rawev.GetBlock(0);
  const std::vector<uint8_t> &data1 = rawev.GetBlock(1);
  uint32_t header0 = eudaq::getlittleendian<uint32_t>(&data0[0]);
  uint32_t header1 = eudaq::getlittleendian<uint32_t>(&data1[0]);
  uint16_t pivot = eudaq::getlittleendian<uint16_t>(&data0[4]);
  uint16_t tluid = eudaq::getlittleendian<uint16_t>(&data0[6]);
  datait it0 = data0.begin() + 8;
  datait it1 = data1.begin() + 8;
  uint32_t board = 0;
  while (it0 < data0.end() && it1 < data1.end()) {
    uint32_t id = m_ids[board];
    uint32_t fcnt0 =  eudaq::getlittleendian<uint32_t>(&(*(it0)));
    uint32_t fcnt1 =  eudaq::getlittleendian<uint32_t>(&(*(it1)));
    uint16_t len0 = eudaq::getlittleendian<uint16_t>(&(*(it0+4)));
    uint16_t len0_h = eudaq::getlittleendian<uint16_t>(&(*(it0+6)));
    uint16_t len1 = eudaq::getlittleendian<uint16_t>(&(*(it1+4)));
    uint16_t len1_h = eudaq::getlittleendian<uint16_t>(&(*(it1+6)));
    if (len0!= len0_h) {
      EUDAQ_WARN("Mismatched lengths decoding first frame (" +
		 std::to_string(len0) + ", " + std::to_string(len0_h) +
		 ")");
      len0 = std::max(len0, len0_h);
    }
    if (len1 != len1_h) {
      EUDAQ_WARN("Mismatched lengths decoding second frame (" +
		 std::to_string(len1) + ", " + std::to_string(len1_h) +
		 ")");
      len1 = std::max(len1, len1_h);
    }

    if (len0 * 4 + 12 > data0.end()-it0) {
      EUDAQ_WARN("Bad length in first frame, len0 * 4 + 12 > data0.end()-it0 ("+
		 std::to_string(len0 * 4 + 12)+" > "+ std::to_string(data0.end()-it0)+
		 ")");
      break;
    }

    if (len1 * 4 + 12 > data1.end() - it1) {
      EUDAQ_WARN("Bad length in second frame,  len1 * 4 + 12 > data1.end()-it1 ("+
		 std::to_string(len1 * 4 + 12)+" > "+ std::to_string(data1.end()-it1)+
		 ")");
      break;
    }

    eudaq::StandardPlane plane(id, "NI", "MIMOSA26");
    plane.SetSizeZS(1152, 576, 0, 2, eudaq::StandardPlane::FLAG_WITHPIVOT |
		    eudaq::StandardPlane::FLAG_DIFFCOORDS);
    plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
    DecodeFrame(plane, 0, &it0[8], len0, use_all_hits);
    DecodeFrame(plane, 1, &it1[8], len1, use_all_hits);
    d2->AddPlane(plane);

    bool advance_one_block_0 = false;
    bool advance_one_block_1 = false;
    if (it0 < data0.end() - (len0 + 4) * 4) {
      advance_one_block_0 = true;
    }
    if (it1 < data1.end() - (len1 + 4) * 4) {
      advance_one_block_1 = true;
    }

    bool dbg = true;
    if (advance_one_block_0 && advance_one_block_1) {
      it0 = it0 + (len0 + 4) * 4;
      if (it0 <= data0.end()){
	header0 = eudaq::getlittleendian<uint32_t>(&(*(it0-4)));
      }

      it1 = it1 + (len1 + 4) * 4;
      if (it1 <= data1.end()){
	header1 = eudaq::getlittleendian<uint32_t>(&(*(it1-4)));
      }
    } else {
      // EUDAQ_WARN("break the block");
      break;
    }
    ++board;
  }
  return true;
}

void NiRawEvent2StdEventConverter::DecodeFrame(eudaq::StandardPlane& plane, const uint32_t fm_n,
                           const uint8_t *const d, const size_t l32, bool fix_pivot) const{
  std::vector<uint16_t> vec;
  for (size_t i = 0; i < l32; ++i) {
    vec.push_back(eudaq::getlittleendian<uint16_t>(d+i*4));
    vec.push_back(eudaq::getlittleendian<uint16_t>(d+i*4+2));
  }

  size_t lvec = vec.size();
  for (size_t i = 0; i+1 < lvec; ++i) {
    uint16_t numstates = vec[i] & 0x000f;
    uint16_t row = vec[i] >> 4 & 0x7ff;
    if (i+1+numstates > lvec){ //offset+ [row] + [column......]
      break;
    }
    bool pivot = (fix_pivot ? 1-fm_n : (row >= (plane.PivotPixel() / 16)));
    for (uint16_t s = 0; s < numstates; ++s) {
      uint16_t v = vec.at(++i);
      uint16_t column = v >> 2 & 0x7ff;
      uint16_t num = v & 3;
      for (uint16_t j = 0; j < num + 1; ++j) {
        plane.PushPixel(column + j, row, 1,0, pivot, fm_n);
      }
    }
  }

}
