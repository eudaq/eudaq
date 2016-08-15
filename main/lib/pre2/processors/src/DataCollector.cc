#include <iostream>
#include <ostream>
#include <thread>
#include <memory>
#include "eudaq/DataCollector.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/PluginManager.hh"

#include "eudaq/FileWriter.hh"
#include "EventSynchronisationBase.hh"

#include "config.h"
#include "Processors.hh"
#include "ProcessorFileWriter.hh"
#include "Processor_batch.hh"


using eudaq_types::outPutString;
namespace eudaq {

TYPE_CLASS(outPut_EventNr, unsigned&);

class P_addMeta_data_to_Bore :public ProcessorBase {
public:
  P_addMeta_data_to_Bore(const Configuration & param, outPut_EventNr outEventNumber) :m_config(param), EventNumber(outEventNumber) {

  }
  void init() override {
    m_runstart = Time::Current();
  }
  virtual ReturnParam ProcessEvent(event_sp ev, ConnectionName_ref con) override {

    if (ev->IsBORE()) {
      ev->SetTag("STARTTIME", m_runstart.Formatted());
      ev->SetTag("CONFIG", to_string(m_config));
    }else if (ev->IsEORE())
    {
      if (ev->IsEORE()) {
        ev->SetTag("STOPTIME", Time::Current().Formatted());
        EUDAQ_INFO("Run " + to_string(ev->GetRunNumber()) + ", EORE = " + to_string(ev->GetEventNumber()));
      }
    }
    necessary_CONVERSION(EventNumber)= ev->GetEventNumber();
    return processNext(std::move(ev), con);
  }

  void end() override {

  }
private:
  Time m_runstart = Time(0);
  const Configuration  m_config;
  outPut_EventNr EventNumber;
};






DataCollector::DataCollector(const std::string & name, const std::string & runcontrol, const std::string & listenaddress, const std::string & runnumberfile) :
CommandReceiver("DataCollector", name, runcontrol, false),
m_runnumberfile(runnumberfile), m_name(name), 
m_runnumber(ReadFromFile(runnumberfile, 0U)), 
m_eventnumber(0)

{
  m_dataReciever = Processors::dataReciver( listenaddress, outPutString(m_ConnectionName));
  EUDAQ_DEBUG("Instantiated datacollector with name: " + name);
  EUDAQ_DEBUG("Listen address=" + m_ConnectionName);
  CommandReceiver::StartThread();
}

DataCollector::~DataCollector() {

}

void DataCollector::OnServer() {
  m_status.SetTag("_SERVER", m_ConnectionName);
}

void DataCollector::OnGetRun() {
  //std::cout << "Sending run number: " << m_runnumber << std::endl;
  m_status.SetTag("_RUN", to_string(m_runnumber));
}





void DataCollector::OnConfigure(const Configuration & param) {
  if (m_batch) {
    m_batch->reset();
  }
  m_pwriter =  std::unique_ptr<ProcessorFileWriter>(new ProcessorFileWriter(param.Get("FileType","")));
  m_pwriter->SetFilePattern(param.Get("FilePattern", ""));
  m_batch = make_batch();
  *m_batch >> m_dataReciever.get()
    >> Processors::merger(param.Get("SyncAlgorithm", "DetectorEvents"))
    >> Processors::ShowEventNR(1000)
    >> Processors::waitForEORE(param.Get("EndOfRunTimeOut", 1000000))
    >> make_Processor_up<P_addMeta_data_to_Bore>(param,outPut_EventNr( m_eventnumber))
    >> m_pwriter.get();

  
  

}

void DataCollector::OnPrepareRun(unsigned runnumber) {

  EUDAQ_INFO("Preparing for run " + to_string(runnumber));

  try {
    if (!m_pwriter) {
      EUDAQ_THROW("You must configure before starting a run");
    }


    m_batch->init();


    WriteToFile(m_runnumberfile, runnumber);
    m_runnumber = runnumber;
    m_eventnumber = 0;

    SetStatus(Status::LVL_OK, "Running");
  } catch (const Exception & e) {
    std::string msg = "Error preparing for run " + to_string(runnumber) + ": " + e.what();
    EUDAQ_ERROR(msg);
    SetStatus(Status::LVL_ERROR, msg);
  }
}


void DataCollector::OnStopRun() {
  EUDAQ_INFO("End of run " + to_string(m_runnumber));
  m_batch->wait();
  m_batch->end();

  std::cout << "stopped " << std::endl;
  SetStatus(Status::LVL_OK, "Stopped");
}







void DataCollector::OnStatus() {
  std::string evt;
  if (m_eventnumber > 0)
    evt = to_string(m_eventnumber - 1);
  m_status.SetTag("EVENT", evt);
  m_status.SetTag("RUN", to_string(m_runnumber));
  if (m_pwriter) {
    m_status.SetTag("FILEBYTES", to_string(m_pwriter->FileBytes()));
  }
}












}
