

#include <iostream>

#include <memory>
#include "eudaq/Event.hh"
#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/factoryDef.hh"
#include "eudaq/OptionParser.hh"


#define FILEINDEX_OFFSET 10000
using std::cout;
using std::endl;
using std::shared_ptr;
using namespace std;

template class Class_factory_Utilities::Factory<eudaq::SyncBase>;

namespace eudaq{
 


  SyncBase::Parameter_t SyncBase::getParameter()
  {
    return m_param;
  }

  SyncBase::SyncBase(Parameter_ref param) :m_param(param)
  {

  }

  registerBaseClassDef(SyncBase);

  class EventSyncFactory::Impl{
  public:
    std::unique_ptr<eudaq::Option<std::string>> m_default_sync;
    

  };


  std::unique_ptr<SyncBase> EventSyncFactory::create(SyncBase::MainType name, SyncBase::Parameter_ref sync)
  {
    if (name.empty())
    {
      return Class_factory_Utilities::Factory<SyncBase>::Create(getDefaultSync(), sync);
    }
    return Class_factory_Utilities::Factory<SyncBase>::Create(name, sync);
  }

  std::unique_ptr<SyncBase> EventSyncFactory::create()
  {
      
    return create(getDefaultSync(), SyncBase::Parameter_t(""));
  }

  std::unique_ptr<SyncBase> EventSyncFactory::create(SyncBase::MainType name)
  {
    return create(name, SyncBase::Parameter_t(""));
  }

  std::vector<std::string> EventSyncFactory::GetTypes()
  {
    return Class_factory_Utilities::Factory<SyncBase>::GetTypes();
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
