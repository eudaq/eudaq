#pragma once

#include <eudaq/DataCollector.hh>
#include <eudaq/Factory.hh>
#include <eudaq/Logger.hh>
#include <eudaq/Event.hh>
#include <eudaq/Configuration.hh>

#include <cstdint>
#include <string>
#include <memory>

class QTPDPaviaDataCollector : public eudaq::DataCollector {
public:
  QTPDPaviaDataCollector(const std::string &name, const std::string &runcontrol)
      : eudaq::DataCollector(name, runcontrol) {}

  ~QTPDPaviaDataCollector() override = default;

  static const uint32_t m_id_factory = eudaq::cstr2hash("QTPDPaviaDataCollector");


  void DoInitialise() override {
    auto ini = GetInitConfiguration();
    if (!ini) {
      EUDAQ_WARN("QTPDPaviaDataCollector: missing init configuration");
    }
    EUDAQ_INFO("QTPDPaviaDataCollector initialized");
  }

  void DoConfigure() override {
    auto conf = GetConfiguration();
    if (!conf) {
      EUDAQ_WARN("QTPDPaviaDataCollector: missing run configuration");
    }
    EUDAQ_INFO("QTPDPaviaDataCollector configured");
    //m_max_events = conf->Get("EX0_MAX_EVENTS", 0);
  }

  void DoStartRun() override {
    EUDAQ_INFO("QTPDPaviaDataCollector start run " + std::to_string(GetRunNumber()));
  }

  void DoStopRun() override {
    EUDAQ_INFO("QTPDPaviaDataCollector stop run " + std::to_string(GetRunNumber()));
  }

  void DoReset() override {
    EUDAQ_INFO("QTPDPaviaDataCollector reset");
  }

  void DoTerminate() override {
    EUDAQ_INFO("QTPDPaviaDataCollector terminate");
  }

  void DoStatus() override {
    SetStatusTag("Status", "OK");
    SetStatusMsg("QTPDPaviaDataCollector running");
  }

  void DoConnect(eudaq::ConnectionSPC id) override {
    EUDAQ_INFO("Connected: " + id->GetType() + " / " + id->GetName());
    eudaq::DataCollector::DoConnect(id);
  }

  void DoDisconnect(eudaq::ConnectionSPC id) override {
    EUDAQ_INFO("Disconnected: " + id->GetType() + " / " + id->GetName());
    eudaq::DataCollector::DoDisconnect(id);
  }

  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override {
    if (!ev) {
      EUDAQ_WARN("QTPDPaviaDataCollector received null event");
      return;
    }

    const auto desc = ev->GetDescription();

    if (ev->IsBORE()) {
      EUDAQ_INFO("Received BORE from " + id->GetName() + " type=" + desc);
    } else if (ev->IsEORE()) {
      EUDAQ_INFO("Received EORE from " + id->GetName() + " type=" + desc);
    } else {
      EUDAQ_DEBUG("Received event " + std::to_string(ev->GetEventN()) +
                  " from " + id->GetName() +
                  " type=" + desc);
    }
   
    //To increment events and stop when maximum is reached 
    /*m_evt_c++;

    if (m_max_events > 0 && m_evt_c >= m_max_events) {
    
	    EUDAQ_INFO("Max events reached -> stopping run");

            SendCommand(eudaq::Command::StopRun());
    }*/

    // Optional filter:
    // if (desc != "CAENQTPRaw") return;

    // Forward to the standard EUDAQ DataCollector writing path.
    WriteEvent(std::move(ev));
  }

  //uint64_t m_max_events;

};
namespace {
  auto dummy0 =
    eudaq::Factory<eudaq::DataCollector>::
	Register<QTPDPaviaDataCollector, const std::string&, const std::string&>
	(QTPDPaviaDataCollector::m_id_factory);

}
