#ifndef EUDAQ_INCLUDED_Monitor
#define EUDAQ_INCLUDED_Monitor

#include "eudaq/CommandReceiver.hh"
#include "eudaq/DataReceiver.hh"
#include "eudaq/Event.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Factory.hh"

#include <string>
#include <vector>
#include <list>
#include <memory>
#include <atomic>

namespace eudaq {
  class Monitor;
  
#ifndef EUDAQ_CORE_EXPORTS
  extern template class DLLEXPORT Factory<Monitor>;
  extern template DLLEXPORT std::map<uint32_t, typename Factory<Monitor>::UP_BASE (*)
				     (const std::string&, const std::string&)>&
  Factory<Monitor>::Instance<const std::string&, const std::string&>();
#endif

  using MonitorSP = Factory<Monitor>::SP_BASE;
  
  //----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
  class DLLEXPORT Monitor : public CommandReceiver, public DataReceiver{
  public:
    Monitor(const std::string &name, const std::string &runcontrol);
    virtual void DoInitialise();
    virtual void DoConfigure();
    virtual void DoStartRun();
    virtual void DoStopRun();
    virtual void DoReset();
    virtual void DoTerminate();
    virtual void DoStatus();
    virtual void DoReceive(EventSP ev);
    void SetServerAddress(const std::string &addr);
    static MonitorSP Make(const std::string &code_name,
			  const std::string &run_name,
			  const std::string &runcontrol);
  private:
    void OnInitialise() override final;
    void OnConfigure() override final;
    void OnStartRun() override final;
    void OnStopRun() override final;
    void OnReset() override final;
    void OnTerminate() override final;
    void OnStatus() override final;
    void OnReceive(ConnectionSPC id, EventSP ev) override final;
  private:
    std::string m_data_addr;
    uint32_t m_evt_c;
  };
  //----------DOC-MARK-----END*DEC-----DOC-MARK----------
}

#endif // EUDAQ_INCLUDED_Monitor
