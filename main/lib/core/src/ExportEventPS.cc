#include "eudaq/ExportEventPS.hh"


namespace eudaq{

  namespace{
    auto dummy0 = Factory<Processor>::
      Register<ExportEventPS>(cstr2hash(ExportEventPS::m_description));
  }


  ExportEventPS::ExportEventPS(): Processor(ExportEventPS::m_description){
  }

  void ExportEventPS::ProcessCommand(const std::string& key, const std::string& val){
    switch(str2hash(key)){
    case cstr2hash("EVETN:CLEAR"):{
      std::unique_lock<std::mutex> lk(m_mtx_que);
      m_que_ev.clear();
      break;
    }
    default:
      std::cout<<"unkonw user cmd"<<std::endl;
    }
  }


  void ExportEventPS::ProcessEvent(EventSPC ev){
    std::unique_lock<std::mutex> lk(m_mtx_que);
    m_que_ev.push_back(ev);
    return;
  }

  EventSPC ExportEventPS::GetEvent(){
    EventSPC ev;
    std::unique_lock<std::mutex> lk(m_mtx_que);
    if(!m_que_ev.empty()){
      ev = m_que_ev.front();
      m_que_ev.pop_front();
    }
    return ev;
  }
  
}

