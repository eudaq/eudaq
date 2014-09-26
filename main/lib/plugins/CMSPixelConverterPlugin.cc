#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"

#include "eudaq/CMSPixelDecoder.h"
#include "dictionaries.h"

#include "iostream"
#include "bitset"

using namespace pxar;

namespace eudaq {

  static const char* EVENT_TYPE = "CMSPixel";

  // Events are not converted so far
  // This class is only here to prevent runtime errors
  class CMSPixelConverterPlugin : public DataConverterPlugin {
    public:
      // virtual unsigned GetTriggerID(const Event & ev) const{}; 

      virtual void Initialize(const Event & bore, const Configuration & cnf)
      {
        std::cout<<"CMSPixelConverterPlugin::Initialize" << std::endl;
        
        DeviceDictionary* devDict;
        std::string roctype = bore.GetTag("ROCTYPE", "");
        m_roctype = devDict->getInstance()->getDevCode(roctype);
        if (m_roctype == 0x0)
          EUDAQ_ERROR("Roctype" + to_string((int) m_roctype) + " not propagated correctly to CMSPixelConverterPlugin");
       
      }

      virtual bool GetStandardSubEvent(StandardEvent & out, const Event & in) const 
      {
        std::cout << "Call CMSPixelConverterPlugin::GetStandardSubEvent" << std::endl;

        // transform data of form char* to vector<int16_t>
        EUDAQ_DEBUG("m_roctype " + eudaq::to_string((int) m_roctype));
        if(m_roctype != 0x0){
          //CMSPixel::Log::ReportingLevel() = CMSPixel::Log::FromString("DEBUG3");
          CMSPixel::CMSPixelEventDecoderDigital evtDecoder(1, FLAG_12BITS_PER_WORD, m_roctype);

          StandardPlane plane(7, EVENT_TYPE);
          // Set the number of pixels
          int cols = 52, rows = 80;
          plane.SetSizeRaw(cols, rows);


          std::vector<uint16_t> rawData = TransformRawData(in);
          std::vector<CMSPixel::pixel> * evt = new std::vector<CMSPixel::pixel>;
          int status = evtDecoder.get_event(rawData, evt);
          EUDAQ_DEBUG("Decoding status: " + status);
          int i = 0;     
          for(std::vector<CMSPixel::pixel>::iterator it = evt->begin(); it != evt->end(); ++it){
            int roc = it->roc;
            int col = it->col;
            int row = it->row;
            int raw = it->raw;
            plane.PushPixel(col, row, raw);
            std::cout <<"Adding pixel to plane..."  << "roc " << roc << " col " << col << " row " << row 
              << " val " << raw << "==" << plane.GetPixel(i-1) << std::endl;
          }
	  plane.SetTLUEvent(GetTriggerID(in));
          out.AddPlane(plane);
          std::cout << "plane added" <<std::endl;
          delete evt;
          return true;
        }
        else{
          EUDAQ_ERROR("Invalid ROC type\n");
          return false;
        }
      }

    private:
      CMSPixelConverterPlugin() : DataConverterPlugin(EVENT_TYPE),
      m_roctype(0)
    {
      std::cout<<"CMSPixelConverterPlugin Event_Type: "<<EVENT_TYPE << std::endl;

    }
      static std::vector<uint16_t> TransformRawData(const RawDataEvent & in)
      {
        // transform data of form char* to vector<int16_t>
        std::vector<uint16_t> rawData;

        if(in.NumBlocks() != 1){
          EUDAQ_WARN("event " + to_string(in.GetEventNumber())
              + ": invalid number of data blocks ");        
          return rawData;
        }

        const std::vector<uint8_t> & block = in.GetBlock(0);

        int size = block.size();
        if(size < 2){
          EUDAQ_WARN("event " + to_string(in.GetEventNumber())
                       + ": data block is too small");
          return rawData;
        }
        int i = 0;
        while(i < size-1){
          uint16_t temp = ((uint16_t)block.data()[i+1] << 8) | block.data()[i];
          std::cout << (std::bitset<16>) temp << "==" << (std::bitset<8>) block.data()[i+1] << (std::bitset<8>) block.data()[i] << std::endl;
          rawData.push_back(temp);
          i+=2;
        }
        std::cout << "rawData roc header " << std::hex << rawData[0] <<  std::dec << std::endl;
       return rawData;
      }
      static std::vector<uint16_t> TransformRawData(const Event & in)
      {
        return TransformRawData(dynamic_cast<const RawDataEvent &>(in));
      }

      static CMSPixelConverterPlugin m_instance;
      uint8_t m_roctype;
  };

  CMSPixelConverterPlugin CMSPixelConverterPlugin::m_instance;

}
