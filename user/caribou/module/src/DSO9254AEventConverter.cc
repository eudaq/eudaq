#include "CaribouEvent2StdEventConverter.hh"
#include "utils/log.hpp"
#include "CLICTDFrameDecoder.hpp"


using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<DSO9254AEvent2StdEventConverter>(DSO9254AEvent2StdEventConverter::m_id_factory);
}

//Timestamp constants...
//Start date? Testbeam start, at 00:00:00:00
int startYear = 2021;
int startMonth = 10;
int startDay = 18;
std::map<std::string, int> monthMap {
  {"JAN", 1},
  {"FEB", 2},
  {"MAR", 3},
  {"APR", 4},
  {"MAY", 5},
  {"JUN", 6},
  {"JUL", 7},
  {"AUG", 8},
  {"SEP", 9},
  {"OCT", 10},
  {"NOV", 11},
  {"DEC", 12}
};


//Function converting the timestamp (date and time) to number of ms since a set start date...
//If we could just get the run starting time, that would be much better. This is a bit silly.
uint64_t getTimeSinceStart(std::string & date, std::string & clockTime) {
    //Parse date
    int year = 2021;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int millisecond = 0;
    //Substring, to remove "
    std::stringstream strStream(date.substr(1, date.size()-2));
    std::string currWord;
    std::vector<std::string> results;
    while(getline(strStream, currWord, ' ')) {
        results.push_back(currWord);
    }
    day = std::stoi(results[0]);
    month = monthMap.find(results[1])->second;
    year = std::stoi(results[2]);
    //Now the time (which is easier)
    strStream = std::stringstream(clockTime.substr(1, date.size()-2));
    results.clear();
    while(getline(strStream, currWord, ':')) {
        results.push_back(currWord);
    }
    hour = std::stoi(results[0]);
    minute = std::stoi(results[1]);
    second = std::stoi(results[2]);
    millisecond = std::stoi(results[3]);
    
    //Let's skip very different months and years for now... It comes with more headaches. Why don't we have 30 days in every single one?? Bloody Romans
    if(month-startMonth != 0) {
       std::cout << "Let's assume it is now November." << std::endl;
       day += (31-startDay);
    }
    uint64_t timestamp = ((day-startDay)*24*60*60 + hour*60*60 + minute*60 + second)*1000 + millisecond;
    return timestamp;
}

bool DSO9254AEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  
  // TODO
  // Store waveforms to root file?
  // feed channel number and waveform foreward... started... does it work/ help?
  
  // FIXME load from config file
  // integration window for pedestal and waveform
  double pedestalStartTime = -200; //[ns]
  double pedestalEndTime =  -50;
  double amplitudeStartTime =    0;
  double amplitudeEndTime =  100;
  
  
  // No event
  if(!ev) {
    return false;
  }

  // Data containers:
  std::vector<uint64_t> timestamps;
  caribou::pearyRawData rawdata;
  // internal containers
  std::vector<double> time;
  std::vector<std::vector<double>> waves;
  std::vector<double> dx;
  std::vector<double> x0;
  std::vector<double> dy;
  std::vector<double> y0;
  std::vector<double> ped;
  std::vector<double> avg;
  std::vector<double> amp;
  
  std::vector<std::string> date;
  std::vector<std::string> clockTime;
  std::vector<uint64_t> timeSinceStart;

  // Retrieve data from event
  if(ev->NumBlocks() == 1) {
    
    // all four scope channels in one data block
    auto datablock = ev->GetBlock(0);
    LOG(DEBUG) << "DSO9254A frame with "<< datablock.size()<<" entries";

    // Number of timestamps: first word of data
    uint64_t block_words;
    uint64_t pream_words;
    uint64_t chann_words;
    
    // Calulate positions and length of data blocks:
    // FIXME FIXME FIXME by Simon: this is prone to break since you are selecting bits from a 64bit word - but there is no guarantee in the endianness here and you might end up having the wrong one. Also, there is no guarantee for all four channels to have the same preamble and data length - theu should, but better check...
    memcpy(&block_words, &datablock[0], sizeof(uint64_t));
    memcpy(&pream_words, &datablock[8], sizeof(uint64_t));
    memcpy(&chann_words, &datablock[8*(pream_words+2)], sizeof(uint64_t));
    LOG(DEBUG) << "        " << block_words << " words per waveform\n";
    LOG(DEBUG) << "        " << pream_words << " words per preamble\n";
    LOG(DEBUG) << "        " << chann_words << " words per channel data\n";
    
    // loop 4 channels
    for( uint64_t nch = 0; nch < 4; nch++ ){
      
      // container for one waveform
      std::vector<double> wave;
      
      // read preamble:
      // FIXME can this be slimmer?
      char c;
      std::string preamble = "";
      for(int i =16 + nch * 8 * (block_words+1); 
              i <16 + nch * 8 * (block_words+1) + 8*pream_words;
         i+=8){
        memcpy(&c, &datablock[i], sizeof(char));
        preamble += c;
      }
      LOG(DEBUG) << preamble;
      std::cout << preamble << std::endl;
    
      // parse preamble to vector of strings
      std::string s;
      std::vector<std::string> vals;
      std::istringstream stream(preamble);
      while(std::getline(stream,s,',')){
        vals.push_back(s);
      }
      
      // might still help for online sync
      //if( nch == 1 ){
      //  std::cout << "Event time: " << vals[16] << std::endl;
      //}
      //Time
      date.push_back(vals[15]);
      clockTime.push_back(vals[16]);
      timeSinceStart.push_back(getTimeSinceStart(date[nch], clockTime[nch]));
      
            
      // pick preamble elements
      dx.push_back( stod( vals[4]) * 1e9 );
      x0.push_back( stod( vals[5]) * 1e9 );
      dy.push_back( stod( vals[7]) );
      y0.push_back( stod( vals[8]) );
      
      // read channel data
      int16_t word;
      int16_t wfi = 0;
      for(int i =(8*(pream_words+3+nch*(block_words+1))); 
              i <(8*(pream_words+3+nch*(block_words+1)))+8000;i+=2){
        memcpy(&word, &datablock[i], sizeof(int16_t));
        if(word!=0x0){
          if( nch == 0 ){ // this we need only once
            time.push_back( wfi * dx.at(nch) + x0.at(nch) );
          }
          wave.push_back( (word * dy.at(nch) + y0.at(nch)) );
        }
        wfi++;
      } // for data block
      
      waves.push_back( wave );
      ped.push_back( 0. );
      avg.push_back( 0. );
      amp.push_back( 0. );
    
    } // for channel
    
  } // for numblock
  
  // re-iterate waveforms
  std::ofstream file( "waveforms.txt");
  for( int i = 0; i < time.size(); i++ ) {
    
    try{
    
      // calculate pedestal and signal
      if( time.at(i) >= pedestalStartTime && time.at(i) < pedestalEndTime ){
        ped.at(0) += waves.at(0).at(i);
        ped.at(1) += waves.at(1).at(i);
        ped.at(2) += waves.at(2).at(i);
        ped.at(3) += waves.at(3).at(i);
        //std::cout << i << " " << ped.at(0) << std::endl;
      }
      if( time.at(i) >= amplitudeStartTime && time.at(i) < amplitudeEndTime ){
        avg.at(0) += waves.at(0).at(i);
        avg.at(1) += waves.at(1).at(i);
        avg.at(2) += waves.at(2).at(i);
        avg.at(3) += waves.at(3).at(i);\
        //Just getting the maximum amplitude point for each channel
        if( amp.at(0) < waves.at(0).at(i) ) amp.at(0) = waves.at(0).at(i);
        if( amp.at(1) < waves.at(1).at(i) ) amp.at(1) = waves.at(1).at(i);
        if( amp.at(2) < waves.at(2).at(i) ) amp.at(2) = waves.at(2).at(i);
        if( amp.at(3) < waves.at(3).at(i) ) amp.at(3) = waves.at(3).at(i);
        //std::cout << i << " " << amp.at(0) << std::endl;
      }
    
      // write waveforms to textfile for online analysis
      file << time.at(i) << " "
          << waves.at(0).at(i) << " "
          << waves.at(1).at(i) << " "
          << waves.at(2).at(i) << " "
          << waves.at(3).at(i) << std::endl;
    }catch(...){ // FIXME why are some data blocks shorter than the preamble suggests?
      
    }
  } // times
  
  // print pedestal etc
  for( uint64_t nch = 0; nch < 4; nch++ ){
    ped.at(nch) = ped.at(nch) * dx.at(nch) / (pedestalEndTime-pedestalStartTime);
    avg.at(nch) = avg.at(nch) * dx.at(nch) / (amplitudeEndTime-amplitudeStartTime) - ped.at(nch);
    amp.at(nch) = amp.at(nch) - ped.at(nch);
      std::cout << "Pedestal ch" << nch+1 << " " << ped.at(nch) << std::endl;
      std::cout << "Avg. Signal ch" << nch+1 << " " << avg.at(nch) << std::endl;
      std::cout << "Amplitude ch" << nch +1<< " " << amp.at(nch) << "\n" << std::endl;
  }
  
  
  // now feed that foreward
  
  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "Caribou", "DSO9254A");
  plane.SetSizeZS(2, 2, 0);
  plane.PushPixel( 1, 1, amp.at(0), timeSinceStart.at(0) ); //FIXME Timestamp proper, how is the orrientation wrt beam?
  plane.PushPixel( 1, 0, amp.at(1), timeSinceStart.at(1) );
  plane.PushPixel( 0, 0, amp.at(2), timeSinceStart.at(2) );
  plane.PushPixel( 0, 1, amp.at(3), timeSinceStart.at(3) );
  
  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Identify the detetor type
  d2->SetDetectorType("DSO9254A");
  
  // Indicate that data were successfully converted
  return true;
}




