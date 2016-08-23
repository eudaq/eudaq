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
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
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
   
    void setIntDataInt(std::vector<int> &vec){
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

      unsigned int nblock =5; // the first 5 blocks contain information
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
      if(rawev->NumBlocks() < 3) return true;
           
      unsigned int nblock = 0;
	
  
      if(rawev->NumBlocks() > 2) {

       	// first two blocks should be string, 3rd is time
       	const RawDataEvent::data_t & bl0 = rawev->GetBlock(nblock++);
       	string colName((char *)&bl0.front(), bl0.size());

       	const RawDataEvent::data_t & bl1 = rawev->GetBlock(nblock++);
       	string dataDesc((char *)&bl1.front(), bl1.size());

       	// EUDAQ TIMESTAMP, saved in ScReader.cc
       	const RawDataEvent::data_t & bl2 = rawev->GetBlock(nblock++);
       	time_t timestamp = *(unsigned int *)(&bl2[0]);

	IMPL::LCEventImpl  & lcevent = dynamic_cast<IMPL::LCEventImpl&>(result);
	lcevent.setTimeStamp((long int)&bl2[0]);

	if ( colName ==  "EUDAQDataBIF") {
	  //-------------------
	  const RawDataEvent::data_t & bl3 = rawev->GetBlock(nblock++);
	  if(bl3.size() > 0)     cout << "Error, block 3 is filled in the BIF raw data" << endl;
  	  const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);
	  if(bl4.size() > 0)     cout << "Error, block 4 is filled in the BIF raw data" << endl;
  	  const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
	  if(bl5.size() > 0)     cout << "Error, block 5 is filled in the BIF raw data" << endl;
  
	  // READ BLOCKS WITH DATA
  	  LCCollectionVec *col = 0;
	  col=createCollectionVec(result,colName,dataDesc, timestamp);
	  getDataLCIOGenericObject(rawev,col,nblock);
	}

	if ( colName ==  "EUDAQDataScCAL") {

	  //-------------------
	  // READ/WRITE SlowControl info
	  //the  block=3, if non empty, contaions SlowControl info
	  const RawDataEvent::data_t & bl3 = rawev->GetBlock(nblock++);
	  if(bl3.size() > 0)   cout << "ERROR: Looking for SlowControl collection..." << endl;

	  // //-------------------
	  // // READ/WRITE LED info
	  // //the  block=4, if non empty, contaions LED info
	  const RawDataEvent::data_t & bl4 = rawev->GetBlock(nblock++);
	  if(bl4.size() > 0)   cout << "ERROR: Looking for LED voltages collection..." << endl;
  
	  // //-------------------
	  // // READ/WRITE Temperature info
	  // //the  block=5, if non empty, contaions Temperature info
	  const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
	  if(bl5.size()>0) {
	    LCCollectionVec *col5 = 0;
	    col5=createCollectionVec(result,"TempSensor", "i:LDA;i:port;i:T1;i:T2;i:T3;i:T4;i:T5;i:T6;i:TDIF;i:TPWR", timestamp);
	    getScCALTemperatureSubEvent(bl5, col5);  
	  }

	  // READ BLOCKS WITH DATA
  	  LCCollectionVec *col = 0;
	  col=createCollectionVec(result,colName,dataDesc, timestamp);
	  getDataLCIOGenericObject(rawev,col,nblock);
	  //use TrackerRawData instead of LCGenericObjects
	  //col=createRawCollectionVec(result,colName,dataDesc, timestamp);
	  //getDataTrackerRawData(rawev,col,nblock);

	}
      }

      return true;
    }

    virtual LCCollectionVec* createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp ) const {
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
      //add timestamp (set by the Producer, is EUDAQ, not real timestamp!!)
      struct tm *tms = localtime(&timestamp);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);
      long int t = (long int)timestamp;
      // result.setTimeStamp(t);
      // IMPL::LCEventImpl *lcevent = dynamic_cast<lcio::LCEvent>(result);
      //lcevent->setTimeStamp(t);


      return col;
    }

    virtual void getScCALTemperatureSubEvent(const RawDataEvent::data_t & bl, LCCollectionVec *col) const{
      
      // sensor specific data
      cout << "Looking for Temperature Collection... " << endl;
      vector<int> vec;
      vec.resize(bl.size() / sizeof(int));
      memcpy(&vec[0], &bl[0],bl.size());
      
      vector<int> output;
      int lda = -1;
      int port = 0;
      for(unsigned int i=0;i<vec.size()-2;i+=3){
	if((i/3)%2 ==0)continue; // just ignore the first data;
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
	  CaliceLCGenericObject *obj = new CaliceLCGenericObject;
	  obj->setIntDataInt(output);
	  try{
	    col->addElement(obj);
	  }catch(ReadOnlyException &e){
	    cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
	    delete obj;
	  }
	  output.clear();
	}
      }
    }

   virtual void getDataLCIOGenericObject(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock) const{
  
      while(nblock < rawev->NumBlocks() ){
	// further blocks should be data (currently limited to integer)

	vector<int> v;
	const RawDataEvent::data_t & bl = rawev->GetBlock(nblock++);
	v.resize(bl.size() / sizeof(int));
	memcpy(&v[0], &bl[0],bl.size());

	CaliceLCGenericObject *obj = new CaliceLCGenericObject;
	obj->setIntDataInt(v);
	try{
	  col->addElement(obj);
	}catch(ReadOnlyException &e){
	  cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
	  delete obj;
	}
      }
    }

    virtual LCCollectionVec* createRawCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp ) const {
      LCCollectionVec *col = 0;
      try{
        // looking for existing collection
        col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
        //cout << "collection found." << endl;
      }catch(DataNotAvailableException &e){
        // create new collection
        //cout << "Creating TempSensor collection..." << endl;
        col = new IMPL::LCCollectionVec(LCIO::TRACKERRAWDATA);
        result.addCollection(col,colName);
        //  cout << "collection added." << endl;
      }
      return col;
    }

    virtual void getDataTrackerRawData(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock) const{
   
      while(nblock < rawev->NumBlocks()){
        // further blocks should be data (currently limited to integer)
        vector<short> v;
        const RawDataEvent::data_t & bl5 = rawev->GetBlock(nblock++);
        v.resize(bl5.size());
        memcpy(&v[0], &bl5[0],bl5.size());

        TrackerRawDataImpl *obj = new TrackerRawDataImpl;
        obj->setCellID0(int(v[0]));
        obj->setCellID1(int(v[1]));
        obj->setTime(int(v[2]));
        obj->setADCValues ( v );

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




  
