#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"


#ifdef WIN32
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#  include <stdint.h>
#endif

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>

using namespace std;

#if USE_EUTELESCOPE
#  include <EUTELESCOPE.h>
#endif

// #define MYDEBUG  // dumps decoding information
// #define DEBUGRAWDUMP // dumps all raw events
#define CHECK_TIMESTAMPS // if timestamps are not consistent marks event as broken

namespace eudaq {
  /////////////////////////////////////////////////////////////////////////////////////////
  //Converter
  ////////////////////////////////////////////////////////////////////////////////////////

  //Detector/Eventtype ID
  static const char* EVENT_TYPE = "pALPIDEfsRAW";

  //Plugin inheritance
  class PALPIDEFSConverterPlugin : public DataConverterPlugin {

  public:
    ////////////////////////////////////////
    //INITIALIZE
    ////////////////////////////////////////
    //Take specific run data or configuration data from BORE
    virtual void Initialize(const Event & bore,
                            const Configuration & /*cnf*/) {    //GetConfig

      m_nLayers = bore.GetTag<int>("Devices", -1);
      cout << "BORE: m_nLayers = " << m_nLayers << endl;
      
      m_DataVersion = bore.GetTag<int>("DataVersion", 1);
      
      for (int i=0; i<m_nLayers; i++) {
	char tmp[100];
	sprintf(tmp, "Config_%d", i);
	std::string config = bore.GetTag<std::string>(tmp, "");
	// cout << "Config of layer " << i << " is: " << config.c_str() << endl;
	
	// get masked pixels
	sprintf(tmp, "MaskedPixels_%d", i);
	std::string pixels = bore.GetTag<std::string>(tmp, "");
	//cout << "Masked pixels of layer " << i << " is: " << pixels.c_str() << endl;
      }
    }
    //##############################################################################
    ///////////////////////////////////////
    //GetTRIGGER ID
    ///////////////////////////////////////

    //Get TLU trigger ID from your RawData or better from a different block
    //example: has to be fittet to our needs depending on block managing etc.
    virtual unsigned GetTriggerID(const Event & /*ev*/) const {
      // Make sure the event is of class RawDataEvent
      //if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
	// TODO get trigger id. probably common code with producer. try to factor out somewhere.
      //}
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    ///////////////////////////////////////
    //STANDARD SUBEVENT
    ///////////////////////////////////////

    //conversion from Raw to StandardPlane format
    virtual bool GetStandardSubEvent(StandardEvent & sev,const Event & ev) const {

#ifdef MYDEBUG
      cout << "GetStandardSubEvent " << ev.GetEventNumber() << " " << sev.GetEventNumber() << endl;
#endif
      
      if(ev.IsEORE()) {
	// TODO EORE
	return false;
      } 
      
      if (m_nLayers < 0) {
	cout << "ERROR: Number of layers < 0 --> " << m_nLayers << ". Check BORE!" << endl;
	return false;
      }
      
      //Reading of the RawData
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
#ifdef MYDEBUG
      cout << "[Number of blocks] " << rev->NumBlocks() << endl;
#endif

      //initialize everything
      std::string sensortype = "pALPIDEfs";
      
      // Create a StandardPlane representing one sensor plane

      // Set the number of pixels
      unsigned int width = 1024, height = 512;
	
      //Create plane for all matrixes? (YES)
      StandardPlane** planes = new StandardPlane*[m_nLayers];
      for(int id=0 ; id<m_nLayers ; id++ ){
        //Set planes for different types
        planes[id] = new StandardPlane(id, EVENT_TYPE, sensortype);
        planes[id]->SetSizeZS(width, height, 0, 1, StandardPlane::FLAG_ZS);
      }
      
      if (ev.GetTag<int>("pALPIDEfs_Type", -1) == 1) {
#ifdef MYDEBUG
	cout << "Skipping status event" << endl;
	for (int id=0 ; id<m_nLayers ; id++) {
	  vector<unsigned char> data = rev->GetBlock(id);
	  if (data.size() == 4) {
	    float temp = 0;
	    for (int i=0; i<4; i++)
	      ((unsigned char*) (&temp))[i] = data[i];
	    cout << "T (layer " << id << ") is: " << temp << endl;
	  }
	}
#endif
	sev.SetFlags(Event::FLAG_STATUS);
      } else {
	//Conversion
	if (rev->NumBlocks() == 1) { 
	  vector<unsigned char> data = rev->GetBlock(0);
#ifdef MYDEBUG
	  cout << "vector has size : " << data.size() << endl;
#endif

	  //###############################################
	  //DATA FORMAT
	  //m_nLayers times
	  //  Header        1st Byte 0xff ; 2nd Byte: Layer number (layers might be missing)
	  //  Length (uint16_t) length of data block for this layer [only for DataVersion >= 2]
	  //  Trigger id (uint64_t)
	  //  Timestamp (uint64_t)
	  //  payload from chip
	  //##############################################
	  
	  unsigned int pos = 0;
	  int current_layer = -1;
	  int current_rgn = -1;

	  const int maxLayers = 100;
	  bool layers_found[maxLayers];
	  uint64_t trigger_ids[maxLayers];
	  uint64_t timestamps[maxLayers];
	  for (int i=0; i<maxLayers; i++) { 
	    layers_found[i] = false;
	    trigger_ids[i] = (uint64_t) ULONG_MAX;
	    timestamps[i] = (uint64_t) ULONG_MAX;
	  }
	  
	  // RAW dump
#ifdef DEBUGRAWDUMP
	  printf("Event %d: ", ev.GetEventNumber());
	  for (unsigned int i=0; i<data.size(); i++)
	    printf("%x ", data[i]);
	  printf("\n");
#endif
	  
	  int last_rgn = -1;
	  int last_pixeladdr = -1;
	  int last_doublecolumnaddr = -1;

	  while (pos+1 < data.size()) { // always need 2 bytes left
    #ifdef MYDEBUG
	    printf("%u %u %x %x\n", pos, (unsigned int) data.size(), data[pos], data[pos+1]);
    #endif
    // 	printf("%x %x ", data[pos], data[pos+1]);
	    if (current_layer == -1) {
	      if (data[pos++] != 0xff) {
		cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Next byte not 0xff but " << (unsigned int) data[pos-1] << endl;
		break;
	      }
	      current_layer = data[pos++];
	      if (current_layer == 0xff) {
		// 0xff 0xff is used as fill bytes to fill up to a 4 byte wide data stream
		current_layer = -1;
		continue;
	      }
	      if (current_layer >= m_nLayers) {
		cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Not defined layer in data " << current_layer << endl;
		break;
	      }
	      layers_found[current_layer] = true;
    #ifdef MYDEBUG
	      cout << "Now in layer " << current_layer << endl;
    #endif
	      current_rgn = -1;
	      last_rgn = -1;
	      last_pixeladdr = -1;
	      last_doublecolumnaddr = -1;
	      
	      // length
	      if (m_DataVersion >= 2) {
		if (pos+sizeof(uint16_t) <= data.size()) {
		  uint16_t length = 0;
		  for (int i=0; i<2; i++)
		    ((unsigned char*) &length)[i] = data[pos++];
#ifdef MYDEBUG
		  cout << "Layer " << current_layer << " has data of length " << length << endl;
#endif
		  if (length == 0) {
		    // no data for this layer
		    current_layer = -1;
		    continue;
		  }
		}
	      }
	      
	      // extract trigger id and timestamp
	      if (pos+2*sizeof(uint64_t) <= data.size()) {
		uint64_t trigger_id = 0;
		for (int i=0; i<8; i++)
		  ((unsigned char*) &trigger_id)[i] = data[pos++];
		uint64_t timestamp = 0;
		for (int i=0; i<8; i++)
		  ((unsigned char*) &timestamp)[i] = data[pos++];
		trigger_ids[current_layer] = trigger_id;
		timestamps[current_layer] = timestamp;
    #ifdef MYDEBUG
	      cout << "Layer " << current_layer << " Trigger ID: " << trigger_id << " Timestamp: " << timestamp << endl;
    #endif
	      }
	    } else {
	      int length = data[pos++];
	      int rgn = data[pos++] >> 3;
	      if (rgn != current_rgn+1) {
		cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Wrong region order. Previous " << current_rgn << " Next " << rgn << endl;
		break;
	      }
	      if (pos+length*2 > data.size()) {
		cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Not enough bytes left. Expecting " << length*2 << " but pos = " << pos << " and size = " << data.size() << endl;
		break;
	      }
    #ifdef MYDEBUG
	      cout << "Now in region " << rgn << ". Going to read " << length << " pixels. " << endl;
    #endif	      
	      for (int i=0; i<length; i++) {
		unsigned short dataword = data[pos++];
		dataword |= (data[pos++] << 8);
		unsigned short pixeladdr = dataword & 0x3ff;
		unsigned short doublecolumnaddr = (dataword >> 10) & 0xf;
		unsigned short clustersize = (dataword >> 14) + 1;
		
		// consistency check
		if (rgn <= last_rgn && doublecolumnaddr <= last_doublecolumnaddr && pixeladdr <= last_pixeladdr) {
		  cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << current_layer << ". ";
		  if (rgn == last_rgn && doublecolumnaddr == last_doublecolumnaddr && pixeladdr == last_pixeladdr)
		    cout << "Pixel duplicated. ";
		  else
                  {
		    cout << "Strict ordering violated. ";
                    sev.SetFlags(Event::FLAG_BROKEN);
                  }
		  cout << "Last pixel was: " << last_rgn << "/" << last_doublecolumnaddr << "/" << last_pixeladdr << " current: " << rgn << "/" << doublecolumnaddr << "/" << pixeladdr << endl;
		}
		last_rgn = rgn;
		last_pixeladdr = pixeladdr;
		last_doublecolumnaddr = doublecolumnaddr;
		
		for (int j=0; j<clustersize; j++) {
		  unsigned short current_pixel = pixeladdr + j;
		  unsigned int x = rgn * 32 + doublecolumnaddr * 2 + 1;
		  if (current_pixel & 0x2)
		    x--;
		  unsigned int y = (current_pixel >> 2) * 2 + 1;
		  if (current_pixel & 0x1)
		    y--;
		  
		  planes[current_layer]->PushPixel(x, y, 1, (unsigned int) 0);
    #ifdef MYDEBUG
		  cout << "Added pixel to layer " << current_layer << " with x = " << x << " and y = " << y << endl;
    #endif
		}
	      }
	      current_rgn = rgn;
	      if (rgn == 31)
		current_layer = -1;
	    }
	  }
	  if (current_layer != -1) {
	    cout << "ERROR: Event " << ev.GetEventNumber() << " data stream too short, stopped in region " << current_rgn << endl;
            sev.SetFlags(Event::FLAG_BROKEN);
	  }
	  for (int i=0;i<m_nLayers;i++) {
	    if (!layers_found[i]) {
	      cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << i << " was missing in the data stream." << endl;
              sev.SetFlags(Event::FLAG_BROKEN);
	    }
	  }
	    
    #ifdef MYDEBUG
	  cout << "EOD" << endl;
    #endif
	
	  // check timestamps
	  bool ok = true;
	  for (int i=0;i<m_nLayers-1;i++) {
	    if (timestamps[i+1] == 0 || (fabs(1.0 - (double) timestamps[i] / timestamps[i+1]) > 0.0001 && fabs((double)timestamps[i] - (double)timestamps[i+1]) > 4))
	      ok = false;
	  }
	  if (!ok) {
	    cout << "ERROR: Event " << ev.GetEventNumber() << " Timestamps not consistent." << endl;
#ifdef MYDEBUG  
	    for (int i=0;i<m_nLayers;i++)
	      printf("%d %lu %lu\n", i, trigger_ids[i], timestamps[i]);
#endif
#ifdef CHECK_TIMESTAMPS
	    sev.SetFlags(Event::FLAG_BROKEN);
#endif 
            sev.SetTimestamp(0); 
	  }
          else 
	    sev.SetTimestamp(timestamps[0]);
	}	
      }
      
      // Add the planes to the StandardEvent
      for(int i=0;i<m_nLayers;i++){
        sev.AddPlane(*planes[i]);
	delete planes[i];
      }
      delete[] planes;
      // Indicate that data was successfully converted
      return true;
    }

    ////////////////////////////////////////////////////////////
    //LCIO Converter
    ///////////////////////////////////////////////////////////
#if USE_LCIO
    virtual bool GetLCIOSubEvent(lcio::LCEvent & lev, eudaq::Event const & ev) const{
      
//       cout << "GetLCIOSubEvent..." << endl;

      StandardEvent sev;                //GetStandardEvent first then decode plains
      GetStandardSubEvent(sev,ev);

      unsigned int nplanes=sev.NumPlanes();             //deduce number of planes from StandardEvent
      
      lev.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,eutelescope::kDE);
      lev.parameters().setValue("TIMESTAMP",(int) sev.GetTimestamp());
      lev.parameters().setValue("FLAG", (int) sev.GetFlags());
      LCCollectionVec *zsDataCollection;
      try {
	zsDataCollection=static_cast<LCCollectionVec*>(lev.getCollection("zsdata_pALPIDEfs"));
      }
      catch (lcio::DataNotAvailableException) {
	zsDataCollection=new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      for(unsigned int n=0 ; n<nplanes ; n++){  //pull out the data and put it into lcio format
        StandardPlane& plane=sev.GetPlane(n);
	const std::vector<StandardPlane::pixel_t>& x_values = plane.XVector();
	const std::vector<StandardPlane::pixel_t>& y_values = plane.YVector();
	
	CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING,zsDataCollection);
	TrackerDataImpl *zsFrame=new TrackerDataImpl();
	zsDataEncoder["sensorID"       ]=n;
	zsDataEncoder["sparsePixelType"]=eutelescope::kEUTelSimpleSparsePixel;
	zsDataEncoder.setCellID(zsFrame);
	
	for (unsigned int i=0; i<x_values.size(); i++) {
	  zsFrame->chargeValues().push_back((int) x_values.at(i));
	  zsFrame->chargeValues().push_back((int) y_values.at(i));
	  zsFrame->chargeValues().push_back(1);
	  
// 	  std::cout << x_values.size() << " " << x_values.at(i) << " " << y_values.at(i) << std::endl;
	}
	
	zsDataCollection->push_back(zsFrame);
      }

      lev.addCollection(zsDataCollection,"zsdata_pALPIDEfs");

      return true;
    }
#endif

  protected:
    int m_nLayers;
    int m_DataVersion;

  private:

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    PALPIDEFSConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_nLayers(-1)
      {}

    // The single instance of this converter plugin
    static PALPIDEFSConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  PALPIDEFSConverterPlugin PALPIDEFSConverterPlugin::m_instance;

} // namespace eudaq
