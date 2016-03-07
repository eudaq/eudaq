#ifndef SIREADER_HH
#define SIREADER_HH

#include "CaliceProducer.hh"

namespace eudaq {

  class SiReader : public CaliceReader {
    public:
      virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent);
      virtual void OnStart(int runNo){_runNo = runNo; _acqStart = 0;}
   
      SiReader(CaliceProducer *r, int difIdx) : CaliceReader(r) {_difIdx = difIdx; _runNo = -1;}
      ~SiReader(){}
      
    private:
      enum {e_sizeSpillHeader = 12, e_sizeChipHeader = 10, e_sizeChipTrailer = 8, e_sizeSpillTrailer = 14};
      int _difIdx;
      int _runNo;
      int _acqStart;
  };
  
}

#endif // CALICEPRODUCER_HH

