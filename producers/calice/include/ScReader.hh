#ifndef SCREADER_HH
#define SCREADER_HH

#include "CaliceProducer.hh"
#include <deque>

namespace eudaq {
  
  class ScReader : public CaliceReader {
  public:
    virtual void Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent);
    virtual void OnStart(int runNo);
    virtual void OnStop(int waitQueueTimeS);

    virtual std::deque<eudaq::RawDataEvent *> NewEvent_createRawDataEvent(std::deque<eudaq::RawDataEvent *>  deqEvent, bool tempcome, int cycle);
    virtual void readTemperature(std::deque<char> buf);
    virtual void AppendBlockTemperature(std::deque<eudaq::RawDataEvent *> deqEvent);

    virtual bool readSpirocData_AddBlock(std::deque<char> buf, std::deque<eudaq::RawDataEvent *> deqEvent);
    
    ScReader(CaliceProducer *r) : CaliceReader(r) {_runNo = -1;}
    ~ScReader(){}
    
  private:
    enum {e_sizeLdaHeader = 10}; // 8bytes + 0xcdcd
    int _runNo;
    unsigned int _cycleNo;
    unsigned int length;

    bool _tempmode; // during the temperature readout time
    std::vector< std::pair<std::pair<int,int>, int> > _vecTemp; // (lda, port), data;
   
  };
}

#endif // CALICEPRODUCER_HH

