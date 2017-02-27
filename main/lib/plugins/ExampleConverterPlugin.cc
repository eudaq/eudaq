#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

namespace eudaq {

  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char *EVENT_TYPE = "Hexagon";

  // Declare a new class that inherits from DataConverterPlugin
  class ExampleConverterPlugin : public DataConverterPlugin {

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event &bore, const Configuration &cnf) {
      m_exampleparam = bore.GetTag("EXAMPLE", 0);
#ifndef WIN32 // some linux Stuff //$$change
      (void)cnf; // just to suppress a warning about unused parameter cnf
#endif
    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event &ev) const {
      static const unsigned TRIGGER_OFFSET = 8;
      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
        // This is just an example, modified it to suit your raw data format
        // Make sure we have at least one block of data, and it is large enough
        if (rev->NumBlocks() > 0 &&
            rev->GetBlock(0).size() >= (TRIGGER_OFFSET + sizeof(short))) {
          // Read a little-endian unsigned short from offset TRIGGER_OFFSET
          return getlittleendian<unsigned short>(
              &rev->GetBlock(0)[TRIGGER_OFFSET]);
        }
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "Hexa";


      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );

      //rev->Print(std::cout);

      unsigned nPlanes = rev->NumBlocks();
      std::cout<<"Number of Raw Data Blocks (=Planes): "<<nPlanes<<std::endl;
      
      for (unsigned pl=0; pl<nPlanes; pl++){
	
	std::cout<<"Plane = "<<pl<<"  Raw GetID = "<<rev->GetID(pl)<<std::endl;

	const RawDataEvent::data_t & bl = rev->GetBlock(pl);
	
	std::cout<<"size of block: "<<bl.size()<<std::endl;

	if (bl.size()==520){
	  std::cout<<"This is non-ZS data plane: "<<pl<<std::endl;
	  // Create a StandardPlane representing one sensor plane
	  StandardPlane plane(2*pl+1, EVENT_TYPE, sensortype);
	  // Set the number of pixels
	  int nROCs = 4, nPixels = 64;
	  plane.SetSizeRaw(nROCs, nPixels, 3);
	  
	  //std::cout<<"Plane type = "<<plane.Type()<<std::endl;
	  //std::cout<<"Plane sensor = "<<plane.Sensor()<<std::endl;
	  


	  // TODO -------------->>>>
	  // Investigate this method instead of memcpy stuf
	  //const std::vector <unsigned char> & data=dynamic_cast<const std::vector<unsigned char> &> (ev_raw.GetBlock(i));
	  // <<<<<<- ----------

	  std::vector<unsigned short> data;
	  data.resize(bl.size() / sizeof(short));
	  std::memcpy(&data[0], &bl[0], bl.size());
	  
	  std::cout<<" size of data = "<<data.size()<<std::endl;
	  
	  //for (size_t d=0; d<data.size();d++){
	  //if (d<10)
	  //  std::cout<<d<<" data value: "<<data[d]<<std::endl;
	  //}
	  
	  int ind=0;
	  for (unsigned px = 0; px < nPixels; ++px) {
	    for (unsigned roc = 0; roc < nROCs; ++roc) {
	      unsigned short charge = data[4+roc+px*nROCs];
	      //for (unsigned fr=0; fr < 3; fr++)
	      plane.SetPixel(ind, roc, px, 0.25*charge, false, 0);	  
	      plane.SetPixel(ind, roc, px, charge, false, 1);	  
	      plane.SetPixel(ind, roc, px, 0.10*charge, false, 2);	  

	      //if (abs(roc-2)+abs(px-42)<7)
	      //std::cout<<ind<<" roc="<<roc<<" px="<<px<<" charge: "<<charge<<std::endl;

	      ind++;
	    }
	  }
	  
	  // Set the trigger ID
	  plane.SetTLUEvent(GetTriggerID(ev));
	  // Add the plane to the StandardEvent
	  
	  sev.AddPlane(plane);
	}
	
	else {
	  std::cout<<"This is ZS data plane "<<pl<<std::endl;
	  // Need special plane constructor etc
	}

      }
      //std::cout<<"St Ev NumPlanes: "<<sev.NumPlanes()<<std::endl;
      
      // Indicate that data was successfully converted
      return true;
      
    }
    
#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent *GetLCIOEvent(const Event * /*ev*/) const {
      return 0;
    }
#endif

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    ExampleConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0) {}

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static ExampleConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  ExampleConverterPlugin ExampleConverterPlugin::m_instance;

} // namespace eudaq
