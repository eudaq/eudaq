#include "Processor.hh"
#include "Event.hh"

#include <map>
#include <deque>
#include <stdexcept>

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
  void AddEvent(EVUP ev);
  EVUP GetMergedEvent();
  void UpdateFifoStatus();
  void UpdateNextNextTimestamp(Event *ev);
private:
  uint32_t m_nstream;
  uint32_t m_nready;
  uint64_t m_ts_last_end;
  uint64_t m_ts_next_begin;
  uint64_t m_ts_next_end;
  uint64_t m_ts_next_next_begin;
  uint64_t m_ts_next_next_end;
  uint32_t m_event_n;
  
  std::map<uint32_t, std::deque<EVUP>> m_fifos;
  std::map<uint32_t, EVUP> m_bores;
  std::map<uint32_t, bool> m_ready;
};


SyncByTimestampPS::SyncByTimestampPS(std::string cmd)
  :Processor("SyncByTimestampPS", ""){
  ProcessCmd(cmd);
  m_nstream = 2;
  m_nready = 0;
  m_ts_last_end=0;
  m_ts_next_begin= uint64_t(-1);
  m_ts_next_end= uint64_t(-1);
  m_ts_next_next_begin= uint64_t(-1);
  m_ts_next_next_end= uint64_t(-1);

  m_event_n = 0;
}


void SyncByTimestampPS::AddEvent(EVUP ev){
  if(ev->IsBORE()){
    m_bores[ev->GetStreamN()] = std::move(ev); //move, !!!
    m_fifos[ev->GetStreamN()];
    if(m_bores.size() == m_nstream){
      std::cout<< "full bore"<<std::endl;
      //TODO: merge a BORE DETEvent
    }
    return;
  }
  if(ev->IsEORE())
    return;

  std::cout<<"\n>>>>>>New event is comming!\n";
  ev->Print(std::cout, 2);
  auto ts_begin = ev->GetTimestampBegin();
  auto ts_end = ev->GetTimestampEnd();  
  auto stm_n = ev->GetStreamN();
  auto &fifo = m_fifos[stm_n];
  auto &isReady = m_ready[stm_n];
  if(ts_begin>=ts_end)
    throw std::domain_error("ts_begin>=ts_end");
  if(ts_begin<m_ts_last_end)
    throw std::domain_error("ts_begin<m_ts_last_end");
  if(!fifo.empty() && ts_begin < fifo.back()->GetTimestampEnd())
    throw std::domain_error("ts_begin < fifo.back()->GetTimestampEnd()");

  fifo.push_back(std::move(ev));
  if(!isReady){
    isReady = true;
    m_nready ++;
  }
  if(ts_begin<m_ts_next_begin){
    m_ts_next_begin = ts_begin;
  }
  if(ts_end<m_ts_next_end){
    m_ts_next_end = ts_end;
    UpdateFifoStatus(); //Do it when next_end is updated
  }

  std::cout<<"m_nready "<< m_nready <<std::endl;
}

void SyncByTimestampPS::ProcessUserEvent(EVUP ev){
  AddEvent(std::move(ev));
  while(m_nready == m_nstream){ //IsReady()
    std::cout<<"\n\n\n";
    std::cout<<">>>>in loop: m_nready "<< m_nready <<std::endl;
    std::cout<<"...................a detector event is begin............."<<std::endl;
    std::cout<<"  last_end ="<<m_ts_last_end;
    std::cout<<"  next_begin ="<<m_ts_next_begin;
    std::cout<<"  next_end ="<<m_ts_next_end;
    std::cout<<"  nnext_begin ="<<m_ts_next_next_begin;
    std::cout<<"  nnext_end ="<<m_ts_next_next_end<<std::endl;
    std::cout<<"........................................................."<<std::endl;

    EVUP ev_merged(GetMergedEvent());
    ev_merged->Print(std::cout);
    ForwardEvent(std::move(ev_merged));
  }
    
}


EVUP SyncByTimestampPS::GetMergedEvent(){
  EVUP ev = Factory<Event>::Create<const uint32_t&, const uint32_t&, const uint32_t&>(cstr2hash("SYNC"), cstr2hash("SYNC"), 0, GetID());
  ev->SetTimestampBegin(m_ts_next_begin);
  ev->SetTimestampEnd(m_ts_next_end);
  
  for(auto &e: m_fifos){
    //print fifos layout
    
    EVUP ev_candidate;
    uint32_t id_stream = e.first;
    auto &isReady = m_ready[id_stream];
    auto &fifo = e.second;
    isReady = false;
    m_nready --;

    auto ts_begin = fifo.front()->GetTimestampBegin();
    auto ts_end = fifo.front()->GetTimestampEnd();
    if(ts_begin<m_ts_next_begin){ //old event;
      std::cout<< "it is old event\n";
      if(ts_end<=m_ts_next_begin){ //no overlap, pop
	fifo.pop_front();  //update next_next later
	std::cout<< "pop fifo\n";
      }
      else if(ts_end<m_ts_next_end){ //candidate, pop, not valid after m_ts_next_end
	ev_candidate = std::move(fifo.front());
	fifo.pop_front();  //update next_next later
	std::cout<< "candidate, pop fifo\n";
      }
      else if(ts_end==m_ts_next_end){ //candidate, pop, full overlap, not valid after m_ts_next_end
	ev_candidate = std::move(fifo.front());
	fifo.pop_front();
	if(!fifo.empty()){
	  isReady = true;
	  m_nready ++;
	  UpdateNextNextTimestamp(fifo.front().get());
	}
	ev->AddEvent(std::move(ev_candidate));
	std::cout<< "candidate, pop fifo, add\n";
	continue;
      }else{ //candidate, pop, full overlap, valid after m_ts_next_end
	ev_candidate = fifo.front()->Clone();
	if(fifo.size()>1){ //if not, ready may be still true. ts_fifo_front may be very long.
	  isReady = true;
	  m_nready ++;
	  UpdateNextNextTimestamp(fifo[1].get());
	}
	ev->AddEvent(std::move(ev_candidate));
	std::cout<< "ext overlap\n";
	continue;
      }
    }
      
    if(fifo.empty()) throw std::domain_error("This fifo should not be ready!");
    ts_begin = fifo.front()->GetTimestampBegin();
    ts_end = fifo.front()->GetTimestampEnd();
    if(ts_begin>=m_ts_next_begin){ //new event
      std::cout<< "it is new event\n";
      if(ts_begin<m_ts_next_end){// overlap, candidate
	// if(ev_candiate){; replace or reserve the old}
	if(ts_end==m_ts_next_end){ // full overlap, no <, only =
	  ev_candidate = std::move(fifo.front()); 
	  fifo.pop_front();
	  std::cout<< "candidate, pop\n";
	  if(!fifo.empty()){
	    isReady = true;
	    m_nready ++;
	    UpdateNextNextTimestamp(fifo.front().get());
	  }
	  ev->AddEvent(std::move(ev_candidate));
	  std::cout<< "det add candidate\n";
	  continue;
	}
	else{ //partial overlap
	  ev_candidate = fifo.front()->Clone();
	  std::cout<< "candidate\n";
	  if(fifo.size()>1){ //if not, ready may be still true. ts_fifo_front may be very long.
	    isReady = true;
	    m_nready ++;
	    UpdateNextNextTimestamp(fifo[1].get());
	  } //Because next_next_end is not updated to its finnal value, isReady is unknown. Leave isReady to be false and checked later.
	  ev->AddEvent(std::move(ev_candidate));
	  std::cout<< "det add candidate\n";
	  continue;
	}
	  
      }
      else{//event is too new
	std::cout<< "too new\n";
	isReady = true;
	m_nready ++;
	UpdateNextNextTimestamp(fifo.front().get());
	// // if(ev_candidate){; option to add the old}
	if(ev_candidate){
	  ev->AddEvent(std::move(ev_candidate));
	  std::cout<< "det add oldevent candidate\n";
	}
      }
    }
  }//end loop the fifos

  std::cout<<"...................a detector event is completed........."<<std::endl;
  std::cout<<"  last_end ="<<m_ts_last_end;
  std::cout<<"  next_begin ="<<m_ts_next_begin;
  std::cout<<"  next_end ="<<m_ts_next_end;
  std::cout<<"  nnext_begin ="<<m_ts_next_next_begin;
  std::cout<<"  nnext_end ="<<m_ts_next_next_end<<std::endl;
  std::cout<<"  SubEvent ="<<ev->GetNumSubEvent()<<std::endl;
  std::cout<<"........................................................."<<std::endl;
  m_ts_last_end = m_ts_next_end;
  m_ts_next_begin = m_ts_next_next_begin;
  m_ts_next_end = m_ts_next_next_end;
  m_ts_next_next_begin = uint64_t(-1);
  m_ts_next_next_end = uint64_t(-1);

  UpdateFifoStatus();
  return ev;
}


void SyncByTimestampPS::UpdateFifoStatus(){ //Do it when next_end is updated
  for(auto &e: m_ready){
    if(!e.second){ //test the unready fifo, since the next_end is moved to earlier.
      auto &fifo = m_fifos[e.first];
      if(!fifo.empty()){
	if(fifo.size()>1) throw std::domain_error("The unready fifo should have maximiun 1 old event!");
	if(fifo.front()->GetTimestampEnd()>=m_ts_next_end){ // old event valid
	  e.second= true;
	  m_nready ++;
	}
      }
    }
  }
  
}


void SyncByTimestampPS::UpdateNextNextTimestamp(Event *ev){
  if(ev->GetTimestampBegin()<m_ts_next_next_begin)
    m_ts_next_next_begin = ev->GetTimestampBegin();
  if(ev->GetTimestampEnd()<m_ts_next_next_end)
    m_ts_next_next_end = ev->GetTimestampEnd();
}
