#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/DEPFETEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

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
#  include "EUTelDEPFETDetector.h"
#  include "EUTelSetupDescription.h"
#  include "EUTelEventImpl.h"
#  include "EUTelSparseDataImpl.h"
#  include "EUTelSimpleSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

#include <iostream>
#include <string>
#include <vector>

#define TB2010bonding 1 

#ifdef TB2010bonding
 static int SWITCHERBMAP[16]={1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14};
#else
 static int SWITCHERBMAP[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
#endif

int DCDCOLMAPPING[128]={ 0, 4, 8, 12, 2, 6, 10, 14, 124, 120,
                        116, 112, 126, 122, 118, 114, 16, 20, 24, 28,
                         18, 22, 26, 30, 108, 104, 100, 96, 110, 106,
                         102, 98, 36, 35, 37, 34, 52, 51, 53, 50,
                         68,67, 69, 66, 84, 83, 85, 82, 38, 33,
                         39,32, 54, 49, 55, 48, 70, 65, 71, 64,
                         86, 81, 87, 80,  1, 5, 9, 13, 3, 7,
                         11, 15, 125, 121, 117, 113, 127, 123, 119, 115, 17,
                         21, 25, 29, 19, 23, 27, 31, 109, 105,
                         101, 97, 111, 107, 103, 99, 44, 43, 45,
                         42, 60, 59, 61, 58, 76, 75, 77, 74, 92,
                         91, 93, 90, 46, 41, 47, 40, 62, 57, 63,
                         56, 78, 73, 79, 72, 94, 89, 95, 88 };

int ZEROSUPP=0;
int EvSize=0;
int DEV_FOLD=0;
namespace eudaq {


  class DEPFETConverterBase {
  public:

#if USE_LCIO && USE_EUTELESCOPE
    void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const;
    bool ConvertLCIO(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const;
#endif

    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
      StandardPlane plane(id, "DEPFET", "DEPFET");
      plane.SetTLUEvent(getlittleendian<unsigned>(&data[4]));
      int DevType=(getlittleendian<unsigned>(&data[0])>>28) & 0xf;
      //printf("TLU=%d Startgate=%d DevType=0x%x \n",plane.m_tluevent,Startgate,DevType);
      //unsigned int TriggerID = getlittleendian<unsigned>(&data[4]);
      //printf("DEPFET:: TLU=%d  DevType=0x%x \n",TriggerID,DevType);
      if (DevType==0x4) DevType=5;
      if (DevType==0x3) {
        plane.SetSizeRaw(64, 256);
       int Startgate=(getlittleendian<unsigned>(&data[8])>>10) & 0x7f;

        for (unsigned gate = 0; gate < plane.YSize()/2; ++gate)  {      // Pixel sind uebereinander angeordnet
          int readout_gate = (Startgate + gate) % (plane.YSize()/2);
          int odderon = readout_gate % 2;  // = 0 for even, = 1 for odd;

          for (unsigned col = 0; col < plane.XSize()/2; col += 2) {

            for (unsigned icase = 0; icase < 8; ++icase) {
              int ix;
              switch (icase % 4) {
              case 0: ix = plane.XSize() - 1 - col; break;
              case 1: ix = col; break;
              case 2: ix = plane.XSize() - 2 - col; break;
              default: ix = col + 1; break;
              }
              int iy = readout_gate * 2 + (icase == 1 || icase == 3 || icase == 4 || icase == 6 ? odderon : 1 - odderon);
              int j = gate * plane.YSize() / 2 + col*4 + icase;

              plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
            } // icase
          } // col
        } // gates

      }    else  if (DevType==0x6) { // S3B 4-fold readout
	  plane.SetSizeRaw(32, 512);
	  int Startgate=(getlittleendian<unsigned>(&data[8])>>10) & 0x7f;
	  
	  for (unsigned gate = 0; gate < plane.YSize()/4; ++gate)  {      // Pixel sind uebereinander angeordnet
	      int readout_gate = (Startgate + gate) % (plane.YSize()/4);
	      
	      for (unsigned col = 0; col < plane.XSize()/2; col += 1) {
		  
		  int ix,iy,j;
		  // 0 
		  ix=plane.XSize()-1-col; iy=readout_gate*4+3; j = gate * plane.YSize() / 4 + col*8 + 0 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 1 
		  ix=col;                 iy=readout_gate*4+0;  j = gate * plane.YSize() / 4 + col*8 + 1 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
                  // 2 
		  ix=plane.XSize()-1-col; iy=readout_gate*4+2; j = gate * plane.YSize() / 4 + col*8 + 2 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 3
		  ix=col;                 iy=readout_gate*4+1; j = gate * plane.YSize() / 4 + col*8 + 3 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 4
		  ix=plane.XSize()-1-col; iy=readout_gate*4+1; j = gate * plane.YSize() / 4 + col*8 + 4 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 5
		  ix=col;                 iy=readout_gate*4+2; j = gate * plane.YSize() / 4 + col*8 + 5 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 6
		  ix=plane.XSize()-1-col; iy=readout_gate*4+0; j = gate * plane.YSize() / 4 + col*8 + 6 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
		  // 7
		  ix=col;                 iy=readout_gate*4+3; j = gate * plane.YSize() / 4 + col*8 + 7 ;
		  plane.SetPixel(ix*plane.YSize() + iy, ix, iy, getlittleendian<unsigned short>(&data[12 + 2*j]));
	      } // col
	  } // gates
	  
      } else if (DevType==0x4) {  
	  
//------- DCD readout ---------- 

//       printf("DEV_TYPE=4\n");
       int EvSize=(getlittleendian<unsigned>(&data[0])) & 0xfffff;
       int Trig=(getlittleendian<unsigned>(&data[4]));
       int Startgate=(getlittleendian<unsigned>(&data[8])>>10) & 0x3f;
       ZEROSUPP=(getlittleendian<unsigned>(&data[8])>>20) & 0x1;

//for DCD         plane.SetSizeRaw(128, 16);
// dcd converted to Matrix + rotated at 90 degree
//        plane.SetSizeRaw(32, 64);
        plane.SetSizeRaw(64, 32);
	int i=-1;
        int icol,irow,icold,irowd;
        std::vector<double> DATA1(plane.XSize() * plane.YSize());
//        unsigned npixels = plane.XSize() * plane.YSize();
//	unsigned char *v4data=(unsigned char *)data[3];
	if( ZEROSUPP==0){

		for (unsigned irowdcd = 0; irowdcd < plane.YSize()/2; irowdcd ++)  {     
		    int readout_gate = (Startgate + irowdcd) % (plane.YSize()/2);
		    irowd=SWITCHERBMAP[readout_gate];
		    int odderon = irowd %2; 
		    for (unsigned icoldcd = 0; icoldcd < plane.XSize()*2; icoldcd ++) {
			icold=DCDCOLMAPPING[icoldcd];	
			i++;
			int odderon_c = icold %2;
			double d = getlittleendian<signed char>(&data[12 + i]);
			if(odderon==0) {  //--  even row
			if(odderon_c==0) { 
			    irow=(irowd*2)%(plane.YSize());
			    icol=icold/2;
			    //               if( icoldcd<4) printf(" gate=%d, irowd=%d odderon=%d,odderon_c=%d, icold=%d, [icol=%d,irow=%d] data=%f\n",readout_gate,irowd,odderon,odderon_c,icold,icol,irow,d);
			} else {            
			    irow=((irowd*2)%(plane.YSize()))+1;
			    icol=icold/2;
			}
			} else {
			    if(odderon_c==0) { 
				irow=((irowd*2)%(plane.YSize()))+1;
				icol=icold/2;
				
			    } else {
				irow=(irowd*2)%(plane.YSize());
				icol=icold/2;
			    }
		    }
			DATA1[icol*plane.YSize()+irow]= d;               
			
			//     if(i<10) printf("DEV_TYPE=DEPFET icol=%d,irow=%d data=%f\n",icol,irow,d);
			//	    if(d>-0.1 && d<0.1)  printf("QQQ ipix=%d icol=%d,irow=%d   d=%f  DATA1=%f \n",i, icol,irow, d,DATA1[icol*plane.YSize()+irow]);
		    plane.SetPixel(icol*plane.YSize() + irow, icol, irow, DATA1[icol*plane.YSize() + irow]);
		    
		    };
		};
//	    for (int iy = 0; iy < plane.YSize(); iy ++)  {     
//		for (int ix = 0; ix < plane.XSize(); ix ++) {
//		    if(iy==27 && ix<10 ) printf("Xmax=%d , ymax=%d  icol=%d,irow=%d data=%f\n",plane.XSize(),plane.YSize(),ix,iy,DATA1[ix*plane.YSize() + iy]);
//		    plane.SetPixel(ix*plane.YSize() + iy, ix, iy, DATA1[ix*plane.YSize() + iy]);
//		}
//	    }
		

	} else { // Zero-supp.
		double d;
		int readout_gate;
	    int ipix, irowdcd,icol,icoldcd, irowd,irow,i=-1;
	    /* ONLY FOR ZS data */
	    printf("ZERO SUPPRESS\n");
	    printf("data.size()=%d Trig=%d 0x%x\n",EvSize,Trig,Trig);
 
	    for(ipix=0; ipix<(EvSize-3)*4; ipix+=3){
                i++;
                icoldcd= ((getlittleendian<unsigned>(&data[12+ipix])) & 0xff)-1;
                icold=DCDCOLMAPPING[icoldcd];
		int odderon_c = icold %2;
		irowdcd= ((getlittleendian<unsigned>(&data[12+ipix+1])) & 0xff)-1;
                readout_gate = (Startgate + irowdcd) % (plane.YSize());
                irowd=SWITCHERBMAP[readout_gate];
                int odderon = irowd %2; 
                d= ((getlittleendian<signed char>(&data[12+ipix+2])) & 0xff);

                if(icold>127 || irowd>15 || d<0.1 ) {i--; continue;}
 
		if(odderon==0) {  //--  even row
                    if(odderon_c==0) { 
			irow=(irowd*2)%(plane.YSize());
			icol=icold/2;
                    } else {            
			irow=((irowd*2)%(plane.YSize()))+1;
			icol=icold/2;
                    }
		} else {
                    if(odderon_c==0) { 
			irow=((irowd*2)%(plane.YSize()))+1;
			icol=icold/2;
                    } else {
			irow=(irowd*2)%(plane.YSize());
			icol=icold/2;
                    }
		}

		if(icol>63 && irow>31) {printf(" AAAH! icol=%d ,irow=%d \n",icol,irow); i--; continue;}
                printf("Decode ZS data =icol=%d 0x%x, irow=%d 0x%x, ADC=%x,%f\n",icol,icol,irow,irow,((getlittleendian<signed char>(&data[12+ipix+2])) & 0xff),d);
 		plane.SetPixel(i, icol, irow, d);

            } 
	    plane.SetSizeZS(plane.XSize(), plane.YSize(),i+1);

	    /* END ONLY FOR ZS data */
	}
      } else if (DevType==0x5) {  // 4-fold readout DCD

//       printf("DEV_TYPE=44\n");
       int EvSize=(getlittleendian<unsigned>(&data[0])) & 0xfffff;
       int Trig=(getlittleendian<unsigned>(&data[4]));
       int Startgate=(getlittleendian<unsigned>(&data[8])>>10) & 0x3f;
       ZEROSUPP=(getlittleendian<unsigned>(&data[8])>>20) & 0x1;

       plane.SetSizeRaw(32, 64);  
	int i=-1;
        int icol,irow,icold,irowd;
        std::vector<double> DATA1(plane.XSize() * plane.YSize());

	if( ZEROSUPP==0){ 
	    for (unsigned irowdcd = 0; irowdcd < plane.YSize()/4; irowdcd ++)  {     
		int readout_gate = (Startgate + irowdcd) % (plane.YSize()/4);
		for (unsigned icoldcd = 0; icoldcd < plane.XSize()*4; icoldcd ++) {
		    icold=DCDCOLMAPPING[icoldcd];	
		    i++;
		    double d = getlittleendian<signed char>(&data[12 + i]);

		    irow=(readout_gate*4+3-icold%4)%(plane.YSize());
		    icol=(icold/4)%(plane.XSize());
		    DATA1[icol*plane.YSize()+irow]= d;               
		    
		    //     if(i<10) printf("DEV_TYPE=DEPFET icol=%d,irow=%d data=%f\n",icol,irow,d);
		    //	    if(d>-0.1 && d<0.1)  printf("QQQ ipix=%d icol=%d,irow=%d   d=%f  DATA1=%f \n",i, icol,irow, d,DATA1[icol*plane.YSize()+irow]);
		    plane.SetPixel(icol*plane.YSize() + irow, icol, irow, DATA1[icol*plane.YSize() + irow]);
				    
		};
	    };
	}
      }
     else { //------- S3A system -------
        plane.SetSizeRaw(64, 128);
        unsigned npixels = plane.XSize() * plane.YSize();
        for (size_t i = 0; i < npixels; ++i) {
          unsigned d = getlittleendian<unsigned>(&data[(3 + i)*4]);
          plane.SetPixel(i, (d >> 16) & 0x3f, (d >> 22) & 0x7f, d & 0xffff);
        }
      }
      return plane;
    }

  protected:
    static size_t NumPlanes(const Event & event) {
      if (const RawDataEvent * ev = dynamic_cast<const RawDataEvent *>(&event)) {
        return ev->NumBlocks();
      } else if (const DEPFETEvent * ev = dynamic_cast<const DEPFETEvent *>(&event)) {
        return ev->NumBoards();
      }
      return 0;
    }

    static std::vector<unsigned char> GetPlane(const Event & event, size_t i) {
      if (const RawDataEvent * ev = dynamic_cast<const RawDataEvent *>(&event)) {
        return ev->GetBlock(i);
      } else if (const DEPFETEvent * ev = dynamic_cast<const DEPFETEvent *>(&event)) {
        return ev->GetBoard(i).GetDataVector();
      }
      return std::vector<unsigned char>();
    }

    static size_t GetID(const Event & event, size_t i) {
      if (const RawDataEvent * ev = dynamic_cast<const RawDataEvent *>(&event)) {
        return ev->GetID(i);
      } else if (const DEPFETEvent * ev = dynamic_cast<const DEPFETEvent *>(&event)) {
        return ev->GetBoard(i).GetID();
      }
      return 0;
    }

  }; // end Base

  /********************************************/

  class DEPFETConverterPlugin : public DataConverterPlugin, public DEPFETConverterBase {
  public:
    //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;

    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

    virtual unsigned GetTriggerID(Event const & ev) const {
      const RawDataEvent & rawev = dynamic_cast<const RawDataEvent &>(ev);
      if (rawev.NumBlocks() < 1) return (unsigned)-1;
      return getlittleendian<unsigned>(&rawev.GetBlock(0)[4]);
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
    DEPFETConverterPlugin() : DataConverterPlugin("DEPFET") {}

    static DEPFETConverterPlugin const m_instance;
  };

  DEPFETConverterPlugin const DEPFETConverterPlugin::m_instance;

  bool DEPFETConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      //FillInfo(source);
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    for (size_t i = 0; i < ev.NumBlocks(); ++i) {
      result.AddPlane(ConvertPlane(ev.GetBlock(i),
                                   ev.GetID(i)));
    }
    return true;
  }

#if USE_LCIO && USE_EUTELESCOPE
  void DEPFETConverterBase::ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & /*conf*/) const {
    eutelescope::EUTelRunHeaderImpl runHeader(&header);
//    runHeader.setDAQHWName(EUTELESCOPE::DEPFET);
    //   printf("DEPFETConverterBase::ConvertLCIOHeader \n");

    // unsigned numplanes = bore.GetTag("BOARDS", 0);
    // runHeader.setNoOfDetector(numplanes);
    //     std::vector<int> xMin(numplanes, 0), xMax(numplanes, 128), yMin(numplanes, 0), yMax(numplanes, 16);
    // runHeader.setMinX(xMin);
    // runHeader.setMaxX(xMax);
    // runHeader.setMinY(yMin);
    // runHeader.setMaxY(yMax);
  }

  bool DEPFETConverterBase::ConvertLCIO(lcio::LCEvent & result, const Event & source) const {
    TrackerRawDataImpl *rawMatrix;
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
    std::auto_ptr< lcio::LCCollectionVec > rawDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERRAWDATA) ) ;
    std::auto_ptr< lcio::LCCollectionVec > zsDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERDATA) ) ;

    // set the proper cell encoder
    CellIDEncoder< TrackerRawDataImpl > rawDataEncoder ( eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection.get() );
   CellIDEncoder< TrackerDataImpl > zsDataEncoder ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection.get() );


    // a description of the setup
    std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
    size_t numplanes = NumPlanes(source);
    std::string  mode;
    for (size_t iPlane = 0; iPlane < numplanes; ++iPlane) {

      StandardPlane plane = ConvertPlane(GetPlane(source, iPlane), GetID(source, iPlane));
      //     printf("DEPFETConverterBase::ConvertLCIO numplanes=%d  \n", numplanes);
      // The current detector is ...
      eutelescope::EUTelPixelDetector * currentDetector = 0x0;
//       printf("DEPFETConverterBase::ConvertLCIO   iplane=%d  \n", iPlane);
 
      //    if ( plane.m_sensor == "DEPFET" ) {
       if(ZEROSUPP==0) {mode = "RAW2"; } else { mode = "ZS";}

      currentDetector = new eutelescope::EUTelDEPFETDetector;


      if(ZEROSUPP==0) {
	  rawMatrix = new TrackerRawDataImpl;
	  
	  currentDetector->setMode( mode );
	  // storage of RAW data is done here according to the mode
	  //printf("XMin =% d, XMax=%d, YMin=%d YMax=%d \n",currentDetector->getXMin(),currentDetector->getXMax(),currentDetector->getYMin(),currentDetector->getYMax());
	  rawDataEncoder["xMin"]     = currentDetector->getXMin();
	  rawDataEncoder["xMax"]     = currentDetector->getXMax();
	  rawDataEncoder["yMin"]     = currentDetector->getYMin();
	  rawDataEncoder["yMax"]     = currentDetector->getYMax();
// for 2009      rawDataEncoder["sensorID"] = 6;
	  rawDataEncoder["sensorID"] = 8;
	  rawDataEncoder.setCellID(rawMatrix);
	  
	  //size_t nPixel = plane.HitPixels();
//	    printf(" max X=%d, max Y=%d  \n",currentDetector->getXMax(),currentDetector->getYMax());
	  for (int yPixel = 0; yPixel <= currentDetector->getYMax(); yPixel++) {
	      for (int xPixel = 0; xPixel <= currentDetector->getXMax(); xPixel++) {
//	    if( yPixel==14 && (xPixel==0 ||xPixel==4 || xPixel==8 || xPixel==12))  printf("xPixel =%d yPixel=%d DATA=%d %f \n",xPixel, yPixel,(signed short) plane.GetPixel(xPixel*(currentDetector->getYMax()+1) + yPixel, 0), plane.GetPixel(xPixel*(currentDetector->getYMax()+1) + yPixel, 0) ); 
//	    printf("xPixel =%d yPixel=%d DATA=%d %f \n",xPixel, yPixel,(signed short) plane.GetPixel(xPixel*(currentDetector->getYMax()+1) + yPixel, 0), plane.GetPixel(xPixel*(currentDetector->getYMax()+1) + yPixel, 0) );
		  rawMatrix->adcValues().push_back((int)plane.GetPixel(xPixel*(currentDetector->getYMax()+1) + yPixel, 0));
	      }
	  }
	  rawDataCollection->push_back(rawMatrix);
	  
	  if ( result.getEventNumber() == 0 ) {
	      setupDescription.push_back( new eutelescope::EUTelSetupDescription( currentDetector )) ;
	  }
	  
	  // add the collections to the event only if not empty!
	  if ( rawDataCollection->size() != 0 ) {
	      result.addCollection( rawDataCollection.release(), "rawdata_dep" );
	  }
	  // this is the right place to prepare the TrackerRawData
	  // object
      } else { /*---------------ZERO SUPP ---------------*/

	  printf("prepare a new TrackerData for the ZS data \n");
	  // prepare a new TrackerData for the ZS data
	  //  std::auto_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
          zsFrame= new TrackerDataImpl;
     	  currentDetector->setMode( mode );
//	  zsDataEncoder["sensorID"] = plane.ID();
	  zsDataEncoder["sensorID"] = 8;
	  zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelSimpleSparsePixel;
	  zsDataEncoder.setCellID( zsFrame );

           size_t nPixel = plane.HitPixels();
//	   printf("EvSize=%d %d \n",EvSize,nPixel);
	  for (unsigned i = 0; i < nPixel; i++) {
	      printf("EvSize=%d iPixel =%d DATA=%d  icol=%d irow=%d  \n",nPixel,i, (signed short) plane.GetPixel(i, 0), (signed short)plane.GetX(i) ,(signed short)plane.GetY(i));

	      // Note X and Y are swapped - for 2010 TB DEPFET module was rotated at 90 degree. 
	      zsFrame->chargeValues().push_back(plane.GetX(i));
	      zsFrame->chargeValues().push_back(plane.GetY(i));
	      zsFrame->chargeValues().push_back(plane.GetPixel(i, 0));
	  }

	  zsDataCollection->push_back( zsFrame);

	  if (  zsDataCollection->size() != 0 ) {
	      result.addCollection( zsDataCollection.release(), "zsdata_dep" );
	  }

      }

    }
    if ( result.getEventNumber() == 0 ) {

      // do this only in the first event

      LCCollectionVec * depfetSetupCollection = NULL;
      bool depfetSetupExists = false;
      try {
        depfetSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "depfetSetup" ) ) ;
        depfetSetupExists = true;
      } catch (...) {
        depfetSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
      }

      for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

        depfetSetupCollection->push_back( setupDescription.at( iPlane ) );

      }

      if (!depfetSetupExists) {

        result.addCollection( depfetSetupCollection, "depfetSetup" );

      }
    }


    //     printf("DEPFETConverterBase::ConvertLCIO return true \n");
    return true;
  }

#endif

  /********************************************/

  class LegacyDEPFETConverterPlugin : public DataConverterPlugin, public DEPFETConverterBase {
    virtual bool GetStandardSubEvent(StandardEvent &, const Event & source) const;
  private:
    LegacyDEPFETConverterPlugin() : DataConverterPlugin(Event::str2id("_DEP")){}
    static LegacyDEPFETConverterPlugin const m_instance;
  };

  bool LegacyDEPFETConverterPlugin::GetStandardSubEvent(StandardEvent & result, const eudaq::Event & source) const {
    std::cout << "GetStandardSubEvent " << source.GetRunNumber() << ", " << source.GetEventNumber() << std::endl;
    if (source.IsBORE()) {
      //FillInfo(source);
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const DEPFETEvent & ev = dynamic_cast<const DEPFETEvent &>(source);
    for (size_t i = 0; i < ev.NumBoards(); ++i) {
      result.AddPlane(ConvertPlane(ev.GetBoard(i).GetDataVector(),
                                   ev.GetBoard(i).GetID()));
    }
    return true;
  }

  LegacyDEPFETConverterPlugin const LegacyDEPFETConverterPlugin::m_instance;

} //namespace eudaq
