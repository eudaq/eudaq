#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

//#include <pthread.h>
#include <string>
#include <vector>
#include <list>

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include <memory>
namespace eudaq {
  class FileWriter;

  class SyncBase;
  class Processor_batch;
  class ProcessorBase;
  class ProcessorFileWriter;
  /** Implements the functionality of the File Writer application.
   *
   */
  class DLLEXPORT DataCollector : public CommandReceiver {
    public:
      DataCollector(const std::string & name, 
		    const std::string & runcontrol,
		    const std::string & listenaddress,
		    const std::string & runnumberfile = "../data/runnumber.dat");


      virtual void OnServer() override;
      virtual void OnGetRun() override;

      virtual void OnConfigure(const Configuration & param) override;
      virtual void OnPrepareRun(unsigned runnumber) override;
      virtual void OnStopRun() override;


      virtual void OnStatus() override;
      virtual ~DataCollector();

 
    protected:
      
      


    private:
      
      const std::string m_runnumberfile; // path to the file containing the run number
      const std::string m_name; // name provided in ctor


      unsigned m_runnumber, m_eventnumber;




      std::unique_ptr<Processor_batch> m_batch;
      std::unique_ptr<ProcessorBase> m_dataReciever;
      std::unique_ptr<ProcessorFileWriter> m_pwriter;
      std::string m_ConnectionName;
  };

}

#endif // EUDAQ_INCLUDED_DataCollector
