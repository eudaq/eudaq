#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/PointerEvent.hh"
#include <map>

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
      virtual ~FileWriterNativePointer();
    private:

      reference_t Write_Event_Body(const Event&);  
      reference_t Write_Event_Pointer(const PointerEvent&);
      reference_t findReference(const event_sp ev);
      void removeReference(const event_sp ev);

      FileSerializer * m_ser;
      std::map < event_sp, reference_t> m_buffer;
      reference_t m_ref =0;
      const counter_t m_internalRef = 3;
  };

  
  registerFileWriter(FileWriterNativePointer, "nativePointer");
  
  FileWriterNativePointer::FileWriterNativePointer(const std::string & /*param*/) : m_ser(0) {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterNativePointer::StartRun(unsigned runnumber) {
    delete m_ser;
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw3").Set('R', runnumber),true);
  }

  void FileWriterNativePointer::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
    }
    

    for (size_t i = 0; i < ev.NumEvents(); ++i)
    {
      auto subPointer = ev.GetEventPtr(i);
      reference_t ref;
     
      ref = findReference(subPointer);
      if (ref==0)
      {
        ref= Write_Event_Body(*subPointer);
        m_buffer[subPointer] = ref;
      }
      
      counter_t counter = subPointer.use_count();
      Event* subRawPointer = subPointer.get();
      subRawPointer->~Event();
      
      new(subRawPointer) PointerEvent(ref, counter - m_internalRef);
      if (counter-m_internalRef == 0)
      {
        removeReference(subPointer);
      }

    }

    auto ref = Write_Event_Body(ev);
    PointerEvent pointerEvent(ref, 0);
    Write_Event_Pointer(pointerEvent);
  }
  void FileWriterNativePointer::WriteBaseEvent(const Event& ev){
   
    auto det = dynamic_cast<const DetectorEvent*>(&ev);
    if (det)
    {
      WriteEvent(*det);
      return;
    }



    auto file_ref=Write_Event_Body(ev);

    PointerEvent p(file_ref, 0);
    Write_Event_Pointer(p);
  }

  FileWriterNativePointer::~FileWriterNativePointer() {
    delete m_ser;
  }

  uint64_t FileWriterNativePointer::FileBytes() const { return m_ser ? m_ser->FileBytes() : 0; }

  FileWriterNativePointer::reference_t FileWriterNativePointer::Write_Event_Body(const Event& ev)
  {
    ++m_ref;
    
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
    return m_ref;
  }

  FileWriterNativePointer::reference_t FileWriterNativePointer::Write_Event_Pointer(const PointerEvent& ev)
  {
    if (!m_ser) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    m_ser->write(ev);
    m_ser->Flush();
  }

  FileWriterNativePointer::reference_t FileWriterNativePointer::findReference(const event_sp ev)
  {
    auto index = m_buffer.find(ev);
    if (index!=m_buffer.end())
    {
      return index->second;
    }
    return 0;
  }

  void FileWriterNativePointer::removeReference(const event_sp ev)
  {
    m_buffer.erase(ev);
  }

}
