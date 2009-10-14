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
    void ConvertPlanes(const std::vector<unsigned char> & data, APIXPlanes & result) const;
    unsigned int isPLLTrig(unsigned int word) const;
    unsigned int isEudetTrig(unsigned int word) const;
    APIXMCConverterPlugin() : DataConverterPlugin("APIX-MC"),
                              m_PlaneMask(0) {}
    unsigned m_PlaneMask;

    static APIXMCConverterPlugin const m_instance;
  };

  APIXMCConverterPlugin const APIXMCConverterPlugin::m_instance;

  void APIXMCConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_PlaneMask = from_string(source.GetTag("PlaneMask"), 0);

    //std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    //std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
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
    APIXPlanes planes;
    int feid = 0;
    unsigned mask = m_PlaneMask;
    while (mask) {
      if (mask & 1) {
        std::cout << "APIX plane " << feid << std::endl;
        planes.planes.push_back(StandardPlane(feid, "APIX", "APIX"));
        planes.feids.push_back(feid);
        planes.planes.back().SetSizeZS(18, 160, 0, 16, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
      }
      feid++;
      mask >>= 1;
    }
    ConvertPlanes(ev.GetBlock(0), planes);
    for (size_t i = 0; i < planes.feids.size(); ++i) {
      result.AddPlane(planes.planes[i]);
    }

    return true;
  }

  unsigned int APIXMCConverterPlugin::isPLLTrig(unsigned int word) const {
    if((word & 0xf000000f) == 0x80000001) {
      return (word & 0x00ffff00) >> 8;
    }
    return 0xffffffff;
  }

  unsigned int APIXMCConverterPlugin::isEudetTrig(unsigned int word) const {
    //similar to: if(word & 0x0c000000) >> 26; case 0x1
    if(word & 0x04000000) {
      return word;
    }
    return 0xffffffff;
  }

  void APIXMCConverterPlugin::ConvertPlanes(const std::vector<unsigned char> & data, APIXPlanes & result) const {
    //StandardPlane plane(id, "APIX", "APIX");
    //unsigned npixels = m_NumRows * m_NumColumns;

    // Size 18x160, no pixels preallocated, one frame, each frame has its own coordinates, and all frames should be accumulated
    //plane.SetSizeZS(18, 160, 0, 1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
    
    //Check that the start of the data block has a PLL trigger and the next is the EUDET trigger ID
    unsigned int pll_trig = 0xffffffff;
    unsigned int eudet_trig = 0xffffffff;
    for (size_t i = 0; i < 8; i += 4) {
      unsigned one_line = getlittleendian<unsigned int>(&data[i]);
      //std::cout << "Raw Data: " << hexdec(one_line) << ", FE: "<< chip << " (" << FE << ")" << std::endl;
      if(i<4) {
        if ((pll_trig = isPLLTrig(one_line)) == 0xffffffff) {
          std::cerr << "Error in APIX-MC Converter: First block in the data is not a PLL trigger: " << hexdec(one_line) << std::endl;
        }
      } else {
        if (isPLLTrig(one_line) != 0xffffffff) {
          std::cerr << "Error in APIX-MC Converter: Second block in the data after "<< hexdec(pll_trig) <<" is a PLL trigger: " << hexdec(one_line) << std::endl;
        } else {
          eudet_trig = one_line;
//           if((eudet_trig = isEudetTrig(one_line)) == 0xffffffff) {
//             std::cerr << "Error in APIX-MC Converter: Was expecting a eudet trigger after "<< hexdec(pll_trig) <<" but got " << hexdec(one_line) << std::endl;
//           }
        }
      }
    }
    
    assert(result.planes.size()==result.feids.size());
    for (size_t iplane = 0; iplane < result.feids.size(); ++iplane) {
      result.planes.at(iplane).SetTLUEvent(eudet_trig);
    }
    
    unsigned subtrigger = 0;
    for (size_t i = 16; i < data.size(); i += 4) {
      unsigned one_line = getlittleendian<unsigned int>(&data[i]);
      //std::cout << "Raw Data: " << hexdec(one_line) << ", FE: "<< chip << " (" << FE << ")" << std::endl;
      bool moduleError = false, moduleEoe = false;
      if ((one_line & 0x80000001)==0x80000001) {
        continue;
      }
      if ((one_line >> 25) & 0x1) {
        moduleEoe = true;
        subtrigger++;
      }
      if ((one_line >> 26) & 0x1) {
        moduleError = true;
      }

      if (!moduleEoe && !moduleError) {
        int chip = (one_line >> 21) & 0xf;

        int index = result.Find(chip);
        if (index == -1) {
          std::cerr << "Error in APIX-MC Converter: Bad index: " << index << std::endl;
        } else {
          //std::cout << "DATA" << std::endl;
          int ypos = (one_line >>13) & 0xff; //row
          int xpos = (one_line >> 8) & 0x1f; //column
          int tot = (one_line) & 0xff;
          if (subtrigger > 15) {
            std::cerr << "Error in APIX-MC Converter: subtrigger > 15, skipping frame" << std::endl;
            continue;
          }
          result.planes.at(index).PushPixel(xpos, ypos, tot, subtrigger);
        }
      }
    }

  }

} //namespace eudaq
