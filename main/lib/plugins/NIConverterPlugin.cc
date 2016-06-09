#ifdef WIN32
#define NOMINMAX
#endif

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "UTIL/CellIDEncoder.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelMimoTelDetector.h"
#include "EUTelMimosa18Detector.h"
#include "EUTelMimosa26Detector.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelRunHeaderImpl.h"
using eutelescope::EUTELESCOPE;
#endif

#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <memory>
#include <iomanip>
#include <algorithm>
#include <functional>

#define GET(d, i) getlittleendian<unsigned>(&(d)[(i)*4])

namespace eudaq {
  static const int dbg = 0; // 0=off, 1=structure, 2=structure+data
  static const int PIVOTPIXELOFFSET = 64;

  class NIConverterPlugin : public DataConverterPlugin {
    typedef std::vector<unsigned char> datavect;
    typedef std::vector<unsigned char>::const_iterator datait;

  public:
    virtual ~NIConverterPlugin() {}

    virtual void Initialize(const Event &bore, const Configuration & /*c*/) {

      m_boards = from_string(bore.GetTag("BOARDS"), 0);
      if (m_boards == 255) {
        m_boards = 6;
      }

      m_ids.clear();
      for (unsigned i = 0; i < m_boards; ++i) {
        unsigned id = from_string(bore.GetTag("ID" + to_string(i)), i);
        m_ids.push_back(id);
      }
    }

    virtual unsigned GetTriggerID(Event const &ev) const {
      const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(ev);
      if (rawev.NumBlocks() < 1 || rawev.GetBlock(0).size() < 8)
        return (unsigned)-1;
      return GET(rawev.GetBlock(0), 1) >> 16;
    }

    virtual bool GetStandardSubEvent(StandardEvent &result,
                                     const Event &source) const {
      if (source.IsBORE()) {
        std::cout << "GetStandardSubEvent : got BORE" << std::endl;
        // shouldn't happen
        return true;
      } else if (source.IsEORE()) {
        std::cout << "GetStandardSubEvent : got EORE" << std::endl;
        // nothing to do
        return true;
      }

      if (dbg > 0)
        std::cout << "GetStandardSubEvent : data" << std::endl;

      // If we get here it must be a data event
      const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(source);
      if (rawev.NumBlocks() != 2 || rawev.GetBlock(0).size() < 20 ||
          rawev.GetBlock(1).size() < 20) {
        EUDAQ_WARN("Ignoring bad event " + to_string(source.GetEventNumber()));
        return false;
      }
      const datavect &data0 = rawev.GetBlock(0);
      const datavect &data1 = rawev.GetBlock(1);
      unsigned header0 = GET(data0, 0);
      unsigned header1 = GET(data1, 0);
      unsigned tluid = GetTriggerID(source);
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
                                             StandardPlane::FLAG_DIFFCOORDS);
        plane.SetTLUEvent(tluid);
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

    void DecodeFrame(StandardPlane &plane, size_t len, datait it,
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

#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & /*header*/,
                                  eudaq::Event const & /*bore*/,
                                  eudaq::Configuration const & /*conf*/) const;
    virtual bool GetLCIOSubEvent(lcio::LCEvent &result,
                                 const Event &source) const;
#endif

  protected:
    // static size_t GetID(const Event & event, size_t i) {
    //  if (const RawDataEvent * ev = dynamic_cast<const RawDataEvent
    //  *>(&event)) {
    //    return ev->GetID(i);
    //  }
    //  return 0;
    //}
    // static size_t NumPlanes(const Event & event) {
    //  const RawDataEvent & rawev = dynamic_cast<const RawDataEvent &>(ev);
    //  if (rawev.NumBlocks() < 1) return 0;
    //  const std::vector<unsigned char> & data = rawev.GetBlock(0);
    //  size_t offset = 2, result = 0;
    //  while (data.size()/4 >= offset + 3) {
    //    unsigned len = GET(data, offset + 1) & 0xffff;
    //    offset += len + 4;
    //  }
    //}
  private:
    NIConverterPlugin() : DataConverterPlugin("NI"), m_boards(0) {}
    unsigned m_boards;
    std::vector<int> m_ids;
    static NIConverterPlugin const m_instance;
  };

  NIConverterPlugin const NIConverterPlugin::m_instance;

#if USE_LCIO && USE_EUTELESCOPE
  void NIConverterPlugin::GetLCIORunHeader(
      lcio::LCRunHeader &header, eudaq::Event const & /*bore*/,
      eudaq::Configuration const & /*conf*/) const {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
    runHeader.setDAQHWName(EUTELESCOPE::EUDRB); // should be:
    // runHeader.setDAQHWName(EUTELESCOPE::NI);

    // the information below was used by EUTelescope before the
    // introduction of the BUI. Now all these parameters shouldn't be
    // used anymore but they are left here for backward compatibility.

    runHeader.setEUDRBMode("ZS2");
    runHeader.setEUDRBDet("MIMOSA26");
    runHeader.setNoOfDetector(m_boards);
    std::vector<int> xMin(m_boards, 0), xMax(m_boards, 1151), yMin(m_boards, 0),
        yMax(m_boards, 575);
    runHeader.setMinX(xMin);
    runHeader.setMaxX(xMax);
    runHeader.setMinY(yMin);
    runHeader.setMaxY(yMax);
  }

  bool NIConverterPlugin::GetLCIOSubEvent(lcio::LCEvent &result,
                                          const Event &source) const {
    if (source.IsBORE()) {
      if (dbg > 0)
        std::cout << "NIConverterPlugin::GetLCIOSubEvent BORE " << std::endl;
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      if (dbg > 0)
        std::cout << "NIConverterPlugin::GetLCIOSubEvent EORE " << std::endl;
      // nothing to do
      return true;
    }
    // If we get here it must be a data event

    if (dbg > 0)
      std::cout << "NIConverterPlugin::GetLCIOSubEvent data " << std::endl;
    result.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                 eutelescope::kDE);

    // prepare the collections for the rawdata and the zs ones
    LCCollectionVec *rawDataCollection, *zsDataCollection, *zs2DataCollection;
    bool rawDataCollectionExists = false, zsDataCollectionExists = false,
         zs2DataCollectionExists = false;

    try {
      rawDataCollection =
          static_cast<LCCollectionVec *>(result.getCollection("rawdata"));
      rawDataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      rawDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERRAWDATA);
    }

    try {
      zsDataCollection =
          static_cast<LCCollectionVec *>(result.getCollection("zsdata"));
      zsDataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
    }

    try {
      zs2DataCollection =
          static_cast<LCCollectionVec *>(result.getCollection("zsdata_m26"));
      zs2DataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      zs2DataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
    }

    // set the proper cell encoder
    CellIDEncoder<TrackerRawDataImpl> rawDataEncoder(
        eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection);
    CellIDEncoder<TrackerDataImpl> zsDataEncoder(
        eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
    CellIDEncoder<TrackerDataImpl> zs2DataEncoder(
        eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zs2DataCollection);

    // a description of the setup
    std::vector<eutelescope::EUTelSetupDescription *> setupDescription;

    // to understand if we have problem with de-syncronisation, let
    // me prepare a Boolean switch and a vector of size_t to contain the
    // pivot pixel position
    bool outOfSyncFlag = false;
    std::vector<size_t> pivotPixelPosVec;

    if (dbg > 0)
      std::cout
          << "NIConverterPlugin::GetLCIOSubEvent rawDataEvent with boards="
          << m_boards << std::endl;
    const RawDataEvent &rawDataEvent =
        dynamic_cast<const RawDataEvent &>(source);

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
      StandardPlane plane =
          static_cast<StandardPlane>(tmp_evt.GetPlane(iPlane));

      // The current detector is ...
      eutelescope::EUTelPixelDetector *currentDetector = 0x0;
      if (plane.Sensor() == "MIMOTEL") {

        currentDetector = new eutelescope::EUTelMimoTelDetector;
        std::string mode;
        plane.GetFlags(StandardPlane::FLAG_ZS) ? mode = "ZS" : mode = "RAW2";
        currentDetector->setMode(mode);
        if (result.getEventNumber() == 0) {
          setupDescription.push_back(
              new eutelescope::EUTelSetupDescription(currentDetector));
        }
      } else if (plane.Sensor() == "MIMOSA18") {

        currentDetector = new eutelescope::EUTelMimosa18Detector;
        std::string mode;
        plane.GetFlags(StandardPlane::FLAG_ZS) ? mode = "ZS" : mode = "RAW2";
        currentDetector->setMode(mode);
        if (result.getEventNumber() == 0) {
          setupDescription.push_back(
              new eutelescope::EUTelSetupDescription(currentDetector));
        }
      } else if (plane.Sensor() == "MIMOSA26") {

        currentDetector = new eutelescope::EUTelMimosa26Detector;
        std::string mode = "ZS2";
        currentDetector->setMode(mode);
        if (result.getEventNumber() == 0) {
          setupDescription.push_back(
              new eutelescope::EUTelSetupDescription(currentDetector));
        }
      } else {

        EUDAQ_ERROR("Unrecognised sensor type in LCIO converter: " +
                    plane.Sensor());
        return true;
      }
      std::vector<size_t> markerVec = currentDetector->getMarkerPosition();

      if (plane.GetFlags(StandardPlane::FLAG_ZS)) {
        zsDataEncoder["sensorID"] = plane.ID();
        zsDataEncoder["sparsePixelType"] =
            eutelescope::kEUTelGenericSparsePixel;

        // get the total number of pixel. This is written in the
        // eudrbBoard and to get it in a easy way pass through the eudrbDecoder
        size_t nPixel = plane.HitPixels();

        // prepare a new TrackerData for the ZS data
        std::auto_ptr<lcio::TrackerDataImpl> zsFrame(new lcio::TrackerDataImpl);
        zsDataEncoder.setCellID(zsFrame.get());

        // this is the structure that will host the sparse pixel
        std::auto_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<
            eutelescope::EUTelGenericSparsePixel>>
            sparseFrame(new eutelescope::EUTelTrackerDataInterfacerImpl<
                eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

        // prepare a sparse pixel to be added to the sparse data
        std::auto_ptr<eutelescope::EUTelGenericSparsePixel> sparsePixel(
            new eutelescope::EUTelGenericSparsePixel);
        for (size_t iPixel = 0; iPixel < nPixel; ++iPixel) {

          // the data contain also the markers, so we have to strip
          // them out. First I need to have the original position
          // (with markers in) and then calculate how many pixels I
          // have to remove
          size_t originalX = (size_t)plane.GetX(iPixel);

          if (find(markerVec.begin(), markerVec.end(), originalX) ==
              markerVec.end()) {
            // the original X is not on a marker column, so I need
            // to remove a certain number of pixels depending on the
            // position

            // this counts the number of markers found on the left
            // of the original X
            short diff =
                (short)count_if(markerVec.begin(), markerVec.end(),
                                std::bind2nd(std::less<short>(), originalX));
            sparsePixel->setXCoord(originalX - diff);

            // no problem instead with the Y coordinate
            sparsePixel->setYCoord((size_t)plane.GetY(iPixel));

            // last the pixel charge. The CDS is automatically
            // calculated by the EUDRB
            sparsePixel->setSignal((size_t)plane.GetPixel(iPixel));

            // in case of DEBUG
            // streamlog_out ( DEBUG0 ) << ( *(sparsePixel.get() ) ) << endl;

            // now add this pixel to the sparse frame
            sparseFrame->addSparsePixel(*sparsePixel.get());
          } else {
            // the original X was on a marker column, so we don't
            // need to process this pixel any further and of course
            // we don't have to add it to the sparse frame.

            /*
               streamlog_out ( DEBUG0 ) << "Found a sparse pixel ("<< iPixel
               <<")  on a marker column. Not adding it to the frame" << endl
               << (* (sparsePixel.get() ) ) << endl;
             */
          }
        }

        // perfect! Now add the TrackerData to the collection
        if (plane.Sensor() == "MIMOSA26") {
          zs2DataCollection->push_back(zsFrame.release());
        } else {
          zsDataCollection->push_back(zsFrame.release());
        }

        // for the debug of the synchronization
        pivotPixelPosVec.push_back(plane.PivotPixel());

      } else {

        // storage of RAW data is done here according to the mode
        rawDataEncoder["xMin"] = currentDetector->getXMin();
        rawDataEncoder["xMax"] = currentDetector->getXMax() - markerVec.size();
        rawDataEncoder["yMin"] = currentDetector->getYMin();
        rawDataEncoder["yMax"] = currentDetector->getYMax();
        rawDataEncoder["sensorID"] = plane.ID();

        // get the full vector of CDS
        std::vector<short> cdsVec = plane.GetPixels<short>();

        // now we have to strip out the marker cols from the CDS
        // value. To do this I need a vector of short large enough
        // to accommodate the full matrix without the markers
        std::vector<short> cdsStrippedVec(
            currentDetector->getYNoOfPixel() *
            (currentDetector->getXNoOfPixel() - markerVec.size()));

        // I need also two iterators, one for the stripped vec and
        // one for the original one.
        std::vector<short>::iterator currentCDSPos = cdsStrippedVec.begin();
        std::vector<short>::iterator cdsBegin = cdsVec.begin();

        // now loop over all the pixels
        for (size_t y = 0; y < currentDetector->getYNoOfPixel(); ++y) {
          size_t offset = y * currentDetector->getXNoOfPixel();
          std::vector<size_t>::iterator marker = markerVec.begin();

          // first copy from the beginning of the row to the first
          // marker column
          currentCDSPos = copy(cdsBegin + offset,
                               cdsBegin + (*(marker) + offset), currentCDSPos);

          // now copy from the next column to the next marker into a
          // while loop
          while (marker != markerVec.end()) {
            if (marker < markerVec.end() - 1) {
              currentCDSPos =
                  copy(cdsBegin + (*(marker) + 1 + offset),
                       cdsBegin + (*(marker + 1) + offset), currentCDSPos);
            } else {
              // now from the last marker column to the end of the
              // row
              currentCDSPos =
                  copy(cdsBegin + (*(marker) + 1 + offset),
                       cdsBegin + offset + currentDetector->getXNoOfPixel(),
                       currentCDSPos);
            }
            ++marker;
          }
        }

        // this is the right place to prepare the TrackerRawData
        // object
        std::auto_ptr<lcio::TrackerRawDataImpl> cdsFrame(
            new lcio::TrackerRawDataImpl);
        rawDataEncoder.setCellID(cdsFrame.get());

        // add the cds stripped values to the current TrackerRawData
        cdsFrame->setADCValues(cdsStrippedVec);

        // put the pivot pixel in the timestamp field of the
        // TrackerRawData. I know that is not correct, but this is
        // the only place where I can put this info
        cdsFrame->setTime(plane.PivotPixel());

        // this is also the right place to add the pivot pixel to
        // the pivot pixel vector for synchronization checks
        pivotPixelPosVec.push_back(plane.PivotPixel());

        // now append the TrackerRawData object to the corresponding
        // collection releasing the auto pointer
        rawDataCollection->push_back(cdsFrame.release());
      }

      delete currentDetector;
    }

    if (result.getEventNumber() == 0) {

      // do this only in the first event

      LCCollectionVec *eudrbSetupCollection = NULL;
      bool eudrbSetupExists = false;
      try {
        eudrbSetupCollection =
            static_cast<LCCollectionVec *>(result.getCollection("eudrbSetup"));
        eudrbSetupExists = true;
      } catch (lcio::DataNotAvailableException &e) {
        eudrbSetupCollection = new LCCollectionVec(lcio::LCIO::LCGENERICOBJECT);
      }

      for (size_t iPlane = 0; iPlane < setupDescription.size(); ++iPlane) {
        eudrbSetupCollection->push_back(setupDescription.at(iPlane));
      }

      if (!eudrbSetupExists) {
        result.addCollection(eudrbSetupCollection, "eudrbSetup");
      }
    }

    // check if all the boards where running in synchronous mode or
    // not. Remember that the last pivot pixel entry is the one of the
    // master board.
    std::vector<size_t>::iterator masterBoardPivotAddress =
        pivotPixelPosVec.end() - 1;
    std::vector<size_t>::iterator slaveBoardPivotAddress =
        pivotPixelPosVec.begin();
    while (slaveBoardPivotAddress < masterBoardPivotAddress) {
      if (*slaveBoardPivotAddress - *masterBoardPivotAddress >= 2) {
        outOfSyncFlag = true;

        // we don't need to continue looping over all boards if one of
        // them is already out of sync
        break;
      }
      ++slaveBoardPivotAddress;
    }
    if (outOfSyncFlag) {

      if (result.getEventNumber() < 20) {
        // in this case we have the responsibility to tell the user that
        // the event was out of sync
        std::cout << "Event number " << result.getEventNumber()
                  << " seems to be out of sync" << std::endl;
        std::vector<size_t>::iterator masterBoardPivotAddress =
            pivotPixelPosVec.end() - 1;
        std::vector<size_t>::iterator slaveBoardPivotAddress =
            pivotPixelPosVec.begin();
        while (slaveBoardPivotAddress < masterBoardPivotAddress) {
          // print out all the slave boards first
          std::cout << " --> Board (S) " << std::setw(3)
                    << setiosflags(std::ios::right)
                    << slaveBoardPivotAddress - pivotPixelPosVec.begin()
                    << resetiosflags(std::ios::right) << " = " << std::setw(15)
                    << setiosflags(std::ios::right) << (*slaveBoardPivotAddress)
                    << resetiosflags(std::ios::right) << " (" << std::setw(15)
                    << setiosflags(std::ios::right)
                    << (signed)(*masterBoardPivotAddress) -
                           (signed)(*slaveBoardPivotAddress)
                    << resetiosflags(std::ios::right) << ")" << std::endl;
          ++slaveBoardPivotAddress;
        }
        // print out also the master. It is impossible that the master
        // is out of sync with respect to itself, but for completeness...
        std::cout << " --> Board (M) " << std::setw(3)
                  << setiosflags(std::ios::right)
                  << slaveBoardPivotAddress - pivotPixelPosVec.begin()
                  << resetiosflags(std::ios::right) << " = " << std::setw(15)
                  << setiosflags(std::ios::right) << (*slaveBoardPivotAddress)
                  << resetiosflags(std::ios::right) << " (" << std::setw(15)
                  << setiosflags(std::ios::right)
                  << (signed)(*masterBoardPivotAddress) -
                         (signed)(*slaveBoardPivotAddress)
                  << resetiosflags(std::ios::right) << ")" << std::endl;

      } else if (result.getEventNumber() == 20) {
        // if the number of consecutive warnings is equal to the maximum
        // allowed, don't bother the user anymore with this message,
        // because it's very likely the run was taken unsynchronized on
        // purpose
        std::cout
            << "The maximum number of unsychronized events has been reached."
            << std::endl
            << "Assuming the run was taken in asynchronous mode" << std::endl;
      }
    }

    // add the collections to the event only if not empty and not yet there
    if (!rawDataCollectionExists) {
      if (rawDataCollection->size() != 0)
        result.addCollection(rawDataCollection, "rawdata");
      else
        delete rawDataCollection; // clean up if not storing the collection here
    }

    if (!zsDataCollectionExists) {
      if (zsDataCollection->size() != 0)
        result.addCollection(zsDataCollection, "zsdata");
      else
        delete zsDataCollection; // clean up if not storing the collection here
    }

    if (!zs2DataCollectionExists) {
      if (zs2DataCollection->size() != 0)
        result.addCollection(zs2DataCollection, "zsdata_m26");
      else
        delete zs2DataCollection; // clean up if not storing the collection here
    }

    return true;
  }

#endif

} // namespace eudaq
