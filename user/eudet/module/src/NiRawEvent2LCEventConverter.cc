#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Logger.hh"


#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "UTIL/CellIDEncoder.h"


#define GET(d, i) getlittleendian<uint32_t>(&(d)[(i)*4])

namespace eudaq{

  using namespace lcio;


  static const int dbg = 0; // 0=off, 1=structure, 2=structure+data
  static const int PIVOTPIXELOFFSET = 64;

  class NiRawEvent2LCEventConverter: public LCEventConverter{
    typedef std::vector<unsigned char> datavect;
    typedef std::vector<unsigned char>::const_iterator datait;

  public:
    bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash("NiRawDataEvent");
    
  private:
    bool GetStandardSubEvent(StandardEvent &result, const Event &source) const;
    void DecodeFrame(StandardPlane &plane, size_t len, datait it, int frame) const;
  };
  
  namespace{
    auto dummy0 = Factory<LCEventConverter>::
      Register<NiRawEvent2LCEventConverter>(NiRawEvent2LCEventConverter::m_id_factory);
  }  

  bool NiRawEvent2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const{
    auto& source = *(d1.get());
    auto& result = *(d2.get());
    static const uint32_t m_boards = 6;

    TrackerDataImpl *zsFrame;
 
    // prepare the collections for the rawdata and the zs ones
    LCCollectionVec *zsDataCollection; //, *rawDataCollection, *zs2DataCollection;
    //bool rawDataCollectionExists = false;
    bool zsDataCollectionExists = false;
    //bool zs2DataCollectionExists = false;
    
    /*try {
      rawDataCollection = static_cast<LCCollectionVec *>(result.getCollection("rawdata"));
      rawDataCollectionExists = true;
      std::cout << "found rawdata" << std::endl;
    } catch (lcio::DataNotAvailableException &e) {
      rawDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERRAWDATA);
    }*/

    // we (jha92, dreyling) suspect there should be anyway no LCCollection with a certain name before executing the Converter
    //try {
    //  zsDataCollection = static_cast<LCCollectionVec *>(result.getCollection("zsdata_m26"));
    //  zsDataCollectionExists = true;
    //} catch (lcio::DataNotAvailableException &e) {
      zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
    //}

    /*try {
      zs2DataCollection =
	static_cast<LCCollectionVec *>(result.getCollection("zsdata_m26"));
      zs2DataCollectionExists = true;
      std::cout << "found zsdata_m26" << std::endl;
    } catch (lcio::DataNotAvailableException &e) {
      zs2DataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      std::cout << "catched zsdata_m26" << std::endl;
    }*/

    //airqui
    //Manually set the variables for encoding:
    const char *   MATRIXDEFAULTENCODING    = "sensorID:5,xMin:12,xMax:12,yMin:12,yMax:12";
    const char *   ZSDATADEFAULTENCODING    = "sensorID:7,sparsePixelType:5";

    // set the proper cell encoder
    //CellIDEncoder<TrackerRawDataImpl> rawDataEncoder(MATRIXDEFAULTENCODING, rawDataCollection);
    CellIDEncoder<TrackerDataImpl> zsDataEncoder(ZSDATADEFAULTENCODING, zsDataCollection);
    //CellIDEncoder<TrackerDataImpl> zs2DataEncoder(ZSDATADEFAULTENCODING, zs2DataCollection);

    // to understand if we have problem with de-syncronisation, let
    // me prepare a Boolean switch and a vector of size_t to contain the
    // pivot pixel position
    bool outOfSyncFlag = false;
    std::vector<size_t> pivotPixelPosVec;

    if (dbg > 0)
      std::cout
	<< "NiRawEvent2LCEventConverter::Converting rawDataEvent with boards="
	<< m_boards << std::endl;
    const RawDataEvent &rawDataEvent =
      dynamic_cast<const RawEvent &>(source);

    size_t numplanes = m_boards; // NumPlanes(source);

    StandardEvent tmp_evt;
    GetStandardSubEvent(tmp_evt, rawDataEvent);

    if (numplanes != tmp_evt.NumPlanes()) {
      numplanes = tmp_evt.NumPlanes();
    }

    for (size_t iPlane = 0; iPlane < numplanes; ++iPlane) {
      if (dbg > 2)
        std::cout << "setting plane #  " << iPlane << " out of " << numplanes
                  << " up to " << tmp_evt.NumPlanes() << std::endl;
      StandardPlane plane = static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

      //if (plane.GetFlags(StandardPlane::FLAG_ZS)) {
      zsDataEncoder["sensorID"] = plane.ID();
      zsDataEncoder["sparsePixelType"] = 2 ; //airqui hardcoded eutelescope::kEUTelGenericSparsePixel;

      // get the total number of pixel. This is written in the
      // eudrbBoard and to get it in a easy way pass through the eudrbDecoder
      size_t nPixel = plane.HitPixels();

      // prepare a new TrackerData for the ZS data
      zsFrame= new TrackerDataImpl;
      zsDataEncoder.setCellID(zsFrame);

      for(int i=0; i<  plane.HitPixels() ; i++) {	  
	zsFrame->chargeValues().push_back(plane.GetX(i));
	zsFrame->chargeValues().push_back(plane.GetY(i));
	zsFrame->chargeValues().push_back(1);
	zsFrame->chargeValues().push_back(0);
      }
      zsDataCollection->push_back( zsFrame);

     
    }
    
    /*if (!rawDataCollectionExists) {
      if (rawDataCollection->size() != 0)
	result.addCollection(rawDataCollection, "rawdata");
      else
	delete rawDataCollection; // clean up if not storing the collection here
    }*/

    // add the collections to the event only if not empty and not yet there
    //if (!zsDataCollectionExists) {
      if (zsDataCollection->size() != 0)
	result.addCollection(zsDataCollection, "zsdata_m26");
      else
	delete zsDataCollection; // clean up if not storing the collection here
    //}

    /*if (!zs2DataCollectionExists) {
      if (zs2DataCollection->size() != 0)
	result.addCollection(zs2DataCollection, "zsdata_m26");
      else
	delete zs2DataCollection; // clean up if not storing the collection here
    }*/
    
    return true;
  
  }



  bool NiRawEvent2LCEventConverter::GetStandardSubEvent(StandardEvent &result,
							const Event &source) const {
    static const std::vector<uint32_t> m_ids = {0, 1, 2, 3, 4, 5}; //TODO

    // If we get here it must be a data event
    const RawEvent &rawev = dynamic_cast<const RawEvent &>(source);
    if (rawev.NumBlocks() < 2 || rawev.GetBlock(0).size() < 20 ||
	rawev.GetBlock(1).size() < 20) {
      EUDAQ_WARN("Ignoring bad event " + to_string(source.GetEventNumber()));
      return false;
    }
    const datavect &data0 = rawev.GetBlock(0);
    const datavect &data1 = rawev.GetBlock(1);
    unsigned header0 = GET(data0, 0);
    unsigned header1 = GET(data1, 0);

    unsigned tluid;;
    if (rawev.NumBlocks() < 1 || rawev.GetBlock(0).size() < 8)
      tluid = (unsigned)-1;
    else
      tluid = GET(rawev.GetBlock(0), 1) >> 16;

    if (dbg)
      std::cout << "TLU id = " << hexdec(tluid, 4) << std::endl;
    unsigned pivot = GET(data0, 1) & 0xffff;
    if (dbg)
      std::cout << "Pivot = " << hexdec(pivot, 4) << std::endl;
    datait it0 = data0.begin() + 8;
    datait it1 = data1.begin() + 8;
    unsigned board = 0;
    while (it0 < data0.end() && it1 < data1.end()) {
      unsigned id = board;
      if (id < m_ids.size())
	id = m_ids[id];
      if (dbg)
	std::cout << "Sensor " << board << ", id = " << id << std::endl;
      if (dbg)
	std::cout << "Mimosa_header0 = " << hexdec(header0) << std::endl;
      if (dbg)
	std::cout << "Mimosa_header1 = " << hexdec(header1) << std::endl;
      if (it0 + 2 >= data0.end()) {
	EUDAQ_WARN("Trailing rubbish in first frame");
	break;
      }
      if (it1 + 2 >= data1.end()) {
	EUDAQ_WARN("Trailing rubbish in second frame");
	break;
      }
      if (dbg)
	std::cout << "Mimosa_framecount0 = " << hexdec(GET(it0, 0))
		  << std::endl;
      if (dbg)
	std::cout << "Mimosa_framecount1 = " << hexdec(GET(it1, 0))
		  << std::endl;
      unsigned len0 = GET(it0, 1);
      if (dbg)
	std::cout << "Mimosa_wordcount0 = " << hexdec(len0 & 0xffff, 4)
		  << ", " << hexdec(len0 >> 16, 4) << std::endl;
      if ((len0 & 0xffff) != (len0 >> 16)) {
	EUDAQ_WARN("Mismatched lengths decoding first frame (" +
		   to_string(len0 & 0xffff) + ", " + to_string(len0 >> 16) +
		   ")");
	len0 = std::max(len0 & 0xffff, len0 >> 16);
      }
      len0 &= 0xffff;
      unsigned len1 = GET(it1, 1);
      if (dbg)
	std::cout << "Mimosa_wordcount1 = " << hexdec(len1 & 0xffff, 4)
		  << ", " << hexdec(len1 >> 16, 4) << std::endl;
      if ((len1 & 0xffff) != (len1 >> 16)) {
	EUDAQ_WARN("Mismatched lengths decoding second frame (" +
		   to_string(len1 & 0xffff) + ", " + to_string(len1 >> 16) +
		   ")");
	len1 = std::max(len1 & 0xffff, len1 >> 16);
      }
      len1 &= 0xffff;

      //        for(unsigned i=0;i<len0;i++) std::cout << "0:i="<<i <<
      //        hexdec(GET(it0, i)) << std::endl;
      //        for(unsigned i=0;i<len1;i++) std::cout << "1:i="<<i <<
      //        hexdec(GET(it1, i)) << std::endl;

      if (len0 * 4 + 12 > static_cast<unsigned>(data0.end() - it0)) {
	EUDAQ_WARN("Bad length in first frame");
	break;
      }
      if (len1 * 4 + 12 > static_cast<unsigned>(data1.end() - it1)) {
	EUDAQ_WARN("Bad length in second frame");
	break;
      }
      StandardPlane plane(id, "NI", "MIMOSA26");
      plane.SetSizeZS(1152, 576, 0, 2, StandardPlane::FLAG_WITHPIVOT |
		      StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      // plane.SetTLUEvent(tluid);
      plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
      DecodeFrame(plane, len0, it0 + 8, 0);
      DecodeFrame(plane, len1, it1 + 8, 1);
      result.AddPlane(plane);

      if (dbg)
	std::cout << "Mimosa_trailer0 = " << hexdec(GET(it0, len0 + 2))
		  << std::endl;
      //        if (dbg) std::cout << "Mimosa        0 = " << hexdec(GET(it0,
      //        0)) << " len0 = " << len0 << " by " << (len0*4+16)
      //        <<std::endl;
      if (dbg)
	std::cout << "Mimosa_trailer1 = " << hexdec(GET(it1, len1 + 2))
		  << std::endl;
      //        if (dbg) std::cout << "Mimosa        1 = " << hexdec(GET(it1,
      //        0)) << " len1 = " << len1 << " by " << (len1*4+16)
      //        <<std::endl;

      //      it0  = it0 + len0*4 + 16; std::cout << " done 0 \n";
      //      it1  = it1 + len1*4 + 16; std::cout << " done 1 \n";

      // add len*4+16 untill reching the end of data block, if reached break!

      bool advance_one_block_0 = false;
      bool advance_one_block_1 = false;

      if (it0 < data0.end() - (len0 + 4) * 4) {
	advance_one_block_0 = true;
      }

      if (it1 < data1.end() - (len1 + 4) * 4) {
	advance_one_block_1 = true;
      }

      if (advance_one_block_0 && advance_one_block_1) {
	it0 = it0 + (len0 + 4) * 4;
	if (dbg)
	  std::cout << " done 0 \n";
	if (it0 <= data0.end())
	  header0 = GET(it0, -1);

	it1 = it1 + (len1 + 4) * 4;
	if (dbg)
	  std::cout << " done 1 \n";
	if (it1 <= data1.end())
	  header1 = GET(it1, -1);
      } else {
	if (dbg)
	  std::cout << "advance_one_block_0 = " << advance_one_block_0
		    << std::endl;
	if (dbg)
	  std::cout << "advance_one_block_1 = " << advance_one_block_1
		    << std::endl;
	if (dbg)
	  std::cout << "break the block" << std::endl;
	break;
      }

      //        std::cout << " to " << hexdec(GET(data0.end(),0 )) <<
      //        std::endl;
      //        std::cout << " to " << hexdec(GET(data1.end(),0 )) <<
      //        std::endl;

      ++board;
    }
    return true;
  }

  
  void NiRawEvent2LCEventConverter::DecodeFrame(StandardPlane &plane, size_t len, datait it,
						int frame) const {
    std::vector<unsigned short> vec;
    for (size_t i = 0; i < len; ++i) {
      unsigned v = GET(it, i);
      vec.push_back(v & 0xffff);
      vec.push_back(v >> 16);
    }

    unsigned npixels = 0;
    for (size_t i = 0; i < vec.size(); ++i) {
      //  std::cout << "  " << i << " : " << hexdec(vec[i]) << std::endl;
      if (i == vec.size() - 1)
	break;
      unsigned numstates = vec[i] & 0xf;
      unsigned row = vec[i] >> 4 & 0x7ff;
      if (numstates + 1 > vec.size() - i) {
	// Ignoring bad line
	// std::cout << "Ignoring bad line " << row << " (too many states)" <<
	// std::endl;
	break;
      }
      if (dbg > 2)
	std::cout << "Hit line " << (vec[i] & 0x8000 ? "* " : ". ") << row
		  << ", states " << numstates << ":";
      bool pivot = (row >= (plane.PivotPixel() / 16));
      for (unsigned s = 0; s < numstates; ++s) {
	unsigned v = vec.at(++i);
	unsigned column = v >> 2 & 0x7ff;
	unsigned num = v & 3;
	if (dbg > 2)
	  std::cout << (s ? "," : " ") << column;
	if (dbg > 2)
	  if ((v & 3) > 0)
	    std::cout << "-" << (column + num);
	for (unsigned j = 0; j < num + 1; ++j) {
	  plane.PushPixel(column + j, row, 1, pivot, frame);
	}
	npixels += num + 1;
      }
      if (dbg > 2)
	std::cout << std::endl;
    }
    if (dbg)
      std::cout << "Total pixels " << frame << " = " << npixels << std::endl;
    //++offset;
  }


}
