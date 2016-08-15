#include <iostream>
#include <memory>

#include "EventSynchronisationBase.hh"

#include "Event.hh"
#include "PluginManager.hh"
#include "Configuration.hh"
#include "OptionParser.hh"

#define FILEINDEX_OFFSET 10000
using std::cout;
using std::endl;
using std::shared_ptr;
using namespace std;


namespace eudaq{
  using EventSyncClassFactory = ClassFactory<SyncBase, SyncBase::MainType, SyncBase::Parameter_t>;
  
  SyncBase::Parameter_t SyncBase::getParameter()
  {
    return m_param;
  }

  SyncBase::SyncBase(Parameter_ref param) :m_param(param)
  {

  }

  REGISTER_BASE_CLASS(SyncBase);

  class EventSyncFactory::Impl{
  public:
    std::unique_ptr<eudaq::Option<std::string>> m_default_sync;
    

  };


  Sync_up EventSyncFactory::create(SyncBase::MainType name, SyncBase::Parameter_ref sync)
  {
    if (name.empty())
    {
      return EventSyncClassFactory::Create(getDefaultSync(), sync);
    }
    return EventSyncClassFactory::Create(name, sync);
  }

  Sync_up EventSyncFactory::create()
  {
      
    return create(getDefaultSync(), SyncBase::Parameter_t(""));
  }

  Sync_up EventSyncFactory::create(SyncBase::MainType name)
  {
    return create(name, SyncBase::Parameter_t(""));
  }

  std::vector<std::string> EventSyncFactory::GetTypes()
  {
    return EventSyncClassFactory::GetTypes();
  }

  std::string EventSyncFactory::Help_text()
  {
    return std::string("Available output types are: " + to_string(eudaq::EventSyncFactory::GetTypes(), ", "));
  }

  std::string EventSyncFactory::getDefaultSync()
  {
    if (getImpl().m_default_sync && getImpl().m_default_sync->IsSet())
    {
      return getImpl().m_default_sync->Value();
    }
    return "DetectorEvents";  
  }

  void EventSyncFactory::addComandLineOptions(eudaq::OptionParser & op)
  {
    getImpl().m_default_sync = std::unique_ptr<eudaq::Option<std::string>>(new eudaq::Option<std::string>(op, "s", "sync", getDefaultSync(), "determines which sync algorithm is used"));
    
  }

  EventSyncFactory::Impl& EventSyncFactory::getImpl()
  {
    static EventSyncFactory::Impl m_impl;
    return m_impl;

  }

  bool EventSyncFactory::DefaultIsSet()
  {
    return getImpl().m_default_sync->IsSet();
  }


}
