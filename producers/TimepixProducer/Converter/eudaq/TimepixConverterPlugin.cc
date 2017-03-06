#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Utils.hh"
#include <stdio.h>
#include <string.h>
#include <vector>
#include "TFile.h"
#include "TTree.h"
// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/TrackerDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "UTIL/CellIDEncoder.h"
#  include "lcio.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
#  include "EUTelTimepixDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelSimpleSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif
#define MATRIX_SIZE 65536

using namespace std;

template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
    unsigned char * src = static_cast < unsigned char* >(static_cast < void * >(&data));
    dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (vector <unsigned char >& src, int index, T& data) {
    copy (&src[index], &src[index + sizeof (T)], &data);
}

class mimtlu_exception : public std::runtime_error
{
public:
  mimtlu_exception(const std::string& message)
    : std::runtime_error(message)
  { };
};

struct mimtlu_event
{
  unsigned long int timestamp;
  unsigned char track;
  unsigned int tlu;
  unsigned char txt[17];
  mimtlu_event(unsigned long int _timestamp,unsigned char _track,  unsigned int _tlu)
  {
    timestamp=_timestamp;
    track=_track;
    tlu=_tlu;
    txt[16]=0;
  }
  mimtlu_event(const unsigned char *buf)
  {
    from_string(buf);
  }

  mimtlu_event(const std::vector<unsigned char> &buf)
  {
    from_vector(buf);
  }
  
  void from_string(const unsigned char* buf){
//    if (strlen((char*)buf)!=16)
//      throw mimtlu_exception("MIMTLU event parssing error");
    strncpy((char*)txt,&((char*)buf)[0],16);
    txt[16]=0;

    char tmp[16];
    //timestamp
    strncpy((char*)tmp,&((char*)buf)[0],10);
    tmp[10]=0;
    sscanf(tmp, "%lx", &timestamp);
    //track
    unsigned int tmpint;
    strncpy((char*)tmp,&((char*)buf)[10],2);
    tmp[2]=0;
    sscanf(tmp, "%x", &tmpint);
    track=tmpint&0xff;
    //tlu
    strncpy((char*)tmp,&((char*)buf)[12],4);
    tmp[4]=0;
    sscanf(tmp, "%x", &tlu);
    return;
  }

  const unsigned char * to_char(void)
  {
    return txt;
  }
  
  void from_vector(const std::vector<unsigned char> &buf)
  {
     from_string(&buf[0]);
  }
};


namespace eudaq {
  
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const char* EVENT_TYPE = "TimepixRaw";



  // Declare a new class that inherits from DataConverterPlugin
  class TimepixConverterPlugin : public DataConverterPlugin {
  
  private: 
	TFile *f ;
	TTree *t;
	double A[256][256];
	double B[256][256];   
	double C[256][256];
	double D[256][256];  

	double Aerr[256][256];
	double Berr[256][256];   
	double Cerr[256][256];
	double Derr[256][256];  
	double Chi2ndf[256][256];
	double meanA,meanB,meanC,meanD;

  public:
    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event & bore,
                            const Configuration & cnf) {
      m_exampleparam = bore.GetTag("Timepix", 0);
      //eudaq::PluginManager::Initialize(bore);
      (void)cnf; // just to suppress a warning about unused parameter cnf
      
        
//	char* path= getenv("EUDAQ");
//	TString str= TString::Format("%s/TimepixProducer/FitParameters_2stepFit_v7-I10_W015.root",path);
//	cout << "Reading calibration file at " << str << endl;
//	f = TFile::Open(str,"open");
//      	t =(TTree*)f->Get("fitPara");
//
//    Float_t At;
//	Float_t Bt;
//	Float_t Ct;
//	Float_t Dt;
//
//	Int_t Xt;
//	Int_t Yt;
//
//	Float_t At_err;
//	Float_t Bt_err;
//	Float_t Ct_err;
//	Float_t Dt_err;
//	Float_t Chi;
//
//
//	t->SetBranchAddress("pixx",&Xt);
//	t->SetBranchAddress("pixy",&Yt);
//	t->SetBranchAddress("a",&At);
//	t->SetBranchAddress("b",&Bt);
//	t->SetBranchAddress("c",&Ct);
//	t->SetBranchAddress("d",&Dt);
//
//	t->SetBranchAddress("a_err",&At_err);
//	t->SetBranchAddress("b_err",&Bt_err);
//	t->SetBranchAddress("c_err",&Ct_err);
//	t->SetBranchAddress("d_err",&Dt_err);
//
//	t->SetBranchAddress("chi2ndf",&Chi);
//
//	meanA=0;
//	meanB=0;
//	meanC=0;
//	meanD=0;
//
//
//	for(unsigned int i=0;i<256;i++){
//		for(unsigned int j=0;j<256;j++){
//
//		A[i][j]=0;
//		B[i][j]=0;
//		C[i][j]=0;
//		D[i][j]=0;
//
//	}};
//
//
//	int nevents = t->GetEntries();
//
//	for (int i =0 ; i<nevents; i++){
//
//		t->GetEntry(i);
//		A[Xt][Yt]=At;
//		B[Xt][Yt]=Bt;
//		C[Xt][Yt]=Ct;
//		D[Xt][Yt]=Dt;
//
//		Aerr[Xt][Yt]=At_err;
//		Berr[Xt][Yt]=Bt_err;
//		Cerr[Xt][Yt]=Ct_err;
//		Derr[Xt][Yt]=Dt_err;
//
//		Chi2ndf[Xt][Yt]=Chi;
//
//		meanA+=At;
//		meanB+=Bt;
//		meanC+=Ct;
//		meanD+=Dt;
//
//		}
//
//	meanA/=nevents;
//	meanB/=nevents;
//	meanC/=nevents;
//	meanD/=nevents;
//
//	f->Close();

    }



    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
	virtual unsigned GetTriggerID(const Event & ev) const 
	{
		const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
		if(rev->NumBlocks()==2)
		{
			vector<unsigned char> tlu  = rev->GetBlock(1);
			mimtlu_event tluev(tlu);
			//cout << tluev.tlu << endl;
			return tluev.tlu;	
		};

		return 0;
	}


    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent & sev,
                                     const Event & ev) const {
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "timepix";
      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
      // Set the number of pixels
      int width = 256, height = 256;

      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
      cout << "[Number of blocks] " << rev->NumBlocks() << endl;
      vector<unsigned char> data = rev->GetBlock(0);
      cout << "vector has size : " << data.size() << endl;

      vector<unsigned int> ZSDataX;
      vector<unsigned int> ZSDataY;
      vector<unsigned int> ZSDataTOT;
      size_t offset = 0;
      unsigned int aWord =0;

      for(unsigned int i=0;i<(data.size())/12;i++){

    	  unpack(data,offset,aWord);
    	  offset+=sizeof(aWord);
    	  ZSDataX.push_back(aWord);

    	  unpack(data,offset,aWord);
    	  offset+=sizeof(aWord);
    	  ZSDataY.push_back(aWord);
	  	  
	  aWord = 0;
	  for(unsigned int j=0;j<4;j++){
	  	//printf("%x",dataTOT[offset2+j]);
		aWord =aWord | (data[offset+j] << j*8);
		}

    	  offset+=sizeof(aWord);
    	  ZSDataTOT.push_back(aWord);
	  std::cout << "[DATA] "  << " " << ZSDataX[i] << " " << ZSDataY[i] << " " << ZSDataTOT[i] << std::endl;


      }
     
      unsigned int TLUevt = GetTriggerID(ev);
      
//      cout << "break here" << endl;

      plane.SetSizeZS(width,height,MATRIX_SIZE,1,1);
      // plane.SetSizeRaw(width, height);
      // Set the trigger ID
      plane.SetTLUEvent(TLUevt);
      // Add the plane to the StandardEvent

      for(unsigned int i = 0 ; i<ZSDataX.size();i++){

    	  plane.PushPixel(ZSDataX[i],ZSDataY[i],ZSDataTOT[i]);


      }
	      
	
      sev.AddPlane(plane);
      // Indicate that data was successfully converted
      
      return true;
    }
                   
#if USE_LCIO && USE_EUTELESCOPE
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
      return ConvertLCIOHeader(header, bore, conf);
    }

    virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const {
      return ConvertLCIO(lcioEvent, eudaqEvent);
    }
#endif

  private:

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    TimepixConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0)
      {}
#if USE_LCIO && USE_EUTELESCOPE


    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & /*conf*/) const {
      eutelescope::EUTelRunHeaderImpl runHeader(&header);

    }


    bool ConvertLCIO(lcio::LCEvent & result, const Event & source) const {
        //TrackerRawDataImpl *rawMatrix;
        TrackerDataImpl *zsFrame;

        if (source.IsBORE()) {
          // shouldn't happen
          return true;
        } else if (source.IsEORE()) {
          // nothing to do
          return true;
        }
        // If we get here it must be a data event

        // prepare the collections for the rawdata and the zs ones
        unique_ptr< lcio::LCCollectionVec > rawDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERRAWDATA) ) ;
        unique_ptr< lcio::LCCollectionVec > zsDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERDATA) ) ;

        // set the proper cell encoder
        CellIDEncoder< TrackerRawDataImpl > rawDataEncoder ( eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection.get() );
        CellIDEncoder< TrackerDataImpl > zsDataEncoder ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection.get() );


        // a description of the setup
        std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
        // FIXME hardcoded number of planes
        size_t numplanes = 1;
        std::string  mode;



        for (size_t iPlane = 0; iPlane < numplanes; ++iPlane) {

		  std::string sensortype = "timepix";
		 // Create a StandardPlane representing one sensor plane
		  int id = 6;
		  StandardPlane plane(id, EVENT_TYPE, sensortype);
		 // Set the number of pixels
		  int width = 256, height = 256;

		// The current detector is ...
		  eutelescope::EUTelPixelDetector * currentDetector = 0x0;

		  mode = "ZS";

		  currentDetector = new eutelescope::EUTelTimepixDetector;

	      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&source);
	      //cout << "[Number of blocks] " << rev->NumBlocks() << endl;
	      vector<unsigned char> data = rev->GetBlock(0);
	      //cout << "vector has size : " << data.size() << endl;

	      vector<unsigned int> ZSDataX;
	      vector<unsigned int> ZSDataY;
	      vector<double> ZSDataTOT;
	      size_t offset = 0;
	      unsigned int aWord =0;
	      int pixx,pixy;
	      for(unsigned int i=0;i<data.size()/12;i++){
    	 		 unpack(data,offset,aWord);
    	 		 offset+=sizeof(aWord);
    	 		 ZSDataX.push_back(aWord);
			 pixx=aWord;
    	 		 unpack(data,offset,aWord);
    	  		 offset+=sizeof(aWord);
    	 		 ZSDataY.push_back(aWord);
			 pixy=aWord;
	  	  
	 		 aWord = 0;
	 	 for(unsigned int j=0;j<4;j++){
	  	        //printf("%x",dataTOT[offset2+j]);
			aWord =aWord | (data[offset+j] << j*8);
			}

    	  offset+=sizeof(aWord);
	  
//	  double Energy;
//	  if(A[pixx][pixy]!=0 and B[pixx][pixy] and C[pixx][pixy] and D[pixx][pixy]){
//	  	 Energy = (D[pixx][pixy]*A[pixx][pixy] + aWord - B[pixx][pixy] + sqrt((B[pixx][pixy]+D[pixx][pixy]*A[pixx][pixy]-aWord)*(B[pixx][pixy]+D[pixx][pixy]*A[pixx][pixy]-aWord)+4*A[pixx][pixy]*C[pixx][pixy]))/(2*A[pixx][pixy]);
//		}
//	  else {
//	  	 Energy = (meanD*meanA + aWord - meanB + sqrt((meanB+meanD*meanA-aWord)*(meanB+meanD*meanA-aWord)+4*meanA*meanC))/(2*meanA);
//
//	  }
    	  ZSDataTOT.push_back(aWord);

	    	  //cout << "[DATA] " << ZSDataX[i] << " " << ZSDataY[i] << " " << ZSDataTOT[i] << endl;
	      }

	      plane.SetSizeZS(width,height,0);
	  	 // plane.SetSizeRaw(width, height);
	      // Set the trigger ID
	      plane.SetTLUEvent(GetTriggerID(source));

	      // Add the plane to the StandardEvent
	      for(unsigned int i = 0 ; i<ZSDataX.size();i++){

	    	  plane.PushPixel(255-ZSDataX[i],255-ZSDataY[i],ZSDataTOT[i]);

	      };


            /*---------------ZERO SUPP ---------------*/

      	  //printf("prepare a new TrackerData for the ZS data \n");
      	  // prepare a new TrackerData for the ZS data
          zsFrame= new TrackerDataImpl;
          currentDetector->setMode( mode );
          zsDataEncoder["sensorID"] = plane.ID();
      	  zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelSimpleSparsePixel;
      	  zsDataEncoder.setCellID( zsFrame );

           size_t nPixel = plane.HitPixels();
           //printf("EvSize=%d %d \n",EvSize,nPixel);
      	  for (unsigned i = 0; i < nPixel; i++) {
      	      //printf("EvSize=%d iPixel =%d DATA=%d  icol=%d irow=%d  \n",nPixel,i, (signed short) plane.GetPixel(i, 0), (signed short)plane.GetX(i) ,(signed short)plane.GetY(i));
	      //cout << ZSDataTOT[i] << endl;
      	      // Note X and Y are swapped - for 2010 TB DEPFET module was rotated at 90 degree.
      	      zsFrame->chargeValues().push_back(plane.GetX(i));
      	      zsFrame->chargeValues().push_back(plane.GetY(i));
      	      zsFrame->chargeValues().push_back(ZSDataTOT[i]);
      	  }

      	  zsDataCollection->push_back( zsFrame);

      	  if (  zsDataCollection->size() != 0 ) {
      	      result.addCollection( zsDataCollection.release(), "zsdata_timepix" );
      	  }

          }
          if ( result.getEventNumber() == 0 ) {

            // do this only in the first event

            LCCollectionVec * timepixSetupCollection = NULL;
            bool timepixSetupExists = false;
            try {
              timepixSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "timepixSetup" ) ) ;
              timepixSetupExists = true;
            } catch (...) {
              timepixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
            }

            for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

              timepixSetupCollection->push_back( setupDescription.at( iPlane ) );

            }

            if (!timepixSetupExists) {

              result.addCollection( timepixSetupCollection, "timepixSetup" );

            }
          }


          //     printf("DEPFETConverterBase::ConvertLCIO return true \n");
          return true;


    }
#endif

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static TimepixConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  TimepixConverterPlugin TimepixConverterPlugin::m_instance;

} // namespace eudaq
