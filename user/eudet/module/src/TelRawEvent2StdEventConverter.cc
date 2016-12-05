#define NOMINMAX
#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"

#define GET(d, i) getlittleendian<uint32_t>(&(d)[(i)*4])
#define PIVOTPIXELOFFSET 64

namespace eudaq{

  class TelRawEvent2StdEventConverter: public StdEventConverter{
    typedef std::vector<uint8_t>::const_iterator datait;
  public:
    bool Converting(EventSPC d1, StandardEventSP d2, const Configuration &conf) const override;
    void DecodeFrame(StandardPlane& plane, const uint32_t fm_n,
		     const uint8_t *const d, const size_t l32) const;
    static const uint32_t m_id_cvt = cstr2hash("TelRawDataEvent");
  };
  
  namespace{
    auto dummy0 = Factory<StdEventConverter>::
      Register<TelRawEvent2StdEventConverter>(TelRawEvent2StdEventConverter::m_id_cvt);
  }  

  bool TelRawEvent2StdEventConverter::Converting(EventSPC d1, StandardEventSP d2, const Configuration &conf) const{
    static const uint32_t m_boards = 6;
    static const std::vector<uint32_t> m_ids = {0, 1, 2, 3, 4, 5};
    //TODO: number of telescope plane may be less than 6. Decode additional tags 
    auto ev = std::dynamic_pointer_cast<const RawDataEvent>(d1);
    if(!ev)
      return false;
    if(ev->IsBORE() || ev->IsEORE())
      return true;
    
    auto &rawev = *ev.get();

    if (rawev.NumBlocks() != 2 || rawev.GetBlock(0).size() < 20 ||
	rawev.GetBlock(1).size() < 20) {
      EUDAQ_WARN("Ignoring bad event " + to_string(rawev.GetEventNumber()));
      return false;
    }

    const std::vector<uint8_t> &data0 = rawev.GetBlock(0);
    const std::vector<uint8_t> &data1 = rawev.GetBlock(1);
    uint32_t header0 = getlittleendian<uint32_t>(&data0[0]);
    uint32_t header1 = getlittleendian<uint32_t>(&data0[0]);
    uint16_t pivot = getlittleendian<uint16_t>(&data0[4]);
    uint16_t tluid = getlittleendian<uint16_t>(&data0[6]);
    datait it0 = data0.begin() + 8;
    datait it1 = data1.begin() + 8;
    uint32_t board = 0;
    while (it0 < data0.end() && it1 < data1.end()) {
      uint32_t id = m_ids[board];
      uint32_t fcnt0 =  getlittleendian<uint32_t>(&data0[8]);
      uint32_t fcnt1 =  getlittleendian<uint32_t>(&data1[8]);
      uint16_t len0 = getlittleendian<uint16_t>(&data0[12]);
      uint16_t len0_h = getlittleendian<uint16_t>(&data0[14]);
      if (len0!= len0_h) {
	EUDAQ_WARN("Mismatched lengths decoding first frame (" +
		   to_string(len0) + ", " + to_string(len0_h) +
		   ")");
	len0 = std::max(len0, len0_h);
      }

      uint16_t len1 = getlittleendian<uint16_t>(&data1[12]);
      uint16_t len1_h = getlittleendian<uint16_t>(&data1[14]);
      if (len1 != len1_h) {
	EUDAQ_WARN("Mismatched lengths decoding second frame (" +
		   to_string(len1) + ", " + to_string(len1_h) +
		   ")");
	len1 = std::max(len1, len1_h);
      }

      if (len0 * 4 + 12 > data0.end()-it0) {
	EUDAQ_WARN("Bad length in first frame");
	break;
      }
      if (len1 * 4 + 12 > data1.end() - it1) {
	EUDAQ_WARN("Bad length in second frame");
	break;
      }

      StandardPlane plane(id, "NI", "MIMOSA26");
      plane.SetSizeZS(1152, 576, 0, 2, StandardPlane::FLAG_WITHPIVOT |
		      StandardPlane::FLAG_DIFFCOORDS);
      plane.SetTLUEvent(tluid);
      plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
      DecodeFrame(plane, 0, &it0[8], len0);
      DecodeFrame(plane, 1, &it1[8], len1);
      d2->AddPlane(plane);

      if (it0 < data0.end()-(len0+4)*4 &&
	  it1 < data1.end()-(len1+4)*4){
	it0 = it0 + (len0 + 4) * 4;
	it1 = it1 + (len1 + 4) * 4;
      } else {
	break;
      }
      ++board;
    }
    return true;
  }


  void TelRawEvent2StdEventConverter::DecodeFrame(StandardPlane& plane, const uint32_t fm_n,
						  const uint8_t *const d, const size_t l32) const{
    std::vector<uint16_t> vec;
    for (size_t i = 0; i < l32; ++i) {
      vec.push_back(getlittleendian<uint16_t>(d+i*4));
      vec.push_back(getlittleendian<uint16_t>(d+i*4+2));
    }

    size_t lvec = vec.size();
    for (size_t i = 0; i+1 < lvec; ++i) {
      uint16_t numstates = vec[i] & 0x000f;
      uint16_t row = vec[i] >> 4 & 0x7ff;
      if (i+1+numstates > lvec){ //offset+ [row] + [column......]
	break;
      }
      bool pivot = (row >= (plane.PivotPixel() / 16));
      for (uint16_t s = 0; s < numstates; ++s) {
	uint16_t v = vec.at(++i);
	uint16_t column = v >> 2 & 0x7ff;
	uint16_t num = v & 3;
	for (uint16_t j = 0; j < num + 1; ++j) {
	  plane.PushPixel(column + j, row, 1, pivot, fm_n);
	}
      }
    }
      
  }
  
}
