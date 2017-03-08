#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"

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

	// TODO -------------->>>>
	// Investigate this method instead of memcpy stuf
	//const std::vector <unsigned char> & data=dynamic_cast<const std::vector<unsigned char> &> (ev_raw.GetBlock(i));
	// <<<<<<- ----------

	std::vector<unsigned short> data;
	data.resize(bl.size() / sizeof(short));
	std::memcpy(&data[0], &bl[0], bl.size());

	  if (bl.size()<8){
	    EUDAQ_ERROR("This data must be corrupt.  Block size=" + to_string(bl.size()));
	  }

	int nROCs = data[0];
	int nPixels = data[1];
	int trigID = data[2];
	int nHits = data[3];

	std::cout<<" Size of data = "<<data.size()<<"  trig ID:"<<trigID
		 <<"  nHits = "<<nHits<<" (If zero: it's non-ZS data!) \n"
		 <<" nROCs = "<<nROCs <<"  nPixels = "<<nPixels<<std::endl;
	//for (size_t d=0; d<data.size();d++){
	//if (d<10)
	//  std::cout<<d<<" data value: "<<data[d]<<std::endl;
	//}

	// Create a StandardPlane representing one sensor plane
	StandardPlane plane(2*pl+1, EVENT_TYPE, sensortype);

	if (nHits == 0){
	  std::cout<<"This is non-ZS data plane: "<<pl<<std::endl;
	  // Let's make sure that the size of data is right:
	  if (bl.size() != nROCs*nPixels*2 + 8)
	    EUDAQ_ERROR("This is no good. The numbers for non-ZS data don't match!  block size=" + to_string(bl.size()));
	 
	  // Set the number of pixels and frames in this plane
	  plane.SetSizeRaw(nROCs, nPixels, 3);

	  //std::cout<<"Plane type = "<<plane.Type()<<std::endl;
	  //std::cout<<"Plane sensor = "<<plane.Sensor()<<std::endl;


	  int ind=0;
	  for (unsigned roc = 0; roc < nROCs; ++roc) {
	    for (unsigned px = 0; px < nPixels; ++px) {
	      unsigned charge = data[4+roc+px*nROCs];
	      //for (unsigned fr=0; fr < 3; fr++)
	      plane.SetPixel(ind, roc, px, 0.25*charge, false, 0);
	      plane.SetPixel(ind, roc, px,      charge, false, 1);
	      plane.SetPixel(ind, roc, px, 0.10*charge, false, 2);

	      //if (abs(roc-2)+abs(px-42)<5)
	      //std::cout<<ind<<" roc="<<roc<<" px="<<px<<" charge: "<<charge<<std::endl;

	      ind++;
	    }
	  }

	}

	else {
	  std::cout<<"This is Zero Suppressed data plane: "<<pl
		   <<"  nHits = "<<nHits<<" (If zero: it's non-ZS data!) \n"<<std::endl;
	  // Need special plane constructor etc

	  if (data.size() != 3*nHits+4)
	    EUDAQ_ERROR("This is no good. The numbers for ZS data don't match! Data size=" + to_string(bl.size()));
	  
	  // Set ZS size of the plane
	  plane.SetSizeZS(nROCs, nPixels, nHits, 3);

	  //for (size_t n = 0; n < data.size(); n++)
	  //std::cout<<" data at "<<n<<"  is: "<<data[n]<<std::endl;

	  int ind = 0;
	  for (size_t hit = 4; hit < data.size()-4; hit+=3) {

	    unsigned short roc = data[hit];
	    unsigned short px  = data[hit+1];
	    unsigned short charge = data[hit+2];

	    //std::cout<<" roc="<<roc<<" px="<<px<<" charge: "<<charge<<std::endl;
	    
	    plane.SetPixel(ind, roc, px, 0.25*charge, false, 0);
	    plane.SetPixel(ind, roc, px,      charge, false, 1);
	    plane.SetPixel(ind, roc, px, 0.10*charge, false, 2);

	    // APZ: for some reason push methods give an error with more than one frame:
	    //plane.PushPixel(roc, px, 0.25*charge, false, 0);
	    //plane.PushPixel(roc, px,      charge, false, 1);
	    //plane.PushPixel(roc, px, 0.10*charge, false, 2);

	    //if (abs(roc-2)+abs(px-42)<5)
	    //std::cout<<" roc="<<roc<<" px="<<px<<" charge: "<<charge<<std::endl;

	    if (roc==0 && px==0){
	      EUDAQ_THROW("Zero-zero pixel. This should not hapen. Charge = "+ to_string(charge));
	      std::cout<<"\n \t ** Zero Pixel problem:"<<std::endl;
	      std::cout<<hit<<" roc="<<roc<<" px="<<px<<" charge: "<<charge<<std::endl;
	    }

	    ind++;
	  }
	}


	// Set the trigger ID
	plane.SetTLUEvent(GetTriggerID(ev));
	// Add the plane to the StandardEvent

	sev.AddPlane(plane);


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
