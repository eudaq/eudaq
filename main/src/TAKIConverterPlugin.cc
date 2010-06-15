#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#  include "UTIL/CellIDEncoder.h"
#endif

#if USE_EUTELESCOPE
#  include "EUTELESCOPE.h"
#  include "EUTelRunHeaderImpl.h"
#  include "EUTelTakiDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelSimpleSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif



#include <iostream>
#include <string>
#include <vector>

namespace eudaq {

  /********************************************/

  class TAKIConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event & e, const Configuration & c);

#if USE_LCIO && USE_EUTELESCOPE
    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const;
    virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
      return ConvertLCIOHeader(header, bore, conf);
    }

    bool GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const;
#endif
    
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

  private:
    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const;
    TAKIConverterPlugin() : DataConverterPlugin("Taki")
    {
    }
   
    static TAKIConverterPlugin const m_instance;
  };


#if USE_LCIO && USE_EUTELESCOPE
  void TAKIConverterPlugin::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const
  {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
  }
  bool TAKIConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const
  {

     bool israw = true; //TODO: lets try to determine the event type once and then assume it.

     if(israw)
       {
         TrackerRawDataImpl *rawMatrix;
         if (source.IsBORE()) {
           //          shouldn't happen
           return true;
         } else if (source.IsEORE()) {
           // nothing to do
           return true;
         }
         // If we get here it must be a data event
         result.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );

         // prepare the collections for the rawdata and the zs ones
         std::auto_ptr< lcio::LCCollectionVec > rawDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERRAWDATA) ) ;
    
         // set the proper cell encoder
         CellIDEncoder< TrackerRawDataImpl > rawDataEncoder ( eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection.get() );
    
         // a description of the setup
         std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
          
         const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    
         for (size_t i = 0; i < ev.NumBlocks(); ++i) 
           {
             StandardPlane plane = ConvertPlane(ev.GetBlock(i), ev.GetID(i));
                
             // The current detector is ...
             eutelescope::EUTelPixelDetector * currentDetector = NULL;
        
             std::string  mode = "RAW2"; //mode correct? will this be used somewhere????
        
             currentDetector = new eutelescope::EUTelTakiDetector;
             rawMatrix = new TrackerRawDataImpl;
        
             currentDetector->setMode( mode );
             // storage of RAW data is done here according to the mode
             //printf("XMin =% d, XMax=%d, YMin=%d YMax=%d \n",currentDetector->getXMin(),currentDetector->getXMax(),currentDetector->getYMin(),currentDetector->getYMax());
             rawDataEncoder["xMin"]     = currentDetector->getXMin();
             rawDataEncoder["xMax"]     = currentDetector->getXMax();
             rawDataEncoder["yMin"]     = currentDetector->getYMin();
             rawDataEncoder["yMax"]     = currentDetector->getYMax();
             rawDataEncoder["sensorID"] = 6;
             rawDataEncoder.setCellID(rawMatrix);

             const short bias = 64; //pedestal bias used during the cern summer testbeam 2009


             //lets do it the hardcore brute force way ...
             std::vector<std::vector<short> > hardcore_matrix(
                                                              currentDetector->getXMax()+1,
                                                              std::vector<short>(currentDetector->getYMax()+1,0)
                                                              );
           

             const unsigned int hitpixel = plane.HitPixels();
             for (unsigned int i = 0; i < hitpixel; ++i) 
               {
                 const int x = plane.GetX(i);
                 const int y = plane.GetY(i);
                 hardcore_matrix.at(x).at(y) = plane.GetPixel(i, 0) - bias;
               }

             for (int yPixel = 0; yPixel <= currentDetector->getYMax(); yPixel++) {
               for (int xPixel = 0; xPixel <= currentDetector->getXMax(); xPixel++) {
                 short amp = 0; //what would be the best default value? maybe 0 because the pedestal and noise processor will then mask these pixels as bad pixels.
                 try
                   {
                     amp = hardcore_matrix.at(xPixel).at(yPixel);
                     //if((yPixel == 64 || yPixel == 65) && amp != 0)
                     //  std::cout << "amp = " << amp << std::endl;
                   } 
                 catch (...)
                   {
                     std::cout << "something really strange ... !" << std::endl;
                     exit(-1);
                   }
            
                 rawMatrix->adcValues().push_back(amp);
               }
             }
             rawDataCollection->push_back(rawMatrix);
        
             if ( result.getEventNumber() == 0 ) {
               setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
             }
        
             // add the collections to the event only if not empty!
             if ( rawDataCollection->size() != 0 ) {
               result.addCollection( rawDataCollection.release(), "rawdata_taki" );
             }
             // this is the right place to prepare the TrackerRawData
             // object
        
           }
         if ( result.getEventNumber() == 0 ) {
           // do this only in the first event

           LCCollectionVec * takiSetupCollection = NULL;
           bool takiSetupExists = false;
           try {
             takiSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "takiSetup" ) ) ;
             takiSetupExists = true;
           } catch (...) {
             takiSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
           }

           for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

             takiSetupCollection->push_back( setupDescription.at( iPlane ) );

           }

           if (!takiSetupExists) {

             result.addCollection( takiSetupCollection, "takiSetup" );

           }
         }
         return true;
       }

     return false; 
  }
#endif



  TAKIConverterPlugin const TAKIConverterPlugin::m_instance;

  void TAKIConverterPlugin::Initialize(const Event &, const Configuration &) {
  }
  
  bool TAKIConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    //printf(" [TAKIConvPlg::GetStdSubEv()]: TLU_eventnumber (as hex) from tag: %s\n", ev.GetTag("triggerID").c_str() );
    //printf(" [TAKIConvPlg::GetStdSubEv()]: This event contained %d Blocks\n", (int) ev.NumBlocks() );
    for (size_t i = 0; i < ev.NumBlocks(); ++i) 
      {
        //printf(" [TAKIConvPlg::GetStdSubEv()]  Block %d's ID is: %u.\n\n", (int) i, ev.GetID(i) );
        result.AddPlane(ConvertPlane(ev.GetBlock(i),
                                     ev.GetID(i)));
      }
    return true;
  }

  StandardPlane TAKIConverterPlugin::ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const
  {
    StandardPlane plane(id, "Taki", "Taki");

    unsigned tlu_eventnumber = 
      (( ((unsigned long) data[5])       ) & 0x000000FF)
      | (( ((unsigned long) data[4]) << 8  ) & 0x0000FF00)
      | (( ((unsigned long) data[3]) << 16 ) & 0x00FF0000)
      | (( ((unsigned long) data[2]) << 24 ) & 0xFF000000);
    /*
      tlu_eventnumber =     (( ((unsigned) *(dataPtr+5))       ) & 0x000000FF) //!only unsigned int available instead of ULL or UL!
      | (( ((unsigned) *(dataPtr+4)) << 8  ) & 0x0000FF00);
    */

    //! printf("[TAKIConverterPlugin::ConvertPlane()]: tlu_eventnumber (as hex) out of stream: %lx\n", tlu_eventnumber);

    plane.SetTLUEvent(( tlu_eventnumber ));

    //npixels is the number of pixels we want to fill
    unsigned int xsize = 128;
    unsigned int ysize = 128;

    int nDataBytesFromStreamContent = (data.size() -6);
    int nDataBytesFromStreamHeader  = (
                                       (( ((unsigned int) data[1])       ) & 0x000000FF)
                                       | (( ((unsigned int) data[0]) << 8  ) & 0x0000FF00)
                                       )
      - 6;


    //printf("RECEIVED ONE EVENT: TID=%u, numPixels=%d (judging from content length) \n",tlu_eventnumber,nDataBytesFromStreamContent/3);

    if (nDataBytesFromStreamContent != nDataBytesFromStreamHeader)
      printf("!!! number of data bytes from the stream contents = %d\n, number of data bytes from the stream header = %d\n\n !!!", 
             nDataBytesFromStreamContent, nDataBytesFromStreamHeader);
    //else
    //  printf("!!! nDataBytesFromStreamContent matches nDataBytesFromStreamHeader (%d)!!!",nDataBytesFromStreamContent);

    plane.SetSizeZS(xsize, ysize, (nDataBytesFromStreamContent/3)); // Set the size for one frames of ... * ...


    //use data in order to decode your sensor
    const unsigned char* dataPtr = &data[6]; //first six bytes contain length of incoming data (2 bytes) and TID (4 bytes)
    for(unsigned int i = 0; i < (unsigned int) (nDataBytesFromStreamContent/3); i++)
      {
        unsigned int x   = (unsigned int) *dataPtr;
        unsigned int y   = (unsigned int) *(dataPtr+1);
        unsigned int amp = (unsigned int) *(dataPtr+2);
        plane.SetPixel(i, x, y, amp); //! take care with this call!
        dataPtr+=3;
      }
    return plane;
  }

}//namespace eudaq
