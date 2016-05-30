#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"


namespace eudaq {

  static const char* EVENT_TYPE = "CCPX";
  
  // Declare a new class that inherits from DataConverterPlugin
  class CLICpixConverterPlugin : public DataConverterPlugin {
    
  public:
    
    
    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
   virtual unsigned GetTriggerID(const Event & ev) const {
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) 
      {
         if ( rev->NumBlocks() > 0 && rev->GetBlock(0).size() >= 4 ) 
         {
            std::vector<unsigned char> data8 = rev->GetBlock( 0 ); // block 0 is trigger data 
            unsigned int * data32 = (unsigned int *) &data8[0];
            return data32[0];
         }
      }
      return (unsigned)-1;
    }

    
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const 
    {
      std::string sensortype = "CLICpix";
            
      // Unpack data
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );
      std::cout << "[Number of blocks] " << rev->NumBlocks() << std::endl;

      std::vector<unsigned char> data = rev->GetBlock( 1 ); // block 1 is pixel data
      std::cout << "Vector has size : " << data.size() << std::endl;

      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      
      const unsigned int PIX_DATA_SIZE = 4;
      unsigned int hits= data.size()  / PIX_DATA_SIZE;
      
      // Set the number of pixels
      int width = 64, height = 64;
      plane.SetSizeZS( width, height, hits );
      
      std::vector<unsigned char> ZSDataX;
      std::vector<unsigned char> ZSDataY;
      std::vector<unsigned char> ZSDataTOT;
      std::vector<unsigned char> ZSDataTOA;
      
      unsigned char aWord = 0;
      
      for( unsigned int i = 0; i < data.size() ; i+=PIX_DATA_SIZE ) 
      {
          ZSDataX   .push_back( data[i+0] );
          ZSDataY   .push_back( data[i+1] );
          ZSDataTOT .push_back( data[i+2] );
          ZSDataTOA .push_back( data[i+3] );
      }

      // Set the trigger ID
      unsigned int tid=GetTriggerID(ev);
      plane.SetTLUEvent( tid );
      std::cout << "Trigger ID:"<<tid<<std::endl;
      
      for( size_t i = 0 ; i < ZSDataX.size(); ++i ) 
      {
         plane.SetPixel( i, ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
         std::cout << "[DATA] "  << " " << (int)ZSDataX[i] << " " << (int)ZSDataY[i] << " " << (int)ZSDataTOT[i] << " " << (int)ZSDataTOA[i] << std::endl;
         std::cout << "!CID "<<tid<<" "<< (int)ZSDataX[i] << " " << (int)ZSDataY[i] <<std::endl;

      }

      // Add the plane to the StandardEvent
      sev.AddPlane( plane );
      
      // Indicate that data was successfully converted
      return true;
    }
    
  private:
    
    CLICpixConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE)
    {}
    
    static CLICpixConverterPlugin m_instance;
  }; // class CLICpixConverterPlugin
  
  // Instantiate the converter plugin instance
  CLICpixConverterPlugin CLICpixConverterPlugin::m_instance;
  
} // namespace eudaq



