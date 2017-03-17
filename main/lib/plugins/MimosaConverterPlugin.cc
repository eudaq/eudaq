
#include "eudaq/MimosaConverterPlugin.hh"

#define MATRIX_SIZE 65536

using namespace std;

namespace eudaq {
  
    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
  bool MimosaConverterPlugin::GetStandardSubEvent(StandardEvent & sev,
                                     const Event & ev) {

   
      // If the event type is used for different sensors
      // they can be differentiated here
      std::string sensortype = "MIMOSA32";
      // Create a StandardPlane representing one sensor plane
      int id = 0;
      StandardPlane plane(id, EVENT_TYPE, sensortype);
    

   
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent* > (&ev);
      cout << "[Number of blocks] " << rev->NumBlocks() << endl;

      vector<unsigned char> Data = rev->GetBlock(2);
      copy(Data.begin(),Data.end(),fData.begin());
      cout << "vector has size : " << fData.size() << endl;

      NewEvent();
      ReadData();

   
      plane.SetSizeRaw(kRowPerChip,kColPerChip,2,2); 

      plane.SetTLUEvent(GetTriggerID(ev));
      int count=0;
      for(int i = 0 ; i<kRowPerChip;i++){
	for(int j=0; j< kColPerChip; i++){
	  plane.SetPixel(count,j,i,fDataFrame[i][j],false,1);
	  plane.SetPixel(count,j,i,fDataFrame2[i][j],false,2);
	  count++;
	}
      }
      
      //      plane.SetupResult();
      sev.AddPlane(plane);
      // Indicate that data was successfully converted
      return true;
    }
    
#if USE_LCIO && USE_EUTELESCOPE
    // virtual void GetLCIORunHeader(lcio::LCRunHeader & header, eudaq::Event const & bore, eudaq::Configuration const & conf) const {
    //   return ConvertLCIOHeader(header, bore, conf);
    // }

    // virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const {
    //   return ConvertLCIO(lcioEvent, eudaqEvent);
    // }
#endif


    //********************************************************************************************
    //********************************************************************************************
 
    //********************************************************************************************
    //********************************************************************************************

#if USE_LCIO && USE_EUTELESCOPE


    // void ConvertLCIOHeader(lcio::LCRunHeader & header, eudaq::Event const & /*bore*/, eudaq::Configuration const & /*conf*/) const {
    //   eutelescope::EUTelRunHeaderImpl runHeader(&header);

    // }


    // bool ConvertLCIO(lcio::LCEvent & result, const Event & source) const {
    //     TrackerRawDataImpl *rawMatrix;
    //     TrackerDataImpl *zsFrame;

    //     if (source.IsBORE()) {
    //       // shouldn't happen
    //       return true;
    //     } else if (source.IsEORE()) {
    //       // nothing to do
    //       return true;
    //     }
    //     // If we get here it must be a data event

    //     // prepare the collections for the rawdata and the zs ones
    //     unique_ptr< lcio::LCCollectionVec > rawDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERRAWDATA) ) ;
    //     unique_ptr< lcio::LCCollectionVec > zsDataCollection ( new lcio::LCCollectionVec (lcio::LCIO::TRACKERDATA) ) ;

    //     // set the proper cell encoder
    //     CellIDEncoder< TrackerRawDataImpl > rawDataEncoder ( eutelescope::EUTELESCOPE::MATRIXDEFAULTENCODING, rawDataCollection.get() );
    //     CellIDEncoder< TrackerDataImpl > zsDataEncoder ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection.get() );


    //     // a description of the setup
    //     std::vector< eutelescope::EUTelSetupDescription * >  setupDescription;
    //     // FIXME hardcoded number of planes
    //     size_t numplanes = 1;
    //     std::string  mode;



    //     for (size_t iPlane = 0; iPlane < numplanes; ++iPlane) {

    // 		  std::string sensortype = "timepix";
    // 		 // Create a StandardPlane representing one sensor plane
    // 		  int id = 6;
    // 		  StandardPlane plane(id, EVENT_TYPE, sensortype);
    // 		 // Set the number of pixels
    // 		  int width = 256, height = 256;

    // 		// The current detector is ...
    // 		  eutelescope::EUTelPixelDetector * currentDetector = 0x0;

    // 		  mode = "ZS";

    // 		  currentDetector = new eutelescope::EUTelTimepixDetector;

    // 	      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&source);
    // 	      //cout << "[Number of blocks] " << rev->NumBlocks() << endl;
    // 	      vector<unsigned char> data = rev->GetBlock(0);
    // 	      //cout << "vector has size : " << data.size() << endl;

    // 	      vector<unsigned int> ZSDataX;
    // 	      vector<unsigned int> ZSDataY;
    // 	      vector<unsigned int> ZSDataTOT;
    // 	      size_t offset = 0;
    // 	      unsigned int aWord =0;
    // 	      for(int i=0;i<data.size()/12;i++){
    // 	    	  unpack(data,offset,aWord);
    // 	    	  offset+=sizeof(aWord);
    // 	    	  ZSDataX.push_back(aWord);

    // 	    	  unpack(data,offset,aWord);
    // 	    	  offset+=sizeof(aWord);
    // 	    	  ZSDataY.push_back(aWord);

    // 	    	  unpack(data,offset,aWord);
    // 	    	  offset+=sizeof(aWord);
    // 	    	  ZSDataTOT.push_back(aWord);

    // 	    	  //cout << "[DATA] " << ZSDataX[i] << " " << ZSDataY[i] << " " << ZSDataTOT[i] << endl;
    // 	      }

    // 	      plane.SetSizeZS(width,height,0);
    // 	  	 // plane.SetSizeRaw(width, height);
    // 	      // Set the trigger ID
    // 	      plane.SetTLUEvent(GetTriggerID(source));

    // 	      // Add the plane to the StandardEvent
    // 	      for(int i = 0 ; i<ZSDataX.size();i++){

    // 	    	  plane.PushPixel(255-ZSDataX[i],255-ZSDataY[i],ZSDataTOT[i]);

    // 	      };


    //         /*---------------ZERO SUPP ---------------*/

    //   	  //printf("prepare a new TrackerData for the ZS data \n");
    //   	  // prepare a new TrackerData for the ZS data
    //       zsFrame= new TrackerDataImpl;
    //       currentDetector->setMode( mode );
    //       zsDataEncoder["sensorID"] = plane.ID();
    //   	  zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;
    //   	  zsDataEncoder.setCellID( zsFrame );

    //        size_t nPixel = plane.HitPixels();
    //        //printf("EvSize=%d %d \n",EvSize,nPixel);
    //   	  for (unsigned i = 0; i < nPixel; i++) {
    //   	      //printf("EvSize=%d iPixel =%d DATA=%d  icol=%d irow=%d  \n",nPixel,i, (signed short) plane.GetPixel(i, 0), (signed short)plane.GetX(i) ,(signed short)plane.GetY(i));
    // 	      //cout << ZSDataTOT[i] << endl;
    //   	      // Note X and Y are swapped - for 2010 TB DEPFET module was rotated at 90 degree.
    //   	      zsFrame->chargeValues().push_back(plane.GetX(i));
    //   	      zsFrame->chargeValues().push_back(plane.GetY(i));
    //   	      zsFrame->chargeValues().push_back(ZSDataTOT[i]);
    //   	  }

    //   	  zsDataCollection->push_back( zsFrame);

    //   	  if (  zsDataCollection->size() != 0 ) {
    //   	      result.addCollection( zsDataCollection.release(), "zsdata_timepix" );
    //   	  }

    //       }
    //       if ( result.getEventNumber() == 0 ) {

    //         // do this only in the first event

    //         LCCollectionVec * timepixSetupCollection = NULL;
    //         bool timepixSetupExists = false;
    //         try {
    //           timepixSetupCollection = static_cast< LCCollectionVec* > ( result.getCollection( "timepixSetup" ) ) ;
    //           timepixSetupExists = true;
    //         } catch (...) {
    //           timepixSetupCollection = new LCCollectionVec( lcio::LCIO::LCGENERICOBJECT );
    //         }

    //         for ( size_t iPlane = 0 ; iPlane < setupDescription.size() ; ++iPlane ) {

    //           timepixSetupCollection->push_back( setupDescription.at( iPlane ) );

    //         }

    //         if (!timepixSetupExists) {

    //           result.addCollection( timepixSetupCollection, "timepixSetup" );

    //         }
    //       }


    //       //     printf("DEPFETConverterBase::ConvertLCIO return true \n");
    //       return true;


    // }
#endif

    //********************************************************************************************
    //********************************************************************************************
    //Custom methods and member from AliMimosa....
    //********************************************************************************************
    //********************************************************************************************

  bool MimosaConverterPlugin::ReadNextInt() {
      // reads next 32 bit into fDataChar1..4 
      // (if first 16 bits read already, just completes the present word)
      

      char ch1[2];
      char ch2[2];
      char ch3[2];
      char ch4[2];

      unsigned char word[4];

      for (int i=0;i<4;i++){
	unpack(fData,fOffset+4,word[i]);   
      }

      sprintf(ch1, "%02X",(unsigned char)word[0]);
      sprintf(ch2, "%02X",(unsigned char)word[1]);
      sprintf(ch3, "%02X",(unsigned char)word[2]);
      sprintf(ch4, "%02X",(unsigned char)word[3]);

      char ch[8];
      strcat(ch,ch1);
      strcat(ch,ch2);
      strcat(ch,ch3);
      strcat(ch,ch4);

      unsigned int tmpInt=0;
      for (int i=0; i<8; i++) {
        int multiply=1;
	for (int j=0; j<i; j++) {
          multiply*=16;
	}
        unsigned int tmpVal=0;
	if      (ch[7-i] == '0') {tmpVal=0;}
	else if (ch[7-i] == '1') {tmpVal=1;}
	else if (ch[7-i] == '2') {tmpVal=2;}
	else if (ch[7-i] == '3') {tmpVal=3;}
	else if (ch[7-i] == '4') {tmpVal=4;}
	else if (ch[7-i] == '5') {tmpVal=5;}
	else if (ch[7-i] == '6') {tmpVal=6;}
	else if (ch[7-i] == '7') {tmpVal=7;}
	else if (ch[7-i] == '8') {tmpVal=8;}
	else if (ch[7-i] == '9') {tmpVal=9;}
	else if (ch[7-i] == 'A' || ch[7-i] == 'a') {tmpVal=10;}
	else if (ch[7-i] == 'B' || ch[7-i] == 'b') {tmpVal=11;}
	else if (ch[7-i] == 'C' || ch[7-i] == 'c') {tmpVal=12;}
	else if (ch[7-i] == 'D' || ch[7-i] == 'd') {tmpVal=13;}
	else if (ch[7-i] == 'E' || ch[7-i] == 'e') {tmpVal=14;}
	else if (ch[7-i] == 'F' || ch[7-i] == 'f') {tmpVal=15;}
	tmpInt+=tmpVal*multiply;
      }
  
      fDataChar1 = tmpInt & 0x000000FF;
      fDataChar2 = (tmpInt >> 8) & 0x000000FF;
      fDataChar3 = (tmpInt >> 16) & 0x000000FF;
      fDataChar4 = (tmpInt >> 24) & 0x000000FF;

      
      return true;
    }


  bool MimosaConverterPlugin::ReadFrame(short data[][kColPerChip]) {

      for(short row=0; row<kRowPerChip;row++){
	short col=0;
	unsigned short tmpbits = 0x0000;    
	for(short i=0; i<5; i++){
	  if(!ReadNextInt()){ 
	    printf("Error Incomplete Frame\n");
	    return false;
	  }
	  else{
	    fOffset=fOffset+4;
	  }

	  unsigned int word = fDataChar1+(fDataChar2<<8)+(fDataChar3<<16)+(fDataChar4<<24);
	  data[row][col] = ((word << 2*i) + tmpbits) & 0x000003FF; col++;
	  data[row][col] = (word >> (10-2*i)) & 0x000003FF; col++;
	  data[row][col] = (word >> (20-2*i)) & 0x000003FF; col++;
	  tmpbits = word >> (32 - (2*i+2));
	}
	data[row][col] = tmpbits & 0x000003FF;
      }
      
      return true;
      
    }



  bool MimosaConverterPlugin::ReadData() {

      if(!ReadFrame(fDataFrame)){
	printf("Error Incomplete Frame\n");
	return false;
      }
      if(!ReadFrame(fDataFrame2)){
	printf("Error Incomplete Frame\n");
	return false;
      } //Then take out

      return true;
    }


  void MimosaConverterPlugin::NewEvent()  {
      // call this to reset flags for a new event
      fOffset = 0;
      for(int i=0;i<kRowPerChip;i++){
	for(int j=0;j<kColPerChip;j++){ 
	  fDataFrame[i][j]=-1; fDataFrame2[i][j]=-1;}}

    }

   


    //********************************************************************************************
    //********************************************************************************************
    //********************************************************************************************
    //********************************************************************************************





} // namespace eudaq
