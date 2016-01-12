// CaliceGenericConverterPlugin.cc
#include <string>
#include <map>

#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/CaliceGenericConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"


// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/LCGenericObjectImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif


using namespace eudaq;
using namespace std;
using namespace lcio;

namespace eudaq {

  static const char* EVENT_TYPE = "CaliceObject";

  class CaliceGenericConverterPlugin : public DataConverterPlugin {

  public:
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const
    {
      std::string sensortype = "Calice";


      // Unpack data
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );
      // std::cout << "[Number of blocks] " << rev->NumBlocks() <<    std::endl;

      int nblock =4;
      StandardPlane plane0(0, EVENT_TYPE, sensortype);
      plane0.SetSizeRaw( 36, 8);//36 channels, 4 chips

      while(nblock < rev->NumBlocks()){

	vector<short> data;
	const RawDataEvent::data_t & bl = rev->GetBlock(nblock++);
	data.resize(bl.size() / sizeof(short));
	memcpy(&data[0], &bl[0],bl.size());

	//data[i]=
	//i=0 --> bunch crossing id
	//i=1 --> memcell or EvtNr (same thing, different naming)
	//i=2 --> ChipId
	//i=3 --> Nchannels per chip (normally 36)
	//i=4 to NC+3 -->  12 bits that contains ADC and hit/gainbit
	//i=NC+4 to NC+NC+3  -->  12 bits that contains TDC and again  acopy of the hit/gainbit

	for(int ichan=0; ichan<data[3]; ichan++) {
	  short adc= data[4+ichan] % 4096; // extract adc
	  short gainbit= (data[4+ichan] & 0x2000)/8192 ; 
	  short hitbit = (data[4+ichan] & 0x1000)/4096;

	  if(data[2]<8)
	    plane0.PushPixel( ichan, data[2], adc*hitbit);
	}

      }

      sev.AddPlane( plane0 );

      return true;

    }



#if USE_LCIO
    virtual  bool GetLCIOSubEvent(lcio::LCEvent &result, eudaq::Event const &source)  const
    {
      //cout << "CaliceGenericConverterPlugin::GetLCIOSubEvent" << endl;
      // try to cast the Event
      eudaq::RawDataEvent const * rawev = 0;
      try{
      	eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(source);
      	rawev = &rawdataevent;
      }catch(std::bad_cast& e){
      	std::cout << e.what() << std::endl;
      	return false;
      }
      // should check the type
      if(rawev->GetSubType() != EVENT_TYPE){
      	cout << "CaliceGenericConverter: type failed!" << endl;
      	return false;
      }

      // no contents -ignore
      if(rawev->NumBlocks() == 3) return true;
    
      unsigned int nblock = 0;

      if(rawev->NumBlocks() > 3) {

      // first two blocks should be string, 3rd is time, time_usec
      const RawDataEvent::data_t & bl = rawev->GetBlock(nblock++);
      string colName((char *)&bl.front(), bl.size());
      // cout << "colName = " << colName << endl;
      const RawDataEvent::data_t & bl2 = rawev->GetBlock(nblock++);
      string dataDesc((char *)&bl2.front(), bl2.size());
      //cout << "dataDesc = " << dataDesc << endl;
      const RawDataEvent::data_t & bl3 = rawev->GetBlock(nblock++);
      if(bl3.size()<8){cout << "CaliceGenericConverter: time data too short!" << endl; return false;}
      time_t timestamp = *(unsigned int *)(&bl3[0]);
      unsigned int timestamp_usec = *(int *)(&bl3[4]);

      //fourth block is temperature (if colName ==  "EUDAQDataScCAL")
      const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);
      if(bl4.size() > 0){
      	// sensor specific data
      	if(colName == "EUDAQDataScCAL"){
      	  cout << "Looking for TempSensor collection..." << endl;
      	  // ScCAL: three data for one data, first should be ignored, and second should be the temperature/voltages
      	  // getting collection
      	  const char *colName2 = "TempSensor";
      	  LCCollectionVec *col2 = 0;
      	  try{
      	    // looking for existing collection
      	    col2 = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName2));
      	    //cout << "collection found." << endl;
      	  }catch(DataNotAvailableException &e){
      	    // create new collection
      	    //cout << "Creating TempSensor collection..." << endl;
      	    col2 = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
      	    result.addCollection(col2,colName2);
      	    //  cout << "collection added." << endl;
      	  }
        
      	  // store description header
      	  col2->parameters().setValue("DataDescription", "i:LDA;i:port;i:T1;i:T2;i:T3;i:T4;i:T5;i:T6;i:TDIF;i:TPWR");
      	  struct tm *tms = localtime(&timestamp);
      	  char tmc[256];
      	  strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      	  col2->parameters().setValue("Timestamp", tmc);
      	  col2->parameters().setValue("Timestamp_i", (int)timestamp);
      	  vector<int> vec;
      	  vec.resize(bl4.size() / sizeof(int));
      	  memcpy(&vec[0], &bl4[0],bl4.size());
        
      	  vector<int> output;
      	  int lda = -1;
      	  int port = 0;
      	  //int dummylda = 0;
      	  for(unsigned int i=0;i<vec.size()-2;i+=3){
      	    if((i/3)%2 ==0)continue; // just ignore the first data;
      	    //dummylda = vec[i]; // lda number: should be dummy
      	    if(output.size()!=0 && port != vec[i+1]){
      	      cout << "Different port number comes before getting 8 data!." << endl;
      	      break;
      	    }
      	    port = vec[i+1]; // port number
      	    int data = vec[i+2]; // data
      	    if(output.size() == 0){
      	      if(port == 0)lda ++;
      	      output.push_back(lda);
      	      output.push_back(port);
      	    }
      	    output.push_back(data);
          
      	    if(output.size() == 10){
      	      //            cout << "Storing TempSensor data..." << endl;
      	      CaliceLCGenericObject *obj = new CaliceLCGenericObject;
      	      obj->setDataInt(output);
      	      try{
      		col2->addElement(obj);
      	      }catch(ReadOnlyException &e){
      		cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
      		delete obj;
      	      }
      	      output.clear();
      	    }
      	  }
      	}
      }
    
      // getting collection
      LCCollectionVec *col = 0;
      try{
      	// looking for existing collection
      	col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));

      	//      cout << "collection found." << endl;
      }catch(DataNotAvailableException &e){
      	// create new collection
      	col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
      	result.addCollection(col,colName);
      	//      cout << "collection added." << endl;
      }
      col->parameters().setValue("DataDescription", dataDesc);
    
      // add timestamp
      struct tm *tms = localtime(&timestamp);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);
      col->parameters().setValue("Timestamp_i", (int)timestamp);
    
      while(nblock < rawev->NumBlocks()){
      	// further blocks should be data (currently limited to integer)
      	vector<short> v;
      	const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
      	v.resize(bl5.size() / sizeof(short));
      	memcpy(&v[0], &bl5[0],bl5.size());

      	CaliceLCGenericObject *obj = new CaliceLCGenericObject;
      	obj->setDataInt(v);
      	try{
      	  col->addElement(obj);
      	}catch(ReadOnlyException &e){
      	  cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
      	  delete obj;
      	}
      }

      }
       return true;
    }
#endif
    
  private:
    CaliceGenericConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE)
    {}
    
    static CaliceGenericConverterPlugin m_instance;
  };
  // Instantiate the converter plugin instance
  CaliceGenericConverterPlugin CaliceGenericConverterPlugin::m_instance;
  
  
  
} // namespace eudaq
