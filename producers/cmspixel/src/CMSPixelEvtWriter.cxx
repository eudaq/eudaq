// pxarCore includes:
#include "log.h"

// producer includes:
#include "CMSPixelEvtMonitor.hh"
#include "CMSPixelProducer.hh"

// system includes:
#include <iostream>
#include <ostream>
#include <vector>

using namespace pxar; 
using namespace std; 

void CMSPixelProducer::ReadInSingleEventWriteBinary() {
  if(int(m_perFull) > 80){
    std::cout << "Warning: Buffer almost full. Please stop the run!" << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "BUFFER almost full. Stop run!");
    // OnStopRun(); // FIXME asking eudaq to stop the run doesn't to work (yet)
  }
  double_t t_0 = m_t -> uSeconds();

  try {
    pxar::rawEvent daqEvent = m_api -> daqGetRawEvent(); 
    //CMSPixelEvtMonitor::Instance() -> TrackROTiming(++m_ev, m_t -> uSeconds() - t_0); 

    m_fout.write(reinterpret_cast<const char*>(&daqEvent.data[0]), sizeof(daqEvent.data[0])*daqEvent.data.size());
    m_ev++;
    if(m_ev%10 == 0) std::cout << "CMSPIX EVT " << m_ev << std::endl;
  }
  catch(...) {}

  if(stopping){
    // assure trigger has stopped before attempting readout.
    while(triggering)
      eudaq::mSleep(20);
    std::cout << "Reading remaining data from buffer." << std::endl;
    std::vector<pxar::rawEvent> daqdat = m_api -> daqGetRawEventBuffer();
    for(std::vector<pxar::rawEvent>::iterator it = daqdat.begin(); it != daqdat.end(); ++it){
      m_fout.write(reinterpret_cast<const char*>(&(it->data[0])), sizeof(it->data[0])*(it->data.size()));
    }
    eudaq::mSleep(10);         
    stopping = false;
    started = false;
  }
}

void CMSPixelProducer::ReadInSingleEventWriteASCII(){
  if(int(m_perFull) > 80){
    std::cout << "Warning: Buffer almost full. Please stop the run!" << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "BUFFER almost full. Stop run!");
    // OnStopRun(); 
  }
  double_t t_0 = m_t -> uSeconds();
  int col, row;
  double value;

  try {
    pxar::Event daqEvent = m_api -> daqGetEvent();
    CMSPixelEvtMonitor::Instance() -> TrackROTiming(++m_ev, m_t -> uSeconds() - t_0); 
      
    LOG(logDEBUGAPI) << "Number of decoder errors: "<< m_api -> daqGetNDecoderErrors();
    m_fout << std::hex << daqEvent.header << "\t";

    for(std::vector<pxar::pixel>::iterator it = daqEvent.pixels.begin(); it != daqEvent.pixels.end(); ++it){
      col = (int)it->column();
      row = (int)it->row();
      value = it->value();
      m_fout << std::dec << col << "\t" << row << "\t" << value << "\t";
    }
    m_fout << "\n";
  }
  catch(...) {}

  // if OnStopRun has been called read out remaining buffer
  if(stopping){
    while(triggering)
      eudaq::mSleep(20);
    std::vector<pxar::Event> daqdat = m_api->daqGetEventBuffer();
    for(std::vector<pxar::Event>::iterator daqEventIt = daqdat.begin(); daqEventIt != daqdat.end(); ++daqEventIt){
      m_fout << hex << daqEventIt -> header << "\t";
      for(std::vector<pxar::pixel>::iterator it = daqEventIt->pixels.begin(); it != daqEventIt->pixels.end(); ++it){
	col = (int)it->column();
	row = (int)it->row();
	value = it->value();
	m_fout << std::dec << col << "\t" << row << "\t" << value << "\t";
      } 
      m_fout << std::endl;
    }
    std::cout << "Read " << daqdat.size() << " remaining events." << std::endl;
    stopping = false;
    started = false;
  }
}

void CMSPixelProducer::ReadInFullBufferWriteBinary()
{
  if(int(m_perFull) > 80 || stopping){
    if(stopping)
      while(triggering)
	eudaq::mSleep(20);
    std::cout << "DAQ buffer is " << int(m_perFull) << "\% full." << std::endl;     
    std::cout << "Start reading data from DTB RAM." << std::endl;
    std::cout << "Halting trigger loop." << std::endl;
    m_api -> daqTriggerLoopHalt();
    std::vector<uint16_t> daqdat = m_api->daqGetBuffer();
    std::cout << "Read " << daqdat.size() << " words of data: ";
    if(daqdat.size() > 550000) std::cout << (daqdat.size()/524288) << "MB." << std::endl;
    else std::cout << (daqdat.size()/512) << "kB." << std::endl;
    m_fout.write(reinterpret_cast<const char*>(&daqdat[0]), sizeof(daqdat[0])*daqdat.size());
    if(stopping){
      stopping = false;
      started = false;
    }
    else{
      std::cout << "Continuing trigger loop.\n";
      m_api -> daqTriggerLoop(m_pattern_delay);
    }
    m_api -> daqStatus(m_perFull);    
  }
  eudaq::mSleep(1000);
}// ReadInFullBufferWriteBinary
//-------------------------------------------------
//
//-------------------------------------------------
void CMSPixelProducer::ReadInFullBufferWriteASCII()
{
  if(int(m_perFull) > 10 || stopping){
    if(stopping)
      while(triggering)
	eudaq::mSleep(20);
    std::cout << "DAQ buffer is " << int(m_perFull) << "\% full." << std::endl;     
    std::cout << "Start reading data from DTB RAM." << std::endl;
    std::cout << "Halting trigger loop." << std::endl;
    m_api -> daqTriggerLoopHalt();
    std::vector<pxar::Event> daqdat = m_api->daqGetEventBuffer();
    std::cout << "Read " << daqdat.size() << " events." << std::endl;
    for(std::vector<pxar::Event>::iterator daqEventIt = daqdat.begin(); daqEventIt != daqdat.end(); ++daqEventIt){
      m_fout << hex << daqEventIt -> header << "\t";
      int col, row;
      double value;
      for(std::vector<pxar::pixel>::iterator it = daqEventIt->pixels.begin(); it != daqEventIt->pixels.end(); ++it){
	col = (int)it->column();
	row = (int)it->row();
	value = it->value();
	m_fout << std::dec << col << "\t" << row << "\t" << value << "\t";
      } 
      m_fout << "\n";
    }
    if(stopping){
      stopping = false;
      started = false;
    }
    else{
      std::cout << "Continuing trigger loop.\n";
      m_api -> daqTriggerLoop(m_pattern_delay);
    }
    m_api -> daqStatus(m_perFull);    
  }
  eudaq::mSleep(1000);
}
