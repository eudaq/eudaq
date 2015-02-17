#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"


#ifdef WIN32
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
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

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#endif

#include <iostream>
#include <iomanip>
#include <iterator>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cfloat>

using std::cout;
using std::cerr;
using std::endl;
using std::hex;
using std::dec;
using std::vector;
using std::pair;
using std::string;

namespace eudaq {
  /////////////////////////////////////////////////////////////////////////////////////////////////
  // Templates ////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////
  template <typename T>
  inline T unpack_fh (vector <unsigned char >::iterator &src, T& data){
    // unpack from host-byte-order
    data=0;
    for(unsigned int i=0;i<sizeof(T);i++){
      data+=((uint64_t)*src<<(8*i));
      src++;
    }
    return data;
  }

  template <typename T>
  inline T unpack_fn(vector<unsigned char>::iterator &src, T& data){
    // unpack from network-byte-order
    data=0;
    for(unsigned int i=0;i<sizeof(T);i++){
      data+=((uint64_t)*src<<(8*(sizeof(T)-1-i)));
      src++;
    }
    return data;
  }

  template <typename T>
  inline T unpack_b(vector<unsigned char>::iterator &src, T& data, unsigned int nb){
    // unpack number of bytes n-b-o only
    data=0;
    for(unsigned int i=0;i<nb;i++){
      data+=(uint64_t(*src)<<(8*(nb-1-i)));
      src++;
    }
    return data;
  }

  typedef pair<vector<unsigned char>::iterator,unsigned int> datablock_t;

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // CONVERTER ////////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////////////////////

  // Detector / Eventtype ID
  static const char* EVENT_TYPE = "pALPIDEssRaw";

  class PALPIDESSConverterPlugin : public DataConverterPlugin {
  public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // INITIALIZE /////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Take specific run data or configuration data from BORE
    virtual void Initialize(const Event & bore,
                            const Configuration & /*cnf*/) {    //GetConfig
      //TODO Take care of all the tags
      m_PalpidessPlaneNo = bore.GetTag<int>("PalpidessPlaneNo", 0);
      m_Vbb              = bore.GetTag<string>("Vbb",       "-4.");
      m_Vrst             = bore.GetTag<string>("Vrst",      "-4.");
      m_Vcasn            = bore.GetTag<string>("Vcasn",     "-4.");
      m_Vcasp            = bore.GetTag<string>("Vcasp",     "-4.");
      m_Ithr             = bore.GetTag<string>("Ithr" ,     "-4.");
      m_Vlight           = bore.GetTag<string>("Vlight",    "-4.");
      m_AcqTime          = bore.GetTag<string>("AcqTime",   "-4.");
      m_TrigDelay        = bore.GetTag<string>("TrigDelay", "-4.");
      cout << "PalpidessPlaneNo=" << m_PalpidessPlaneNo << endl;
      cout << "Vbb="              << m_Vbb          << endl;
      cout << "Vrst="             << m_Vrst         << endl;
      cout << "Vcasn="            << m_Vcasn        << endl;
      cout << "Vcasp="            << m_Vcasp        << endl;
      cout << "Ithr="             << m_Ithr         << endl;
      cout << "Vlight="           << m_Vlight       << endl;
      cout << "AcqTime="          << m_AcqTime      << endl;
      cout << "TrigDelay="        << m_TrigDelay    << endl;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // GetTRIGGER ID //////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////
    //Get TLU trigger ID from your RawData or better from a different block
    //example: has to be fittet to our needs depending on block managing etc.
    virtual unsigned GetTriggerID(const Event & ev) const {
      // Make sure the event is of class RawDataEvent
      static unsigned trig_offset = (unsigned)-1;
      if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
        // TLU ID is contained in the trailer of the last data frame in the 4th and
        // 3rd byte from the end
        // in the calculation of the offset also the end-of-event frame and its
        // length has to be taken in to account:
        // - eoe frame = 9 byte + 4 byte
        // - 2 byte after the trigger id => 4 including the tlu id
        const unsigned int offset = 9 + 4 + 4 + 4;
        if (rev->NumBlocks() < 1) return (unsigned)(-1);
        unsigned int size = rev->GetBlock(0).size();
        if (size < offset) return (unsigned)(-1);
        unsigned int pos = size - offset;
        unsigned int id = getbigendian<unsigned int>(&rev->GetBlock(0)[pos]);
        if (trig_offset==(unsigned)-1) {
          trig_offset=id;
        }
        return (unsigned)(id-trig_offset);
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    virtual int IsSyncWithTLU(eudaq::Event const & ev,eudaq::TLUEvent const & tlu) const {
      unsigned triggerID=GetTriggerID(ev);
      auto tlu_triggerID=tlu.GetEventNumber();
      if (triggerID==(unsigned)-1) return Event_IS_LATE;
      else return compareTLU2DUT(tlu_triggerID,triggerID);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // STANDARD SUBEVENT //////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////

    //conversion from Raw to StandardPlane format
    virtual bool GetStandardSubEvent(StandardEvent & sev,const Event & ev) const {

      //Conversion
      //Reading of the RawData
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
      //cout << "[Number of blocks] " << rev->NumBlocks() << endl;
      if(rev->NumBlocks()==0) return false;
      vector<unsigned char> data = rev->GetBlock(0);
      //cout << "vector has size : " << data.size() << endl;

      //data iterator and element access number
      vector<unsigned char>::iterator it=data.begin();

      /////////////////////////////////////////////////////////////////////////////////////////////
      // DATA CONSISTENCY CHECK ///////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////
      // ## Format ##
      // Event length                    4 Byte (host-byte-order / hbo)
      //
      // Frame length                    4 Byte (hbo)
      // Event number                    8 Byte (network-byte-order / nbo)
      // Data specifier                  3 Byte (nbo)
      // Frame count                     1 Byte
      // Data            (Frame length-12) Byte (nbo)
      //
      // Trailer                        24 Byte (last data frame only)
      //
      // (End of Event Frame)
      // Framelength                     4 Byte ( = 9 by definition)
      // Event Number                    8 Byte
      // Max Frame Count                 1 Byte ( varies )
      //###########################################################################################

      //numbers extracted from the header
      unsigned int EVENT_LENGTH, FRAME_LENGTH, DS/*Data specifier*/, ds;
      uint64_t EVENT_NUM, evn;

      //array to save the iterator starting point of each frame ordered by frame number 16+1
      //Save starting point and block length
      datablock_t framepos[9];
      //variable to move trough the array
      unsigned int iframe=1;
      unsigned int nframe=0;

      // check whether the data is consistent
      if(data.size()-4!=unpack_fh(it,EVENT_LENGTH)) {
        cout<<"Event Length ERROR:\n";
        cout<<"\tEvent Length not correct!"<<endl
            <<"\tEither order not correct or Event corrupt!!"
            <<endl<<"\tExpected Length: "<<EVENT_LENGTH
            <<" Seen: "<<data.size()-4<<endl;
        return false;
      }
      unpack_fh(it,FRAME_LENGTH);
      if(FRAME_LENGTH==9){ //discard events with EoE-Frame as first frame, as they are empty or corrupted
        printf("\nFirst Frame received was an EoE-Frame => discarding event!\n");
        return false;
      }
      unpack_fn(it,EVENT_NUM);  //event id
      unpack_b(it,DS,3);        //data specifier
      framepos[(int)*it]=make_pair(it+1,FRAME_LENGTH-12); // id and position of this frame
      it++;
      it+=FRAME_LENGTH-12;      //move on to the begining of the next frame
      while(it!=data.end()){
        if(unpack_fh(it,FRAME_LENGTH)==9) { // EoE-Frame
          if(unpack_fn(it,evn)==EVENT_NUM){
            framepos[8]=make_pair(data.end(),FRAME_LENGTH);
            iframe++;
            nframe=(unsigned int)(*it)+1;
            it++;
          }
          else{
            cout<<"EOE ERROR:"<<endl<<"\tEvent Number not correct!"<<endl<<"\tFound: "<<evn<<" instead of "<<EVENT_NUM<<endl
                <<"Dropping Event"<<endl;
            sev.SetFlags(Event::FLAG_BROKEN);
            return false;
          }
        }
        else{
          if(unpack_fn(it,evn)!=EVENT_NUM){             //EvNum exception
            cout<<"Event Number ERROR: "<<endl<<"\tEventnumber mismatch!"<<endl<<"\tDropping Event!"<<endl;
            sev.SetFlags(Event::FLAG_BROKEN);
            return false;
          }
          if( unpack_b(it,ds,3)!=DS ){
            cout<<"Data Specifier ERROR:\n"<<"\tData specifier do not fit!"<<endl<<"\tDropping Event!"<<endl;
            sev.SetFlags(Event::FLAG_BROKEN);
            return false;
          }
          //if everything went correct up to here the next byte should be the Framecount
          if((int)*it<=0 || (int)*it>7){
            cout<<"Frame Counter ERROR:\n"<<"\tFramecount not possible! Range: 0..7 + EoE-frame"<<endl<<"\tDropping Event"<<endl;
            sev.SetFlags(Event::FLAG_BROKEN);
            return false;
          }
          framepos[(int)*it]=make_pair(it+1,FRAME_LENGTH-12);   //Set FrameInfo
          it++;
          iframe++;
          if(iframe>7){               //this should not be possible but anyway
            cout<<"Frame ERROR:\n\tToo many frames!"<<endl<<"\tDropping Event"<<endl;
            sev.SetFlags(Event::FLAG_BROKEN);
            return false;
          }
          it+=FRAME_LENGTH-12;  //Set iterator to begin of next frame
        }
      }

      unsigned int nhit=((EVENT_LENGTH)-nframe*(12+4)-24-(9+4))/2; // estimate the number of hits

      /////////////////////////////////////////////////////////////////////////////////////////////
      // Payload Decoding /////////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////
      uint16_t buffer     = 0x0;
      uint16_t buffer_old = 0x0;
      iframe=0;

      //initialize everything
      string sensortype = "pALPIDEss";
      // create a StandardPlane representing one sensor plane
      int id = 0;
      // plane dimensions
      unsigned int width  =  64;
      unsigned int height = 512;

      StandardPlane plane(id, EVENT_TYPE, sensortype);
      plane.SetSizeZS(width, height, 0, 1, StandardPlane::FLAG_ZS);

      vector<unsigned char>::iterator end;
      it=framepos[iframe].first;
      end=it+framepos[iframe].second;

      unsigned int nrealhit = 0;
      for (unsigned int ihit=0; ihit<nhit; ++ihit) {
        unpack_b(it, buffer, 2);
        if (buffer>0 && buffer<=buffer_old) {
//          cerr << "Priority violated!" << endl;
          sev.SetFlags(Event::FLAG_BROKEN);
        }
        if (buffer&0x8000) {
          unsigned int row = height-1-(buffer&0x1FF);
          unsigned int col = width-1-(buffer>>9&0x3F);
          //cout << "good 0x" << hex << (unsigned int)buffer << dec << '\t' << col << '\t' << row << endl;
          plane.PushPixel(col, row, 1, 0U);
          ++nrealhit;
        }
        //else {
        //  cout << "bad  0x" << hex << (unsigned int)buffer << dec << endl;
        //}
        if(it==end){
          iframe++;
          it=framepos[iframe].first;
          end=it+framepos[iframe].second;
        }
        buffer_old = buffer;
      }
      //if (nrealhit==0) cerr << "Empty pALPIDEss event!" << endl;
      //else {
      //  cout << nhit << '\t' << nrealhit << endl;
      //  cout << endl << endl;
      //}

      /////////////////////////////////////////////////////////////////////////////////////////////
      // Trailer Conversion ///////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////////
      // Total:                 24 Byte
      //  EventTimestamp         8 Byte
      //  TriggerTimestamp       8 Byte
      //  TriggerCounter         4 Byte
      //  TluCounter             2 Byte
      //  StatusInfo             2 Byte
      //
      uint64_t EvTS;       // Event Timestamp
      uint64_t TrTS;       // Trigger Timestamp
      unsigned int TrCnt;  // Trigger Counter
      unsigned int TluCnt; // TLU Counter
      unsigned int SI;     // Status Info
      //
      // unpack that stuff
      unpack_fn(it, EvTS);
      unpack_fn(it, TrTS);
      unpack_fn(it, TrCnt);
      unpack_b(it, TluCnt, 2);
      unpack_b(it, SI, 2);

      // Add the planes to the StandardEvent
      plane.SetTLUEvent(TluCnt);
      sev.SetTimestamp(EvTS-TrTS);
      sev.AddPlane(plane);

      // Indicate that data was successfully converted
      return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // LCIO Converter //////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////
#if USE_LCIO
    virtual bool GetLCIOSubEvent(lcio::LCEvent & lev, eudaq::Event const & ev) const {
      if (ev.IsBORE() || ev.IsEORE()) return true;
      StandardEvent sev;
      GetStandardSubEvent(sev,ev);
      StandardPlane plane = sev.GetPlane(0);

      lev.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,eutelescope::kDE);
      lcio::LCCollectionVec* pALPIDEssDataCollection = 0x0;
      bool pALPIDEssDataCollectionExists = false;

      try {
        pALPIDEssDataCollection = static_cast<LCCollectionVec*>(lev.getCollection("zsdata_pALPIDEss"));
        pALPIDEssDataCollectionExists = true;
      }
      catch (lcio::DataNotAvailableException &) {
        pALPIDEssDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
        pALPIDEssDataCollectionExists = false;
      }

      CellIDEncoder<lcio::TrackerDataImpl> pALPIDEssDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING,pALPIDEssDataCollection);
      lcio::TrackerDataImpl *pALPIDEssFrame = new lcio::TrackerDataImpl();
      pALPIDEssDataEncoder["sensorID"       ]=m_PalpidessPlaneNo;
      pALPIDEssDataEncoder["sparsePixelType"]=eutelescope::kEUTelSimpleSparsePixel;
      pALPIDEssDataEncoder.setCellID(pALPIDEssFrame);

      const vector<StandardPlane::pixel_t>& x_values = plane.XVector();
      const vector<StandardPlane::pixel_t>& y_values = plane.YVector();

      for (unsigned int ihit = 0; ihit < x_values.size(); ++ihit) {
        pALPIDEssFrame->chargeValues().push_back((int)x_values.at(ihit));
        pALPIDEssFrame->chargeValues().push_back((int)y_values.at(ihit));
        pALPIDEssFrame->chargeValues().push_back(1);
      }
      pALPIDEssDataCollection->push_back(pALPIDEssFrame);
      if (pALPIDEssDataCollection->size()!=0) lev.addCollection(pALPIDEssDataCollection, "zsdata_pALPIDEss");
      else if (!pALPIDEssDataCollectionExists) { delete pALPIDEssDataCollection; pALPIDEssDataCollection = 0x0; }

      lev.parameters().setValue("FLAG", (int) sev.GetFlags());
      lev.parameters().setValue("PalpidessPlaneNo", m_PalpidessPlaneNo);
      lev.parameters().setValue("Vbb"             , m_Vbb         );
      lev.parameters().setValue("Vrst"            , m_Vrst        );
      lev.parameters().setValue("Vcasn"           , m_Vcasn       );
      lev.parameters().setValue("Vcasp"           , m_Vcasp       );
      lev.parameters().setValue("Ithr"            , m_Ithr        );
      lev.parameters().setValue("Vlight"          , m_Vlight      );
      lev.parameters().setValue("AcqTime"         , m_AcqTime     );
      lev.parameters().setValue("TrigDelay"       , m_TrigDelay   );
      // TODO add all the option and so on
      return true;
    }
#endif
  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    PALPIDESSConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE)
      , m_PalpidessPlaneNo(0)
      , m_Vbb()
      , m_Vrst()
      , m_Vcasn()
      , m_Vcasp()
      , m_Ithr()
      , m_Vlight()
      , m_AcqTime()
      , m_TrigDelay()
      {}

    // Information extracted in Initialize() can be stored here:
    // TODO add all the stuff here

    // The single instance of this converter plugin
    static PALPIDESSConverterPlugin m_instance;
    int m_PalpidessPlaneNo;
    string m_Vbb;
    string m_Vrst;
    string m_Vcasn;
    string m_Vcasp;
    string m_Ithr;
    string m_Vlight;
    string m_AcqTime;
    string m_TrigDelay;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  PALPIDESSConverterPlugin PALPIDESSConverterPlugin::m_instance;
} // namespace eudaq
