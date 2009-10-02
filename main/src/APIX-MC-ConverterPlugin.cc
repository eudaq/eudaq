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

  struct APIXPlanes {
    std::vector<StandardPlane> planes;
    std::vector<int> feids;
    int Find(int id) {
      std::vector<int>::const_iterator it = std::find(feids.begin(), feids.end(), id);
      if (it == feids.end()) return -1;
      else return it - feids.begin();
    }
  };

  class APIXMCConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event & e, const Configuration & c);
    //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

  private:
    APIXPlanes ConvertPlanes(const std::vector<unsigned char> & data) const;
    APIXMCConverterPlugin() : DataConverterPlugin("APIX-MC"),
                              m_NumRows(160), m_NumColumns(18),
                              m_InitialRow(0), m_InitialColumn(0) {}
    unsigned m_NumRows, m_NumColumns, m_InitialRow , m_InitialColumn;

    static APIXMCConverterPlugin const m_instance;
  };

  APIXMCConverterPlugin const APIXMCConverterPlugin::m_instance;

  void APIXMCConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_NumRows = from_string(source.GetTag("NumRows"), 160);
    m_NumColumns = from_string(source.GetTag("NumColumns"), 18);
    m_InitialRow = from_string(source.GetTag("InitialRow"), 0);
    m_InitialColumn = from_string(source.GetTag("InitialColumn"), 0);

    std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  bool APIXMCConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);

    /* Yes this is bit strange...
       in MutiChip-Mode which is used here, all the Data are written in only one block,
       in principal its possible to do it different, but then a lot of data would be doubled.
       The Data in block 1 is exactly what is in the eudet-fifo and the datafifo for on event.
    */
//     for (int FE=0; FE<4; FE++) {
//       result.AddPlane(ConvertPlane(ev.GetBlock(0),ev.GetID(0),FE));
//     }
    APIXPlanes planes = ConvertPlanes(ev.GetBlock(0));
    for (size_t i = 0; i < planes.feids.size(); ++i) {
      result.AddPlane(planes.planes[i]);
    }

    return true;
  }

  APIXPlanes APIXMCConverterPlugin::ConvertPlanes(const std::vector<unsigned char> & data) const {
    APIXPlanes result;
    //StandardPlane plane(id, "APIX", "APIX");
    //unsigned npixels = m_NumRows * m_NumColumns;

    // Size 18x160, no pixels preallocated, one frame, each frame has its own coordinates, and all frames should be accumulated
    //plane.SetSizeZS(18, 160, 0, 1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

    for (size_t i = 0; i < data.size(); i += 4) {
      unsigned one_line = getlittleendian<unsigned int>(&data[i]);
      int chip = (one_line >> 21) & 0xf;
      //std::cout << "Raw Data: " << hexdec(one_line) << ", FE: "<< chip << " (" << FE << ")" << std::endl;
      if ((one_line & 0x80000001)!=0x80000001 && i > 12 && (one_line & 0x02000000)>>25 != 0x1) {

        chip = (one_line >> 21) & 0xf;

        int index = result.Find(chip);
        if (index == -1) {
          index = result.planes.size();
          result.planes.push_back(StandardPlane(chip, "APIX", "APIX"));
          result.feids.push_back(chip);
          result.planes[index].SetSizeZS(18, 160, 0, 1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
        }
        //std::cout << "DATA" << std::endl;
        int ypos = (one_line >>13) & 0xff; //row
        int xpos =(one_line >> 8 ) & 0x1f; //column
        int tot = (one_line) & 0xff;
        unsigned subtrigger = 0;
        result.planes.at(index).PushPixel(xpos, ypos, tot, subtrigger);
      }
    }
    return result;
  }

} //namespace eudaq
