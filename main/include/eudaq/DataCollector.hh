#ifndef EUDAQ_INCLUDED_DataCollector
#define EUDAQ_INCLUDED_DataCollector

//#include <pthread.h>
#include <string>
#include <vector>
#include <list>

#include "eudaq/TransportServer.hh"
#include "eudaq/CommandReceiver.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/AidaFileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include <memory>
namespace eudaq {

class JSON;

  /** Implements the functionality of the File Writer application.
   *
   */
  class DLLEXPORT DataCollector : public CommandReceiver {
    public:
      DataCollector(const std::string & name, 
		    const std::string & runcontrol,
		    const std::string & listenaddress,
		    const std::string & runnumberfile = "../data/runnumber.dat");

      virtual void OnConnect(const ConnectionInfo & id);
      virtual void OnDisconnect(const ConnectionInfo & id);
      virtual void OnServer();
      virtual void OnGetRun();
      virtual void OnConfigure(const Configuration & param);
      virtual void OnPrepareRun(unsigned runnumber);
      virtual void OnStopRun();
      virtual void OnReceive(const ConnectionInfo & id, std::shared_ptr<Event> ev );
      virtual void OnReceive(const ConnectionInfo & id, std::shared_ptr<AidaPacket> packet );
      virtual void OnCompleteEvent();
      virtual void OnStatus();
      virtual ~DataCollector();

      void DataThread();
    protected:
      void WriteEvent( const DetectorEvent & ev );
      void WritePacket( std::shared_ptr<AidaPacket> packet );
      std::shared_ptr<JSON> buildJsonConfigHeader( unsigned int runnumber );


    private:
      struct Info {
       std::shared_ptr<ConnectionInfo> id;
       std::list<std::shared_ptr<Event> > events;
       std::list<std::shared_ptr<AidaPacket> > packets;
      };

      const std::string m_runnumberfile; // path to the file containing the run number
      const std::string m_name; // name provided in ctor

      void DataHandler(TransportEvent & ev);
      size_t GetInfo(const ConnectionInfo & id);

      bool m_done, m_listening;
      TransportServer * m_dataserver; ///< Transport for receiving data packets
//       pthread_t m_thread;
//       pthread_attr_t m_threadattr;
	  std::unique_ptr<std::thread> m_thread;
      std::vector<Info> m_buffer;
      size_t m_numwaiting; ///< The number of producers with events waiting in the buffer
      size_t m_itlu; ///< Index of TLU in m_buffer vector, or -1 if no TLU
      unsigned m_runnumber, m_eventnumber, m_packetNumberLastPacket;
      std::unique_ptr<AidaFileWriter> m_writer;
      Configuration m_config;
      Time m_runstart;
  };

}

#endif // EUDAQ_INCLUDED_DataCollector
