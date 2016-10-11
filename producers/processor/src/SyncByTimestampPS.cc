#include "Processor.hh"
#include "Event.hh"
#include "DetectorEvent.hh"

#include <map>
#include <deque>

using namespace eudaq;
class SyncByTimestampPS;

namespace{
  auto dummy0 = Factory<Processor>::Register<SyncByTimestampPS, std::string&>
    (eudaq::cstr2hash("SyncByTimestampPS"));
  auto dummy1 = Factory<Processor>::Register<SyncByTimestampPS, std::string&&>
    (eudaq::cstr2hash("SyncByTimestampPS"));
}


class SyncByTimestampPS: public Processor{
public:
  SyncByTimestampPS(std::string cmd);
  virtual void ProcessUserEvent(EVUP ev);
  // virtual void ProcessUserCmd();
  
private:
  uint32_t m_nstream;
  uint32_t m_nready;
  uint64_t m_ts_last_end;
  uint64_t m_ts_next_begin;
  uint64_t m_ts_next_end;
  uint64_t m_ts_next_next_begin;
  uint64_t m_ts_next_next_end;

  std::map<uint32_t, std::deque<EVUP>> m_fifos;
  std::map<uint32_t, EVUP> m_bores;
  std::map<uint32_t, bool> m_ready;
};


SyncByTimestampPS::SyncByTimestampPS(std::string cmd)
  :Processor("SyncByTimestampPS", ""){
  ProcessCmd(cmd);

  m_nstream = 4;
  m_nready = 0;
  m_ts_last_end=0;
  m_ts_next_begin= uint64_t(-1);
  m_ts_next_end= uint64_t(-1);
  m_ts_next_next_begin= uint64_t(-1);
  m_ts_next_next_end= uint64_t(-1);

  
}

void SyncByTimestampPS::ProcessUserEvent(EVUP ev){
  if(ev->GetEventID() != Event::str2id("_RAW")){
    ForwardEvent(std::move(ev));
    ;
  }
  
  std::cout<< "merging " <<"  EVType="<<ev->GetSubType()<<"  EVNum="<<ev->GetEventNumber()<<std::endl;
  auto ts_begin = ev->GetTimestampBegin(); //begin
  auto ts_end = ev->GetTimestampEnd(); //end;
  uint32_t id_stream_ev = ev->GetStreamID();
  std::cout<< "ts_begin ="<< ts_begin << "  ts_end ="<<ts_end<< "  id_stream ="<<id_stream_ev<<std::endl;
  
  if(ev->IsBORE()){
    m_bores[id_stream_ev] = std::move(ev); //move, !!!
    if(m_bores.size() == m_nstream){
      auto&  dummy = m_fifos[id_stream_ev];
      ;
      std::cout<< "full bore"<<std::endl;
      //TODO: merge a BORE DETEvent
    }
    return;
  }

  
  if(ev->IsEORE()){
    return;
  }
  if(ts_begin>=ts_end){
    return;
    //ERROR
  }

  m_fifos[id_stream_ev].push_back(std::move(ev));
  
  if(!m_ready[id_stream_ev]){
    m_ready[id_stream_ev] = true;
    m_nready ++;
  }
  if(ts_begin<m_ts_next_begin){
    m_ts_next_begin = ts_begin;
  }
  
  if(ts_end<m_ts_next_end){
    m_ts_next_end = ts_end;
    //if m_ts_next_begin=>m_ts_next_end overlapps the whole old event;
    for(auto &e: m_ready){
      if(!e.second){
	auto &fifo = m_fifos[e.first];
	if(!fifo.empty()){ //if not empty, there should be only one old event
	  if(fifo.front()->GetTimestampEnd()>=m_ts_next_end){ // old event valid
	    e.second= true;
	    m_nready ++;
	  }
	}
      }
    }
  }
  std::cout<<"m_nready "<< m_nready <<std::endl;
  while(m_nready == m_fifos.size()){ // when a wating event arrives in a stream,  N DETEvent can be built
    std::cout<<">>>>in loop: m_nready "<< m_nready <<std::endl;
    std::cout<<"\n\n\n";
    std::cout<<"...................a detector event is begin"<<std::endl;
    std::cout<<"m_ts_last_end "<<m_ts_last_end<<std::endl;
    std::cout<<"m_ts_next_begin "<<m_ts_next_begin<<std::endl;
    std::cout<<"m_ts_next_end "<<m_ts_next_end<<std::endl;
    std::cout<<"m_ts_next_next_begin "<<m_ts_next_next_begin<<std::endl;
    std::cout<<"m_ts_next_next_end "<<m_ts_next_next_end<<std::endl;

    
    auto evup_det = Factory<Event>::Create<>(Event::str2id("_DET"));
    evup_det->SetTimestampBegin(m_ts_next_begin);
    evup_det->SetTimestampEnd(m_ts_next_end);
    auto ev_det = dynamic_cast<DetectorEvent*> (evup_det.get());

    for(auto &e: m_fifos){
      EVUP ev_candidate;
      uint32_t id_stream = e.first;
      auto &ready_stream = m_ready[id_stream];
      auto &fifo = e.second;
      ready_stream = false;
      m_nready --;
      
      if(fifo.front()->GetTimestampBegin()<m_ts_next_begin){ //old event;
	std::cout<< "it is old event\n";
	if(fifo.front()->GetTimestampEnd()<=m_ts_next_begin){ //no overlap, pop
	  fifo.pop_front();  //update next_next later
	  std::cout<< "pop fifo\n";
	}
	else if(fifo.front()->GetTimestampEnd()<m_ts_next_end){ //candidate, pop, not valid after m_ts_next_end
	  ev_candidate = std::move(fifo.front()); 
	  fifo.pop_front();  //update next_next later
	  std::cout<< "candidate, pop fifo\n";
	}
	else if(fifo.front()->GetTimestampEnd()==m_ts_next_end){ //candidate, pop, full overlap, not valid after m_ts_next_end
	  ev_candidate = std::move(fifo.front()); 
	  fifo.pop_front();
	  if(!fifo.empty()){
	    ready_stream = true;
	    m_nready ++;
	    if(fifo.front()->GetTimestampBegin()<m_ts_next_next_begin)
	      m_ts_next_next_begin = fifo.front()->GetTimestampBegin();
	    if(fifo.front()->GetTimestampEnd()<m_ts_next_next_end)
	      m_ts_next_next_end = fifo.front()->GetTimestampEnd();
	  }
	  ev_det->AddEvent(std::move(ev_candidate));
	  std::cout<< "candidate, pop fifo, add\n";
	  continue;
	}else{ //candidate, pop, full overlap, valid after m_ts_next_end
	  ev_candidate = fifo.front()->Clone();
	  //update next_next now
	  if(fifo.size()>1){ //if not, ready may be still true. ts_fifo_front may be very long.
	    ready_stream = true;
	    m_nready ++;
	    if(fifo[1]->GetTimestampBegin()<m_ts_next_next_begin)
	      m_ts_next_next_begin = fifo[1]->GetTimestampBegin();
	    if(fifo[1]->GetTimestampEnd()<m_ts_next_next_end)
	      m_ts_next_next_end = fifo[1]->GetTimestampEnd();
	  }
	  ev_det->AddEvent(std::move(ev_candidate));
	  std::cout<< "candidate, add\n";
	  continue;
	}
      }
      
      if(fifo.front()->GetTimestampBegin()>=m_ts_next_begin){ //new event
	std::cout<< "it is new event\n";
	if(fifo.front()->GetTimestampBegin()<m_ts_next_end){// overlap, candidate
	  // if(ev_candiate){; replace or reserve the old}
	  if(fifo.front()->GetTimestampEnd()==m_ts_next_end){ // full overlap, no <, only =
	    ev_candidate = std::move(fifo.front()); 
	    fifo.pop_front();
	    std::cout<< "candidate, pop\n";
	    if(!fifo.empty()){
	      ready_stream = true;
	      m_nready ++;
	      if(fifo.front()->GetTimestampBegin()<m_ts_next_next_begin)
		m_ts_next_next_begin = fifo.front()->GetTimestampBegin();
	      if(fifo.front()->GetTimestampEnd()<m_ts_next_next_end)
		m_ts_next_next_end = fifo.front()->GetTimestampEnd();
	    }
	    ev_det->AddEvent(std::move(ev_candidate));
	    std::cout<< "det add candidate\n";
	    continue;
	  }
	  else{ //partial overlap
	    ev_candidate = fifo.front()->Clone();
	    std::cout<< "candidate\n";
	    if(fifo.size()>1){ //if not, ready may be still true. ts_fifo_front may be very long.
	      ready_stream = true;
	      m_nready ++;
	      if(fifo[1]->GetTimestampBegin()<m_ts_next_next_begin)
		m_ts_next_next_begin = fifo[1]->GetTimestampBegin();
	      if(fifo[1]->GetTimestampEnd()<m_ts_next_next_end)
		m_ts_next_next_end = fifo[1]->GetTimestampEnd();
	    }
	    ev_det->AddEvent(std::move(ev_candidate));
	    std::cout<< "det add candidate\n";
	    continue;
	  }
	  
	}
	else{//event is too new
	  std::cout<< "too new\n";
	  ready_stream = true;
	  m_nready ++;
	  if(fifo.front()->GetTimestampBegin()<m_ts_next_next_begin)
	    m_ts_next_next_begin = fifo.front()->GetTimestampBegin();
	  if(fifo.front()->GetTimestampEnd()<m_ts_next_next_end)
	    m_ts_next_next_end = fifo.front()->GetTimestampEnd();
	  // if(ev_candidate){; option to add the old}
	  if(ev_candidate){
	    ev_det->AddEvent(std::move(ev_candidate));
	    std::cout<< "det add oldevent candidate\n";
	  }
	}
      }
    }//end loop the fifos


    ForwardEvent(std::move(evup_det));
    std::cout<<"...................a detector event is completed"<<std::endl;
    std::cout<<"m_ts_last_end "<<m_ts_last_end<<std::endl;
    std::cout<<"m_ts_next_begin "<<m_ts_next_begin<<std::endl;
    std::cout<<"m_ts_next_end "<<m_ts_next_end<<std::endl;
    std::cout<<"m_ts_next_next_begin "<<m_ts_next_next_begin<<std::endl;
    std::cout<<"m_ts_next_next_end "<<m_ts_next_next_end<<std::endl;
    
    m_ts_last_end = m_ts_next_end;
    m_ts_next_begin = m_ts_next_next_begin;
    m_ts_next_end = m_ts_next_next_end;
    m_ts_next_next_begin = uint64_t(-1);
    m_ts_next_next_end = uint64_t(-1);


    for(auto &e: m_ready){
      if(!e.second){
	auto &fifo = m_fifos[e.first];
	if(!fifo.empty()){ //if not empty, there should be only one old event
	  if(fifo.front()->GetTimestampEnd()>=m_ts_next_end){ // old event valid
	    e.second= true;
	    m_nready ++;
	  }
	}
      }
    }
    
  }//end loop, merged as many as posssable
    
}
