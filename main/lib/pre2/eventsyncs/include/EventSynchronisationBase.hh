#ifndef EventSynchronisationBase_h__
#define EventSynchronisationBase_h__


#include <memory>
#include <string>
#include "ClassFactory.hh"
#include "Platform.hh"
#include "Configuration.hh"
#include "Event.hh"

// base class for all Synchronization Plugins
// it is desired to be as modular es possible with this approach.
// first step is to separate the events from different Producers. 
// then it will be recombined to a new event
// The Idea is that the user can define a condition that need to be true to define if an event is sync or not


#define registerSyncClass(DerivedFileWriter,ID)  REGISTER_DERIVED_CLASS(SyncBase,DerivedFileWriter,ID)

namespace eudaq{

  class OptionParser;
  class Event;
  class DLLEXPORT SyncBase {
  public:
    using MainType = std::string;
    using Parameter_t = eudaq::Configuration;
    using Parameter_ref = const Parameter_t&;


    // public interface

    SyncBase(Parameter_ref param);
    virtual bool pushEvent(event_sp ev,size_t Index=0) = 0;
    virtual bool getNextEvent(event_sp& ev) = 0;

    virtual bool OutputIsEmpty() const = 0;
    virtual bool InputIsEmpty() const = 0;
    virtual bool InputIsEmpty(size_t FileID) const = 0;

    Parameter_t getParameter();
  private:
    Parameter_t m_param;

  };


  using Sync_up = std::unique_ptr<SyncBase, std::function<void(eudaq::SyncBase*)>>;

  class DLLEXPORT EventSyncFactory{
  public:

   static Sync_up create(SyncBase::MainType name, SyncBase::Parameter_ref sync);
   static Sync_up create(SyncBase::MainType name );
   static Sync_up create();
   static std::vector<std::string> GetTypes();
   static std::string  Help_text();
   static void addComandLineOptions(eudaq::OptionParser & op);
   static bool DefaultIsSet();
   static std::string getDefaultSync();
   
  private:
    class Impl;
    static Impl& getImpl();
    
  };
}//namespace eudaq



#endif // EventSynchronisationBase_h__
