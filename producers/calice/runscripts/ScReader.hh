#ifndef SCREADER_HH
#define SCREADER_HH

#include "CaliceReceiver.hh"

namespace calice_eudaq {

  class ScReader : public CaliceReader {
    public:
      virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent);
      virtual void OnStart(int runNo);
      virtual void OnStop(int waitQueueTimeS);
   
      ScReader(CaliceReceiver *r) : CaliceReader(r) {_runNo = -1;}
      ~ScReader(){}
      
    private:
      enum {e_sizeLdaHeader = 10}; // 8bytes + 0xcdcd
      int _runNo;
      unsigned int _cycleNo;
      
      bool _tempmode; // during the temperature readout time
      std::vector< std::pair<std::pair<int,int>, int> > _vecTemp; // (lda, port), data;
  };
  
}

#endif // CALICERECEIVER_HH

