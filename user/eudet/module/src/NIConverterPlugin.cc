#ifdef WIN32
#define NOMINMAX
#endif

#include "DataConverterPlugin.hh"
#include "Exception.hh"
#include "RawDataEvent.hh"
#include "Configuration.hh"
#include "Logger.hh"
#include "proc.hh"

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

#include <array>

#define GET(d, i) getlittleendian<unsigned>(&(d)[(i)*4])
#define _DEBUG_OUT if (dbg > 0) std::cout
#define _DEBUG_OUT_2 if (dbg > 2) std::cout


namespace eudaq {
  class NIConverterPlugin;

  namespace {
    auto dummy0 = Factory<DataConverterPlugin>::Register<NIConverterPlugin>(cstr2hash("NI"));
  }
  inline bool isBORE_or_EORE(const eudaq::Event& source) {

    if (source.IsBORE()) {
      std::cout << "GetStandardSubEvent : got BORE" << std::endl;
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      std::cout << "GetStandardSubEvent : got EORE" << std::endl;
      // nothing to do
      return true;
    }

    return false;

  }





  static const int dbg = 0; // 0=off, 1=structure, 2=structure+data
  static const int PIVOTPIXELOFFSET = 64;





  inline bool isBadEvent(const eudaq::RawDataEvent& rawev) {

    if (rawev.NumBlocks() != 2
      ||
      rawev.GetBlock(0).size() < 20
      ||
      rawev.GetBlock(1).size() < 20) {
      EUDAQ_WARN("Ignoring bad event " + to_string(rawev.GetEventNumber()));
      return true;
    }
    return false;
  }


  inline unsigned getPlaneID(const std::vector<int>& ids, unsigned board) {
    unsigned id = board;

    if (id < ids.size()) { id = ids[id]; }

    _DEBUG_OUT << "Sensor " << board << ", id = " << id << std::endl;

    return id;
  }







  inline StandardPlane make_Standardplane(unsigned planeID, unsigned TLUID, unsigned pivot) {
    StandardPlane plane(planeID, "NI", "MIMOSA26");
    plane.SetSizeZS(1152, 576, 0, 2, StandardPlane::FLAG_WITHPIVOT |
      StandardPlane::FLAG_DIFFCOORDS);
    plane.SetTLUEvent(TLUID);
    plane.SetPivotPixel((9216 + pivot + PIVOTPIXELOFFSET) % 9216);
    return plane;
  }


  DEFINE_PROC1(push_hit_to_plane, nextProc, decoder) {

    decoder.set_pivot((decoder.get_row() >= (decoder.get_plane().PivotPixel() / 16)));

    for (unsigned s = 0; s < decoder.get_numstates(); ++s) {
      decoder.push_to_plane(s);
    }

    return nextProc(decoder);
  }
  DEFINE_PROC1(loop_over_frame_decoder, t, decoder) {

    while (decoder.next()) {
      if (t(decoder) != success) {
        return procReturn::stop_;
      };
    }

    return procReturn::success;
  }




  DEFINE_PROC1(DEBUG_OUT_2_Hit_line, t, decoder) {
    decoder.DEBUG_OUT_2_Hit_line();
    auto ret = t(decoder);
    _DEBUG_OUT_2 << std::endl;
    _DEBUG_OUT << "Total pixels " << decoder.get_frame() << " = " << decoder.get_npixels() << std::endl;
    return  ret;

  }
  class NIConverterPlugin : public DataConverterPlugin {
    typedef std::vector<unsigned char> datavect;
    typedef std::vector<unsigned char>::const_iterator datait;

  public:
    NIConverterPlugin() : DataConverterPlugin("NI"), m_boards(0) {}
    virtual void Initialize(const Event &bore, const Configuration & /*c*/);
    virtual unsigned GetTriggerID(Event const &ev) const;
    virtual bool GetStandardSubEvent(StandardEvent &result, const Event &source) const;
    class NI_block_Decoder {
    public:
      NI_block_Decoder(const datavect &data, int blockID);



      bool hasTrailing_Rubbish() const;

      bool isValid() const;
      unsigned getPivot() const;
      void DEBUG_OUT_Mimosa_header() const;

      void DEBUG_OUT_Mimosa_framecount() const;


      bool calc_Mimosa_Wordcount_Length();


      unsigned getLen() const;

      void DEBUG_OUT_Mimosa_trailer() const;
      datait getItterator() const;

      bool can_Advance_one_block() const;
      void do_advance_one_block();

      void DEBUG_OUT_advance_one_block() const;



      unsigned getBlockId() const;

    private:
      static unsigned getHeader(const datavect &  vec);

      static std::string getFrameName(int frameID);
      static unsigned getPivot_(const datavect & vec);
      void calc_Mimosa_wordcount();

      bool hasBad_Mimosa_Wordcount_Length() const;


      const datavect & m_data;
      unsigned m_header, m_len = 0;
      datait m_it;
      const int m_blockID;
      const unsigned m_pivot;
    };


    class NI_frame_decoder {
    public:
      NI_frame_decoder(NI_block_Decoder& block, StandardPlane &plane);

      bool next();
      void DEBUG_OUT_2_Hit_line()const;
      unsigned get_row() const;
      unsigned get_numstates() const;
      void push_to_plane(const unsigned s);
      void set_pivot(bool pivot_);
      unsigned get_npixels() const;
      int get_frame() const;
      unsigned short at(size_t index) const;
      const StandardPlane& get_plane() const;
      StandardPlane& get_plane();
      NI_block_Decoder& get_block();
    private:
      const size_t m_len;
      const int frame;
      datait m_it;

      StandardPlane &m_plane;
      unsigned npixels = 0;
      unsigned numstates = 0;
      unsigned row = 0;
      bool pivot = 0;
      size_t m_index = 0;
      bool first = true;
      NI_block_Decoder& m_block;
    };


  public:
    virtual ~NIConverterPlugin() {}






    class push_to_Event {
    public:
      push_to_Event(StandardEvent &result) :m_result(result) {

      }
      template <typename NEXT_T>
      procReturn operator()(NEXT_T&& t, const StandardPlane& plane) {
        m_result.AddPlane(plane);
        return t(plane);
      }
      StandardEvent & m_result;
    };

    template <typename DECODER_T>
    class convert_to_plane_impl {

    public:

      convert_to_plane_impl(unsigned tluID, DECODER_T&& decoder) :m_tluID(tluID), m_decoder(std::move(decoder)) {}

      template <typename NEXT_T, typename BLOCKS_T, typename ID_T>
      procReturn operator()(NEXT_T&& t, BLOCKS_T& blocks, const ID_T& id) const {


        auto plane = make_Standardplane(id, m_tluID, blocks[0].getPivot());
        for (auto&e : blocks) {

          //DecodeFrame(plane, e.getLen(), e.getItterator() + 8, e.getBlockId());
          //m_decoder(e, plane);

          //auto  t = proc() >> check_Block_integrity();

          if (m_decoder(NI_frame_decoder(e, plane)) != success) {
            return stop_;
          };
        }
        return t(plane);
      }
      const unsigned m_tluID;
      mutable DECODER_T m_decoder;
    };

    template<typename T>
    auto convert_to_plane(unsigned tluID, T&& decoder) const {
      return convert_to_plane_impl<typename std::remove_reference<T>::type>(tluID, std::forward<T>(decoder));
    }
    class loop_over_blocks {
    public:
      loop_over_blocks(const std::vector<int>& vec) :m_ids(vec) {


      }

      template <typename T>
      static bool isValid(const T& blocks) {
        for (const auto& e : blocks) {
          if (!e.isValid()) {
            return false;
          }
        }
        return true;
      }

      template <typename T>
      static bool can_Advance_one_block(T& blocks) {
        for (const auto& e : blocks) {
          if (!e.can_Advance_one_block()) {
            return false;
          }
        }
        return true;
      }
      template <typename T>
      static bool advanceBlocks(T& blocks) {
        if (can_Advance_one_block(blocks)) {
          for (auto& e : blocks) {
            e.do_advance_one_block();
          }

        } else {
          for (const auto& e : blocks) {
            e.DEBUG_OUT_advance_one_block();
          }
          _DEBUG_OUT << "break the block" << std::endl;
          return false;
        }
        return true;
      }
      template <typename BLOCKS_T, typename NEXT_T>
      procReturn operator()(NEXT_T&& t, BLOCKS_T& blocks) const {
        unsigned board = 0;
        while (isValid(blocks)) {

          if (t(blocks, getPlaneID(m_ids, board)) != procReturn::success) {
            return procReturn::stop_;
          }



          if (!advanceBlocks(blocks)) {
            return procReturn::stop_;
          }



          ++board;
        }

      }
      const std::vector<int>& m_ids;
    };














#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & /*header*/,
                                  eudaq::Event const & /*bore*/,
                                  eudaq::Configuration const & /*conf*/) const;
    virtual bool GetLCIOSubEvent(lcio::LCEvent &result,
                                 const Event &source) const;
#endif
  private:
    unsigned m_boards;
    std::vector<int> m_ids;
  };

  void NIConverterPlugin::Initialize(const Event &bore, const Configuration & /*c*/) {
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
  
  
  unsigned NIConverterPlugin::GetTriggerID(Event const &ev) const {
    const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(ev);
    if (rawev.NumBlocks() < 1 || rawev.GetBlock(0).size() < 8)
      return (unsigned)-1;
    return GET(rawev.GetBlock(0), 1) >> 16;
  }


  DEFINE_PROC1(check_Block_integrity, nextProc, d) {
    auto& block = d.get_block();
    auto& plane = d.get_plane();
    block.DEBUG_OUT_Mimosa_header();



    if (block.hasTrailing_Rubbish()) {
      return procReturn::stop_;
    }

    block.DEBUG_OUT_Mimosa_framecount();


    if (!block.calc_Mimosa_Wordcount_Length()) {
      return procReturn::stop_;
    }



    //DecodeFrame(plane, e.getLen(), e.getItterator() + 8, e.getBlockId());


    auto ret = nextProc(NIConverterPlugin::NI_frame_decoder(block, plane));
    block.DEBUG_OUT_Mimosa_trailer();
    return ret;
  }
  bool NIConverterPlugin::GetStandardSubEvent(StandardEvent &result, const Event &source) const {
    if (isBORE_or_EORE(source)) { return true; }


    _DEBUG_OUT << "GetStandardSubEvent : data" << std::endl;

    // If we get here it must be a data event
    const RawDataEvent &rawev = dynamic_cast<const RawDataEvent &>(source);
    if (isBadEvent(rawev)) { return false; }





    auto tluid = GetTriggerID(source);

    _DEBUG_OUT << "TLU id = " << hexdec(tluid, 4) << std::endl;


    std::array<NI_block_Decoder, 2> blocks = { NI_block_Decoder(rawev.GetBlock(0), 0), NI_block_Decoder(rawev.GetBlock(1), 1) };


    blocks | proc()
      >> loop_over_blocks(m_ids)
      >> convert_to_plane(tluid,
        proc() >> check_Block_integrity() >> loop_over_frame_decoder() >> DEBUG_OUT_2_Hit_line() >> push_hit_to_plane()
      )
      >> push_to_Event(result);





    return true;
  }


  NIConverterPlugin::NI_block_Decoder::NI_block_Decoder(const datavect &data, int blockID) :m_data(data),
    m_header(getHeader(data)), m_it(data.begin() + 8),
    m_blockID(blockID), m_pivot(getPivot_(data)) {

  }



  bool NIConverterPlugin::NI_block_Decoder::hasTrailing_Rubbish() const {
    if (m_it + 2 >= m_data.end()) {
      EUDAQ_WARN("Trailing rubbish in " + getFrameName(m_blockID) + " frame");
      return true;
    }

    return false;
  }

  bool NIConverterPlugin::NI_block_Decoder::isValid() const {
    if (m_it < m_data.end()) {
      return true;
    }
    return false;
  }

  unsigned NIConverterPlugin::NI_block_Decoder::getPivot() const {
    return m_pivot;
  }

  void NIConverterPlugin::NI_block_Decoder::DEBUG_OUT_Mimosa_header() const {
    _DEBUG_OUT << "Mimosa_header" << m_blockID << " = " << hexdec(m_header) << std::endl;
  }

  void NIConverterPlugin::NI_block_Decoder::DEBUG_OUT_Mimosa_framecount() const {
    _DEBUG_OUT << "Mimosa_framecount" << m_blockID << " = " << hexdec(GET(m_it, 0)) << std::endl;
  }

  bool NIConverterPlugin::NI_block_Decoder::calc_Mimosa_Wordcount_Length() {
    calc_Mimosa_wordcount();
    if (hasBad_Mimosa_Wordcount_Length()) {
      return false;
    }
    return true;
  }

  unsigned NIConverterPlugin::NI_block_Decoder::getLen() const {
    return m_len;
  }

  void NIConverterPlugin::NI_block_Decoder::DEBUG_OUT_Mimosa_trailer() const {
    _DEBUG_OUT << "Mimosa_trailer0 = " << hexdec(GET(m_it, m_len + 2)) << std::endl;
  }

  eudaq::NIConverterPlugin::datait NIConverterPlugin::NI_block_Decoder::getItterator() const {
    return m_it;
  }

  bool NIConverterPlugin::NI_block_Decoder::can_Advance_one_block() const {
    if (m_it < m_data.end() - (m_len + 4) * 4) {
      return true;
    }
    return false;
  }

  void NIConverterPlugin::NI_block_Decoder::do_advance_one_block() {
    m_it = m_it + (m_len + 4) * 4;
    _DEBUG_OUT << " done 0 \n";
    if (m_it <= m_data.end())
      m_header = GET(m_it, -1);
  }

  void NIConverterPlugin::NI_block_Decoder::DEBUG_OUT_advance_one_block() const {
    _DEBUG_OUT << "advance_one_block_" << m_blockID << " " << can_Advance_one_block()
      << std::endl;
  }





  unsigned NIConverterPlugin::NI_block_Decoder::getBlockId() const {
    return m_blockID;
  }

  unsigned NIConverterPlugin::NI_block_Decoder::getHeader(const datavect & vec) {
    return GET(vec, 0);
  }

  std::string NIConverterPlugin::NI_block_Decoder::getFrameName(int frameID) {
    if (frameID == 0) {
      return "first";
    }
    if (frameID == 1) {
      return "second";
    }

    EUDAQ_THROW("unknown frame ID=" + to_string(frameID));
    return "error";
  }

  unsigned NIConverterPlugin::NI_block_Decoder::getPivot_(const datavect & vec) {
    unsigned pivot = GET(vec, 1) & 0xffff;
    _DEBUG_OUT << "Pivot = " << hexdec(pivot, 4) << std::endl;
    return pivot;
  }

  void NIConverterPlugin::NI_block_Decoder::calc_Mimosa_wordcount() {
    unsigned len0 = GET(m_it, 1);

    _DEBUG_OUT << "Mimosa_wordcount" << m_blockID << " = " << hexdec(len0 & 0xffff, 4)
      << ", " << hexdec(len0 >> 16, 4) << std::endl;


    if ((len0 & 0xffff) != (len0 >> 16)) {
      EUDAQ_WARN("Mismatched lengths decoding " + getFrameName(m_blockID) + " frame (" +
        to_string(len0 & 0xffff) + ", " + to_string(len0 >> 16) + ")");
      len0 = std::max(len0 & 0xffff, len0 >> 16);
    }
    len0 &= 0xffff;
    m_len = len0;
  }

  bool NIConverterPlugin::NI_block_Decoder::hasBad_Mimosa_Wordcount_Length() const {
    if (m_len * 4 + 12 > static_cast<unsigned>(m_data.end() - m_it)) {
      EUDAQ_WARN("Bad length in " + getFrameName(m_blockID) + " frame");
      return true;
    }
    return false;
  }




  NIConverterPlugin::NI_frame_decoder::NI_frame_decoder(NI_block_Decoder& block, StandardPlane &plane) :
    m_block(block), m_plane(plane), m_len(block.getLen()), m_it(block.getItterator() + 8), frame(block.getBlockId()) {

  }

  bool NIConverterPlugin::NI_frame_decoder::next() {
    auto i = m_index;
    if (m_len == 0) {
      return false;
    }
    if (first) {
      first = false;
    } else {
      i += get_numstates() + 1;
      m_index = i;
    }
    if (i >= m_len * 2 - 1) {
      return false;
    }
    numstates = at(i) & 0xf;
    row = at(i) >> 4 & 0x7ff;
    if (numstates + 1 > m_len * 2 - i) {
      return false;
    }

    return true;
  }
  void NIConverterPlugin::NI_frame_decoder::DEBUG_OUT_2_Hit_line() const {
    _DEBUG_OUT_2 << "Hit line " << (at(m_index) & 0x8000 ? "* " : ". ") << row << ", states " << numstates << ":";
  }

  unsigned NIConverterPlugin::NI_frame_decoder::get_row() const {
    return row;
  }

  unsigned NIConverterPlugin::NI_frame_decoder::get_numstates() const {
    return numstates;
  }

  void NIConverterPlugin::NI_frame_decoder::push_to_plane(const unsigned s) {
    auto i = m_index + s + 1;
    unsigned v = at(i);

    unsigned column = v >> 2 & 0x7ff;
    unsigned num = v & 3;
    _DEBUG_OUT_2 << (s ? "," : " ") << column;

    if (dbg > 2 && (v & 3) > 0) std::cout << "-" << (column + num);


    for (unsigned j = 0; j < num + 1; ++j) {

      m_plane.PushPixel(column + j, row, 1, pivot, frame);
    }
    npixels += num + 1;
  }

  void NIConverterPlugin::NI_frame_decoder::set_pivot(bool pivot_) {
    pivot = pivot_;
  }

  unsigned NIConverterPlugin::NI_frame_decoder::get_npixels() const {
    return npixels;
  }

  int NIConverterPlugin::NI_frame_decoder::get_frame() const {
    return frame;
  }

  unsigned short NIConverterPlugin::NI_frame_decoder::at(size_t index)  const {

    unsigned v = GET(m_it, index / 2);
    if (index % 2) {
      return v >> 16;
    }

    return v & 0xffff;

  }

  const StandardPlane& NIConverterPlugin::NI_frame_decoder::get_plane() const {
    return m_plane;
  }

  StandardPlane& NIConverterPlugin::NI_frame_decoder::get_plane() {
    return m_plane;
  }

  NIConverterPlugin::NI_block_Decoder& NIConverterPlugin::NI_frame_decoder::get_block() {
    return m_block;
  }









#if USE_LCIO && USE_EUTELESCOPE
  void NIConverterPlugin::GetLCIORunHeader(
      lcio::LCRunHeader &header, eudaq::Event const & /*bore*/,
      eudaq::Configuration const & /*conf*/) const {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
    runHeader.setDAQHWName(EUTELESCOPE::EUDRB); // should be:

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
      _DEBUG_OUT << "NIConverterPlugin::GetLCIOSubEvent BORE " << std::endl;
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      _DEBUG_OUT << "NIConverterPlugin::GetLCIOSubEvent EORE " << std::endl;
      // nothing to do
      return true;
    }
    // If we get here it must be a data event

    _DEBUG_OUT << "NIConverterPlugin::GetLCIOSubEvent data " << std::endl;
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

            // now add this pixel to the sparse frame
            sparseFrame->addSparsePixel(sparsePixel.get());
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
