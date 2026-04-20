#pragma once

#include <eudaq/DataCollector.hh>
#include <eudaq/Factory.hh>
#include <eudaq/Logger.hh>
#include <eudaq/Event.hh>
#include <eudaq/Configuration.hh>

#include <cstdint>
#include <string>
#include <memory>

class HidraDataCollector : public eudaq::DataCollector {
public:
  HidraDataCollector(const std::string &name, const std::string &runcontrol)
      : eudaq::DataCollector(name, runcontrol) {}

  ~HidraDataCollector() override = default;

  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraDataCollector");

private:
  uint64_t m_event_count;
  uint64_t m_max_events;
  bool m_stop_sent = false;

  void DoInitialise() override {
    auto ini = GetInitConfiguration();
    if (!ini) {
      EUDAQ_WARN("HidraDataCollector: missing init configuration");
    }
    EUDAQ_INFO("HidraDataCollector initialized");
    m_event_count = 0;
  }

  void DoConfigure() override {
    auto conf = GetConfiguration();
    if (!conf) {
      EUDAQ_WARN("HidraDataCollector: missing run configuration");
    }
    EUDAQ_INFO("HidraDataCollector configured");
    m_max_events = conf->Get("EX0_MAX_EVENTS", 0);
    if(m_max_events == 0) {
	    EUDAQ_WARN("In hidra.config file: missing max event number initializzation");
    }	    
  }

  void DoStartRun() override {
    m_event_count = 0;
    EUDAQ_INFO("HidraDataCollector start run " + std::to_string(GetRunNumber()));
  }

  void DoStopRun() override {
    EUDAQ_INFO("HidraDataCollector stop run " + std::to_string(GetRunNumber()));
  }

  void DoReset() override {
    EUDAQ_INFO("HidraDataCollector reset");
  }

  void DoTerminate() override {
    EUDAQ_INFO("HidraDataCollector terminate");
  }

  void DoStatus() override {
    SetStatusTag("Status", "OK");
    SetStatusMsg("HidraDataCollector running");
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
      EUDAQ_WARN("HidraDataCollector received null event");
      return;
    }

    const auto desc = ev->GetDescription();

    if (ev->IsBORE()) {
      EUDAQ_INFO("Received BORE from " + id->GetName() + " type=" + desc);
    } else if (ev->IsEORE()) {
      EUDAQ_INFO("Received EORE from " + id->GetName() + " type=" + desc);
    } else {
	    if(!m_stop_sent && m_event_count < m_max_events){

      
		    EUDAQ_DEBUG("Received event " + std::to_string(ev->GetEventN()) +
                  " from " + id->GetName() +
                  " type=" + desc);
		    ++m_event_count;
		    std::cout << "Event number: " << m_event_count << std::endl;
		    WriteEvent(std::move(ev));
	    }

	    else if (!m_stop_sent && m_event_count >= m_max_events) {
	  
		    m_stop_sent = true;

		    EUDAQ_INFO("Max events reached. Sending STOP request");
		    SetStatus(eudaq::Status::STATE_RUNNING, "STOP_REQUEST");
		    SendStatus();

	    }
    }

  } 
};

namespace {
  auto dummy0 =
    eudaq::Factory<eudaq::DataCollector>::
	Register<HidraDataCollector, const std::string&, const std::string&>
	(HidraDataCollector::m_id_factory);

}
