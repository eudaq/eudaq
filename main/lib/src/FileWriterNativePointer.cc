#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/PointerEvent.hh"
#include "eudaq/EventSynchronisationBase.hh"

//#include "eudaq/Logger.hh"

namespace eudaq {

  class FileWriterNativePointer : public FileWriter {
    public:
      using counter_t = PointerEvent::counter_t;
      using reference_t = PointerEvent::reference_t;
      FileWriterNativePointer(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &); // used for detector events 
      virtual void WriteBaseEvent(const Event&);      // used as handle for base classes 
      virtual uint64_t FileBytes() const;
      virtual ~FileWriterNativePointer(){}
    private:


      std::unique_ptr<SyncBase> m_sync;
      std::unique_ptr<FileWriter> m_writer;
     
  };

  
  registerFileWriter(FileWriterNativePointer, "nativePointer");
  
  FileWriterNativePointer::FileWriterNativePointer(const std::string & param)  {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
    SyncBase::Parameter_t p("");
    m_sync = EventSyncFactory::create("Events2Pointer", p);
    m_writer = FileWriterFactory::Create("native", "raw3");

  }

  void FileWriterNativePointer::StartRun(unsigned runnumber) {
    m_writer->StartRun(runnumber);
  }

  void FileWriterNativePointer::WriteEvent(const DetectorEvent & ev) {
    
    event_sp det = DetectorEvent::ShallowCopy(ev);

    m_sync->pushEvent(det);
    event_sp dummy;
    while (m_sync->getNextEvent(dummy))
    {
      m_writer->WriteBaseEvent(*dummy);
    }
  }
  void FileWriterNativePointer::WriteBaseEvent(const Event& ev){
    if (ev.get_id()==DetectorEvent::eudaq_static_id())
    {
      auto det = dynamic_cast<const DetectorEvent*>(&ev);
      WriteEvent(*det);
      return;
    }

    m_writer->WriteBaseEvent(ev);
  }



  uint64_t FileWriterNativePointer::FileBytes() const { return m_writer->FileBytes(); }




}
