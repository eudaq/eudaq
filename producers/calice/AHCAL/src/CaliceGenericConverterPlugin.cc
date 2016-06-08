// CaliceGenericConverterPlugin.cc
#include <string>
#include <iostream>
#include <typeinfo>  // for the std::bad_cast

#include "eudaq/DataConverterPlugin.hh"
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

#if USE_LCIO
  // LCIO class
  class CaliceLCGenericObject : public lcio::LCGenericObjectImpl {
  public:
    CaliceLCGenericObject() : lcio::LCGenericObjectImpl() {_typeName = EVENT_TYPE;}
    virtual ~CaliceLCGenericObject(){}
      
    void setTags(std::string &s){_dataDescription = s;}
    std::string getTags()const{return _dataDescription;}
     
    void setDataInt(std::vector<int> &vec){
      _intVec.resize(vec.size());
      std::copy(vec.begin(), vec.end(), _intVec.begin());
    }

    void setDataInt(std::vector<short> &vec){
      _intVec.resize(vec.size());
      std::copy(vec.begin(), vec.end(), _intVec.begin());
    }
        
    const std::vector<int> & getDataInt()const{return _intVec;}
  };

#endif


  class CaliceGenericConverterPlugin : public DataConverterPlugin {

  public:
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const
    {
      std::string sensortype = "Calice";


      // Unpack data
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> ( &ev );
      // std::cout << "[Number of blocks] " << rev->NumBlocks() <<    std::endl;

      unsigned int nblock =4; // the first 4 blocks contain information
      StandardPlane plane0(0, EVENT_TYPE, sensortype);
      plane0.SetSizeRaw( 36, 8);//36 channels, 4 chips

      while(nblock < rev->NumBlocks()){

	vector<short> data;
	const RawDataEvent::data_t & bl = rev->GetBlock(nblock++);
	data.resize(bl.size() / sizeof(short));
	memcpy(&data[0], &bl[0],bl.size());

	//data[i]=
	//i=0 --> cycleNr
	//i=1 --> bunch crossing id
	//i=2 --> memcell or EvtNr (same thing, different naming)
	//i=3 --> ChipId
	//i=4 --> Nchannels per chip (normally 36)
	//i=5 to NC+4 -->  14 bits that contains TDC and hit/gainbit
	//i=NC+5 to NC+NC+4  -->  14 bits that contains ADC and again a copy of the hit/gainbit

	for(int ichan=0; ichan<data[4]; ichan++) {
	  short adc= data[5+data[4]+ichan] % 4096; // extract adc
	  short gainbit= (data[5+data[4]+ichan] & 0x2000)/8192 ; //extract gainbit
	  short hitbit = (data[4+data[4]+ichan] & 0x1000)/4096;  //extract hitbit

	  if(data[3]<8)
	    plane0.PushPixel( ichan, data[3], adc*hitbit);
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

	const RawDataEvent::data_t & bl2 = rawev->GetBlock(nblock++);
	string dataDesc((char *)&bl2.front(), bl2.size());

	// EUDAQ TIMESTAMP, saved in ScReader.cc
	const RawDataEvent::data_t & bl3 = rawev->GetBlock(nblock++);
	if(bl3.size()<8){cout << "CaliceGenericConverter: time data too short!" << endl; return false;}
	time_t timestamp = *(unsigned int *)(&bl3[0]);
	unsigned int timestamp_usec = *(int *)(&bl3[4]);


	//-------------------
	// READ/WRITE Temperature
	//fourth block is temperature (if colName ==  "EUDAQDataScCAL")
	const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);

	if(bl4.size() > 0){
	  // sensor specific data
	  if(colName == "EUDAQDataScCAL"){
	    cout << "Looking for TempSensor collection..." << endl;
	           
	    vector<int> vec;
	    vec.resize(bl4.size() / sizeof(int));
	    memcpy(&vec[0], &bl4[0],bl4.size());

	    LCCollectionVec *col2 = 0;
	    col2=createCollectionVec(result,"TempSensor", "i:LDA;i:port;i:T1;i:T2;i:T3;i:T4;i:T5;i:T6;i:TDIF;i:TPWR", timestamp);
	    getTemperatureSubEvent(col2, vec);
	  }
	}
	
	//-------------------
	// READ/WRITE SPIROC DATA
	LCCollectionVec *col = 0;
	col=createCollectionVec(result,colName,dataDesc, timestamp);
	getSpirocSubEvent(rawev,col,nblock);
	//-------------------

      }
      return true;
    }


    virtual LCCollectionVec* createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp ) const {
      const char *colName2 = "TempSensor";
      LCCollectionVec *col = 0;
      try{
	// looking for existing collection
	col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
	//cout << "collection found." << endl;
      }catch(DataNotAvailableException &e){
	// create new collection
	//cout << "Creating TempSensor collection..." << endl;
	col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
	result.addCollection(col,colName);
	//  cout << "collection added." << endl;
      }
      col->parameters().setValue("DataDescription", dataDesc);
      //add timestamp (EUDAQ!!)
      struct tm *tms = localtime(&timestamp);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);
      col->parameters().setValue("Timestamp_i", (int)timestamp);

      return col;
    }

    virtual void getTemperatureSubEvent(LCCollectionVec *col2, vector<int> vec) const{


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

    virtual void getSpirocSubEvent(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock) const{
   
       
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


#endif
    
  private:
    CaliceGenericConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE)
    {}
    
    static CaliceGenericConverterPlugin m_instance;
    

  };

  // Instantiate the converter plugin instance
  CaliceGenericConverterPlugin CaliceGenericConverterPlugin::m_instance;
  
}




  
