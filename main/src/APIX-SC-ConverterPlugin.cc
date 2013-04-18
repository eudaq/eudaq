#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#include <iostream>
#include <string>
#include <vector>

namespace eudaq {

  /********************************************/

  class APIXSCConverterPlugin : public DataConverterPlugin {
    public:
      virtual void Initialize(const Event & e, const Configuration & c);
      //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;
      virtual unsigned GetTriggerID(eudaq::Event const &) const;
      virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

    private:
      StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const;
      APIXSCConverterPlugin() : DataConverterPlugin("APIX-SC"),
      m_NumRows(160), m_NumColumns(18),
      m_InitialRow(0), m_InitialColumn(0) {}
      unsigned m_NumRows, m_NumColumns, m_InitialRow , m_InitialColumn;

      static APIXSCConverterPlugin const m_instance;
  };

  APIXSCConverterPlugin const APIXSCConverterPlugin::m_instance;

  void APIXSCConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_NumRows = from_string(source.GetTag("NumRows"), 160);
    m_NumColumns = from_string(source.GetTag("NumColumns"), 18);
    m_InitialRow = from_string(source.GetTag("InitialRow"), 0);
    m_InitialColumn = from_string(source.GetTag("InitialColumn"), 0);

    std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  unsigned APIXSCConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
    // Untested (copypasta from APIX-MC)
    const RawDataEvent & rev = dynamic_cast<const RawDataEvent &>(ev);
    if (rev.NumBlocks() < 1) return (unsigned)-1;
    unsigned tid = getlittleendian<unsigned int>(&rev.GetBlock(0)[4]);
    // Events that should have tid=0 have rubbish instead
    // so if number is illegal, convert it to zero.
    if (tid >= 0x8000) return 0;
    return tid;
  }

  bool APIXSCConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    for (size_t i = 0; i < ev.NumBlocks(); ++i) {
      result.AddPlane(ConvertPlane(ev.GetBlock(i),
            ev.GetID(i)));
      std::cout << "Danach" << std::endl;
    }
    return true;
  }

  StandardPlane APIXSCConverterPlugin::ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
    StandardPlane plane(id, "APIX", "APIX");
    //unsigned npixels = m_NumRows * m_NumColumns;

    // Size 18x160, no pixels preallocated, one frame, each frame has its own coordinates, and all frames should be accumulated
    plane.SetSizeZS(18, 160, 0, 1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

    for (size_t i = 0; i < data.size(); i += 4) {
      unsigned one_line = getlittleendian<unsigned int>(&data[i]);
      if ((one_line & 0x80000001)!=0x80000001 && i > 12 && (one_line & 0x0f000000)>>20 != 0xf0) {
        //std::cout << "DATEN!!!" << std::endl;
        //std::cout << " 0x" << std::hex << one_line <<  std::endl;
        //ypos = (one_line & 0x0ff00000)>>20;
        int ypos = (one_line >>20) & 0xff;

        //xpos =(one_line & 0x000f800)>>11;
        int xpos =(one_line >> 15 ) & 0x1f;
        //tot=(one_line & 0xff);
        int tot = (one_line >> 7) & 0xff;
        //std::cout << xpos << " " <<ypos << " " <<tot << std::endl;
        unsigned subtrigger = 0;
        plane.PushPixel(xpos, ypos, tot, subtrigger);
      }
    }
    return plane;
  }

} //namespace eudaq
