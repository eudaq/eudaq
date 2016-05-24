#ifndef EventSynchronisationBase_h__
#define EventSynchronisationBase_h__


#include <memory>
#include <string>
#include "eudaq/factory.hh"
#include "Platform.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Event.hh"

// base class for all Synchronization Plugins
// it is desired to be as modular es possible with this approach.
// first step is to separate the events from different Producers. 
// then it will be recombined to a new event
// The Idea is that the user can define a condition that need to be true to define if an event is sync or not


#define registerSyncClass(DerivedFileWriter,ID)  registerClass(SyncBase,DerivedFileWriter,ID)

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




  class DLLEXPORT EventSyncFactory{
  public:

   static std::unique_ptr<SyncBase>  create(SyncBase::MainType name, SyncBase::Parameter_ref sync);
   static std::unique_ptr<SyncBase>  create(SyncBase::MainType name );
   static std::unique_ptr<SyncBase> create();
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
