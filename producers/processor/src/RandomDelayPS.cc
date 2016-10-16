#include "Processor.hh"
#include "Event.hh"

#include <map>
#include <deque>
#include <stdexcept>
#include <random>


using namespace eudaq;
class RandomDelayPS;

namespace{
  auto dummy0 = Factory<Processor>::Register<RandomDelayPS, std::string&>
    (eudaq::cstr2hash("RandomDelayPS"));
  auto dummy1 = Factory<Processor>::Register<RandomDelayPS, std::string&&>
    (eudaq::cstr2hash("RandomDelayPS"));
}


class RandomDelayPS: public Processor{
public:
  RandomDelayPS(std::string cmd);
  void ProcessUserEvent(EVUP ev);
  void ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par);
  EventUP GetEventFromBuffer();

private:
  std::map<uint32_t, std::deque<EVUP>> m_fifos;
  uint32_t m_nevent;
  uint32_t m_ndelay;
  bool m_isend;
  std::random_device m_rd;
  std::mt19937 m_gen;

};

  
RandomDelayPS::RandomDelayPS(std::string cmd)
  :Processor("RandomDelayPS", ""){
  ProcessCmd(cmd);
  m_nevent = 0;
  m_ndelay = 100;
  m_isend = false;
  m_gen.seed(m_rd());
}

void RandomDelayPS::ProcessUsrCmd(const std::string cmd_name, const std::string cmd_par){
  switch(cstr2hash(cmd_name.c_str())){
  case cstr2hash("EMPTY"):{
    while(m_nevent){
      std::cout<< "................m_nevent = "<<m_nevent<<std::endl;
      ForwardEvent(GetEventFromBuffer());
    }
    break;
  }
  default:
    std::cout<<"unkonw user cmd"<<std::endl;
  }

}

void RandomDelayPS::ProcessUserEvent(EVUP ev){

  auto stm_n = ev->GetStreamN();
  m_fifos[stm_n].push_back(std::move(ev));
  m_nevent++;
  
  if(m_nevent<m_ndelay){
    return;
  }
  ForwardEvent(GetEventFromBuffer());
  
}

EventUP RandomDelayPS::GetEventFromBuffer(){
  std::vector<uint32_t> ready; 
  for(auto &e: m_fifos){
    auto fifo_stm = e.first;
    auto fifo_len = e.second.size();
    if(fifo_len)
      ready.insert(ready.end(), fifo_len, fifo_stm);
  }
  
  std::uniform_int_distribution<uint32_t> dis(0, ready.size()-1);
  auto stm_n_output = ready[dis(m_gen)];
  EVUP ev_output = std::move(m_fifos[stm_n_output].front());
  m_fifos[stm_n_output].pop_front();
  m_nevent--;
  return ev_output;
}
