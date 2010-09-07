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
#  include "EUTelFortisDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelSimpleSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

// Define number of rows, columns in entire FORTIS
#define FORTIS_ROWS 512
#define FORTIS_COLUMNS 448


#include <iostream>
#include <string>
#include <vector>

namespace eudaq {

  /********************************************/

  class FORTISConverterPlugin : public DataConverterPlugin {
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
    FORTISConverterPlugin() : DataConverterPlugin("FORTIS"),
			      m_NumRows(FORTIS_ROWS), m_NumColumns(FORTIS_COLUMNS),
			      m_InitialRow(0), m_InitialColumn(0) {}
    unsigned m_NumRows, m_NumColumns, m_InitialRow, m_InitialColumn;
    static FORTISConverterPlugin const m_instance;
    mutable StandardPlane m_frameDataBuffer;
    mutable unsigned m_parentEvent;

  };


#if USE_LCIO && USE_EUTELESCOPE
  void FORTISConverterPlugin::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const
  {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
  }
  bool FORTISConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & result, const Event & source) const
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
        
             std::string  mode = "ZS2"; //mode correct? will this be used somewhere????
        
             currentDetector = new eutelescope::EUTelFortisDetector;
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

             //lets do it the hardcore brute force way ...
             std::vector<std::vector<short> > hardcore_matrix(
                                                              currentDetector->getXMax()+1,
                                                              std::vector<short>(currentDetector->getYMax()+1,0)
                                                              );
           
	     //const unsigned int frameNumber = 0; // for now just look at the first frame out of two.
             const unsigned int hitpixel = plane.HitPixels();
             for (unsigned int i = 0; i < hitpixel; ++i) 
               {
                 const int x = (int) plane.GetX(i); //,frameNumber);
                 const int y = (int) plane.GetY(i); //,frameNumber);
                 hardcore_matrix.at(x).at(y) = (short int) plane.GetPixel(i); //, frameNumber);
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
               result.addCollection( rawDataCollection.release(), "rawdata_fortis" );
             }
             // this is the right place to prepare the TrackerRawData
             // object
        
           }
         if ( result.getEventNumber() == 0 ) {
           // do this only in the first event

           LCCollectionVec * fortisSetupCollection = NULL;
           bool fortisSetupExists = false;
           try {
             fortisSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "FortisSetup" ) ) ;
             fortisSetupExists = true;
           } catch (...) {
             fortisSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
           }

           for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

             fortisSetupCollection->push_back( setupDescription.at( iPlane ) );

           }

           if (!fortisSetupExists) {

             result.addCollection( fortisSetupCollection, "fortisSetup" );

           }
         }
         return true;
       }
     return false; // bodge up to avoid compiler warning
  }
#endif


  FORTISConverterPlugin const FORTISConverterPlugin::m_instance;

  void FORTISConverterPlugin::Initialize(const Event & source, const Configuration &) {
    //std::cout << "FORTISConverterPlugin::Initialize::" << std::endl;
    EUDAQ_EXTRA("FORTISConverterPlugin::Initialize:: ");

    m_NumRows = from_string(source.GetTag("NumRows"), FORTIS_ROWS);
    m_NumColumns = from_string(source.GetTag("NumColumns"), FORTIS_COLUMNS);
    m_InitialRow = from_string(source.GetTag("InitialRow"), 0);
    m_InitialColumn = from_string(source.GetTag("InitialColumn"), 0);

    //std::cout << " FORTIS: Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    EUDAQ_EXTRA(std::string(" FORTIS: Nrows , NColumns = ")+to_string(m_NumRows)+"  ,  "+to_string(m_NumColumns)+": ");
    //std::cout << " FORTIS: Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
    EUDAQ_EXTRA(std::string(" FORTIS: Initial row , column = ")+to_string(m_InitialRow)+"  ,  "+to_string(m_InitialColumn)+": ");
  }

  bool FORTISConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
#   if FORTIS_DEBUG
    std::cout << "FORTISConverterPlugin::GetStandardSubEvent" << std::endl;
#   endif
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    if ( ev.NumBlocks() > 1) {
      // more than one fortis not yet implemented
      EUDAQ_THROW("More than one FORTIS not yet supported");
    } else if (ev.NumBlocks() == 1) {
      // if we have the frame data, then take a copy so that events with only pivot row info can refer to it
      m_frameDataBuffer = ConvertPlane(ev.GetBlock(0),ev.GetID(0));
      m_parentEvent = ev.GetEventNumber();
    } else if ( m_parentEvent != ev.GetTag("PARENTEVENT",0U)) { // if we get here we have no frame data, so check that parent event is correct...
	EUDAQ_ERROR("FORTIS PARENT EVENT doesn't match .... ");
    }

    StandardPlane & localPlane = result.AddPlane(m_frameDataBuffer); // add plane to data from copy.

    unsigned pivotRow =  ev.GetTag("PIVOTROW",0);
    localPlane.SetPivotPixel( pivotRow ); // set the pivot pixel to be the pivot row

    // now loop through setting the pivot pixel bits.
    for (size_t pixel = 0; pixel < localPlane.HitPixels(0); ++pixel) {
      localPlane.SetPivot( pixel, 0 ,  (localPlane.GetY(pixel,0) < pivotRow ) );
    }

    return true;
  }


  StandardPlane FORTISConverterPlugin::ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
#   if FORTIS_DEBUG
    std::cout << "FORTISConverterPlugin::ConvertPlane:" << std::endl;
#   endif
    size_t expected = ((m_NumColumns + 2) * m_NumRows) * sizeof (short);
    if (data.size() < expected)
      EUDAQ_THROW("Bad Data Size (" + to_string(data.size()) + " < " + to_string(expected) + ")");
    StandardPlane plane(id, "FORTIS", "FORTIS");
    unsigned npixels = m_NumRows * m_NumColumns;
    unsigned nwords =  m_NumRows * ( m_NumColumns + 2);

#   if FORTIS_DEBUG
    std::cout << "FORTIS:: Size of data (bytes)= " << data.size() << std::endl ;
#   endif
    
    plane.SetSizeZS(FORTIS_ROWS, FORTIS_COLUMNS, npixels, 2 , StandardPlane::FLAG_WITHPIVOT); // Set the size for two frames of 512*512

    unsigned int frame_number[2];                   //frame_number was 2
    
    unsigned int triggerRow = 0;
    
    for (size_t Frame = 0; Frame < 2; ++Frame ) {           //Frame was <2
      
      size_t i = 0;

      unsigned int frame_offset = (Frame*nwords) * sizeof (short);

      frame_number[Frame] = getlittleendian<unsigned int>(&data[frame_offset]);

#     if FORTIS_DEBUG
      std::cout << "FORTIS:: Frame = " << Frame << std::endl;
      std::cout << "FORTIS:: frame number in data = " << eudaq::to_hex(frame_number[Frame]) << std::endl;
#     endif

      for (size_t Row = 0; Row < m_NumRows; ++Row) {

        unsigned int header_offset = ( (m_NumColumns+sizeof(short))*Row + Frame*nwords) * sizeof (short);

	unsigned short TriggerWord = getlittleendian<unsigned short>(&data[header_offset]);
	unsigned short TriggerCount = TriggerWord & 0x00FF;

#       if FORTIS_DEBUG
	unsigned short LatchWord   =  getlittleendian<unsigned short>(&data[header_offset+sizeof (short) ]) ;
        std::cout << "Row = " << Row << " Header = " << eudaq::to_hex(TriggerWord) << "    " << LatchWord << std::endl ;
#       endif
	if ( (triggerRow == 0) && (TriggerCount >0 )) { triggerRow = Row ; }

        for (size_t Column = 0; Column < m_NumColumns; ++Column) {
          unsigned offset = (Column + 2 + (m_NumColumns + 2) * Row + Frame*nwords) * sizeof (short);
          unsigned short d = getlittleendian<unsigned short>(&data[offset]);
          plane.SetPixel(i, Column + m_InitialColumn, Row + m_InitialRow, d, (unsigned)Frame); // need to put option to have inversion at this stage ( ie. put in 0xFFFF -d rather than d. This is because by selecting jumpers can read out FORTIS with singled ended *inverting* amplifier.
          ++i;
        }
      }
    }

    /* Check that the two frames have frame numbers separated by one */
    if ( frame_number[1] != (frame_number[0] + 1) ) {
      std::cout << "Frame_number[0] ,  Frame_number[1] = " << frame_number[0] <<"   "<< frame_number[1] << std::endl;

      EUDAQ_THROW("FORTIS data corruption: Frame_number[1] != Frame_number[0]+1 (" + to_string(frame_number[1]) + " != " + to_string(frame_number[0]) + " +1 )");
      } 


#   if FORTIS_DEBUG
    std::cout << "Trigger Row = 0x" << triggerRow << std::endl;
#   endif

    return plane;
  }



}//namespace eudaq
