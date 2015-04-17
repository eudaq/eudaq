#include <list>
#include "eudaq/baseFileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/factory.hh"

namespace eudaq {

  class DLLEXPORT FileReaderTLU_NI : public baseFileReader {
  public:
    FileReaderTLU_NI(const std::string & filename, const std::string & filepattern = "");

    FileReaderTLU_NI(Parameter_ref param);


    ~FileReaderTLU_NI();
    virtual  unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual std::shared_ptr<eudaq::Event> getEventPtr() { return m_ev; }
    virtual std::shared_ptr<eudaq::Event> GetNextEvent();
    virtual void Interrupt() { m_des.Interrupt(); }



    const eudaq::Event & GetEvent() const;
    const DetectorEvent & Event() const { return GetDetectorEvent(); } // for backward compatibility
    const DetectorEvent & GetDetectorEvent() const;
    const StandardEvent & GetStandardEvent() const;
    std::shared_ptr<eudaq::DetectorEvent> GetDetectorEvent_ptr(){ return std::dynamic_pointer_cast<eudaq::DetectorEvent>(m_ev); };

  private:

    FileDeserializer m_des;
    std::shared_ptr<eudaq::Event> m_ev;
    unsigned m_ver;
    size_t m_subevent_counter = 0;
    size_t m_tlu = (size_t) -1, m_ni = (size_t) -1;
  };




  FileReaderTLU_NI::FileReaderTLU_NI(const std::string & file, const std::string & filepattern)
    : baseFileReader(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
    m_des(Filename()),
    m_ev(EventFactory::Create(m_des)),
    m_ver(1)
  {
 
    DetectorEvent* det = dynamic_cast<DetectorEvent*> (m_ev.get());
    for (size_t i = 0; i < det->NumEvents(); ++i){
      if (PluginManager::isTLU(*det->GetEvent(i))){
        m_tlu = i;

      }
      if (det->GetEvent(i)->GetSubType().compare("NI") == 0){
        m_ni = i;
      }
    }
    det->GetEventPtr(m_ni)->setTimeStamp(det->GetEventPtr(m_tlu)->GetTimestamp());

     m_ev = det->GetEventPtr(m_ni);
  }

  FileReaderTLU_NI::FileReaderTLU_NI(Parameter_ref param) :FileReaderTLU_NI(param.Get(getKeyFileName(),""), param.Get(getKeyInputPattern(),""))
  {

  }

  FileReaderTLU_NI::~FileReaderTLU_NI() {

  }

  bool FileReaderTLU_NI::NextEvent(size_t skip) {
    std::shared_ptr<eudaq::Event> ev = nullptr;


    bool result = m_des.ReadEvent(m_ver, ev, skip);



    if (ev){
      DetectorEvent* det = dynamic_cast<DetectorEvent*> (ev.get());
      auto buf = det->GetEventPtr(m_ni)->GetTimestamp();
      det->GetEventPtr(m_ni)->setTimeStamp(det->GetEventPtr(m_tlu)->GetTimestamp());
      det->GetEventPtr(m_ni)->pushTimeStamp(buf);
      m_ev = det->GetEventPtr(m_ni);
    }
#ifdef _DEBUG
    else{
      std::cout << "end of file" << std::endl;
    }
#endif //_DEBUG
    return result;
  }

  unsigned FileReaderTLU_NI::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const Event & FileReaderTLU_NI::GetEvent() const {
    return *m_ev;
  }

  const DetectorEvent & FileReaderTLU_NI::GetDetectorEvent() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

  const StandardEvent & FileReaderTLU_NI::GetStandardEvent() const {
    return dynamic_cast<const StandardEvent &>(*m_ev);
  }

  std::shared_ptr<eudaq::Event> FileReaderTLU_NI::GetNextEvent(){

    if (!NextEvent()) {
      return nullptr;
    }

    return m_ev;


  }



//   std::shared_ptr<eudaq::Event> FileReader::GetNextROF()
//   {
// 
// 
//     if (GetEvent().IsPacket())
//     {
//       if (m_subevent_counter < PluginManager::GetNumberOfROF(GetEvent()))
//       {
//         return PluginManager::ExtractEventN(getEventPtr(), m_subevent_counter++);
//       }
//       else
//       {
//         m_subevent_counter = 0;
//         if (NextEvent())
//         {
//           return GetNextROF();
//         }
//         else{
//           return nullptr;
//         }
// 
//       }
// 
//     }
//     else
//     {
// 
//       return getEventPtr();
//     }
//     return nullptr;
//   }

  RegisterFileReader(FileReaderTLU_NI, "tlu_ni");

}
