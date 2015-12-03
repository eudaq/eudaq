#include "eudaq/Processor_batch.hh"
#include <memory>


template<class Cont>
class const_reverse_wrapper {
  const Cont& container;

public:
  const_reverse_wrapper(const Cont& cont) : container(cont) {}
  auto begin() const -> decltype(container.rbegin()) { return container.rbegin(); }
  auto end() const -> decltype(container.rbegin()) { return container.rend(); }
};

template<class Cont>
class reverse_wrapper {
  Cont& container;

public:
  reverse_wrapper(Cont& cont) : container(cont) {}
  auto begin()-> decltype(container.rbegin()) { return container.rbegin(); }
  auto end()-> decltype(container.rbegin()) { return container.rend(); }
};

template<class Cont>
const_reverse_wrapper<Cont> reverse(const Cont& cont) {
  return const_reverse_wrapper<Cont>(cont);
}

template<class Cont>
reverse_wrapper<Cont> reverse(Cont& cont) {
  return reverse_wrapper<Cont>(cont);
}

namespace eudaq {
using ReturnParam = ProcessorBase::ReturnParam;
Processor_batch::Processor_batch() : m_processors(__make_unique<std::vector<Processor_up>>()) {


}

Processor_batch::~Processor_batch() {

}

ReturnParam Processor_batch::ProcessEvent(event_sp ev, ConnectionName_ref con) {
  return m_first->ProcessEvent(ev, con);
}

void Processor_batch::init() {
  
  for (auto& e : reverse(m_processors_rp))
  {
    e->init();
  }
  m_first = m_processors_rp.front();
}

void Processor_batch::end() {
  for (auto& e : m_processors_rp) {
    e->end();
  }
}

void Processor_batch::pushProcessor(Processor_up processor) {
  if (!processor)
  {
    EUDAQ_THROW("trying to push Empty Processor to batch");
  }
  pushProcessor(processor.get());
  m_processors->push_back(std::move(processor));



}



void Processor_batch::pushProcessor(Processor_rp processor) {
  if (!processor) {
    EUDAQ_THROW("trying to push Empty Processor to batch");
  }

  m_processors_rp.push_back(processor);
  auto new_last = m_processors_rp.back();
  if (m_last) {
    m_last->AddNextProcessor(new_last);
  }

  m_last = new_last;
}

void Processor_batch::wait() {
  for (auto& e : m_processors_rp) {
    e->wait();
  }
} 

void Processor_batch::run() {
  ReturnParam ret = sucess;
  do {
    ret = ProcessEvent(nullptr, 0);
  } while (ret != stop);
  wait();
}

void Processor_batch::reset() {
  for (auto& e : m_processors_rp) {
    e->wait();
  }
  m_processors->clear();
  m_processors_rp.clear();
}

Processor_batch& operator>>(Processor_batch& batch, Processor_up proc) {
  batch.pushProcessor(std::move(proc));
  return batch;
}

 Processor_batch& operator>>(Processor_batch& batch, Processor_rp proc) {
   batch.pushProcessor(proc);
   return batch;
}

 Processor_batch& operator>>(Processor_batch& batch, processor_i_up proc) {
   batch.pushProcessor(std::move(proc));
   return batch;
 }

 Processor_batch_up operator>>(Processor_up proc1, Processor_rp proc2) {
   auto ret = Processor_batch_up(new Processor_batch());
   ret->pushProcessor(std::move(proc1));
   ret->pushProcessor(proc2);
   return ret;
 }

 Processor_batch_up operator>>(Processor_up proc1, Processor_up proc2) {
   auto ret = Processor_batch_up(new Processor_batch());
   ret->pushProcessor(std::move(proc1));
   ret->pushProcessor(std::move(proc2));
   return ret;
 }

 
 Processor_batch_up operator>>(Processor_batch_up batch, Processor_rp proc) {
   batch->pushProcessor(proc);
   return batch;
 }

 Processor_batch_up operator>>(Processor_batch_up batch, Processor_up proc) {
   batch->pushProcessor(std::move(proc));
   return batch;
 }

 Processor_batch_up operator>>(Processor_batch_up batch, processor_i_up proc) {
   batch->pushProcessor(std::move(proc));
   return batch;
 }

 std::unique_ptr<Processor_batch> make_batch() {
   return std::unique_ptr<Processor_batch>(new Processor_batch());
 }

 ReturnParam Processor_i_batch::inspectEvent(const Event& ev, ConnectionName_ref con) {
   for (auto e:m_processors_rp) {
     auto ret = e->inspectEvent(ev, con);
     if (ret!=sucess) {
       return ret;
     }
   }
   return ProcessorBase::sucess;
 }

 void Processor_i_batch::init() {
   for (auto& e : reverse(m_processors_rp)) {
     e->init();
   }
   m_first = m_processors_rp.front();
 }

 void Processor_i_batch::end() {
   for (auto& e : m_processors_rp) {
     e->end();
   }
 }

 void Processor_i_batch::wait() {
   for (auto& e : m_processors_rp) {
     e->wait();
   }
 }

 void Processor_i_batch::pushProcessor(processor_i_up processor) {
   if (!processor) {
     EUDAQ_THROW("trying to push Empty Processor to batch");
   }
   pushProcessor(processor.get());
   m_processors->push_back(std::move(processor));
 }

 void Processor_i_batch::pushProcessor(processor_i_rp processor) {
   if (!processor) {
     EUDAQ_THROW("trying to push Empty Processor to batch");
   }

   m_processors_rp.push_back(processor);
   auto new_last = m_processors_rp.back();
   if (m_last) {
     m_last->AddNextProcessor(new_last);
   }

   m_last = new_last;
 }

 void Processor_i_batch::reset() {
   for (auto& e : m_processors_rp) {
     e->wait();
   }
   m_processors->clear();
   m_processors_rp.clear();
 }



 void Processor_batch_splitter::init() {
   for (auto& e : reverse(m_processors_rp)) {
     e->init();
   }
 }

 void Processor_batch_splitter::end() {
   for (auto& e : m_processors_rp) {
     e->end();
   }
 }

}