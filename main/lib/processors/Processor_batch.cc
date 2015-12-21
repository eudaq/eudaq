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

int Processor_batch::pushProcessor(Processor_up processor) {
  if (!processor)
  {
    EUDAQ_THROW("trying to push Empty Processor to batch");
  }
  pushProcessor(processor.get());
  m_processors->push_back(std::move(processor));

  return 1;

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
   auto ret = make_batch();
   ret->pushProcessor(std::move(proc1));
   ret->pushProcessor(proc2);
   return ret;
 }

 Processor_batch_up operator>>(Processor_up proc1, Processor_up proc2) {
   auto ret = make_batch();
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

 Processor_batch_up operator>>(Processor_batch_up batch, Processor_up_splitter proc) {
  
   batch->pushProcessor(std::move(proc));
   return batch;
 }

 Processor_batch_up operator>>(Processor_batch_up batch, Processor_i_batch_up proc) {
   batch->pushProcessor(std::move(proc));
   return batch;
 }

 Processor_batch& operator>>(Processor_batch& batch, processor_view proc) {
   proc.getValue(batch);
   return batch;
 }

 Processor_batch_up operator>>(processor_view first_, processor_view second_) {
   auto ret = make_batch();
   first_.getValue(*ret);
   second_.getValue(*ret);
   return ret;
 }

 std::unique_ptr<Processor_batch> operator>>(std::unique_ptr<Processor_batch> batch, processor_view proc) {
   proc.getValue(*batch);
   return batch;
 }

 Processor_i_batch_up operator*(processor_i_up first_, processor_i_up second_) {
   auto ret = __make_unique<Processor_i_batch>();
   ret->pushProcessor(std::move(first_));
   ret->pushProcessor(std::move(second_));
   return ret;
 }

 Processor_i_batch_up operator*(Processor_i_batch_up first_, processor_i_up second_) {
   first_->pushProcessor(std::move(second_));
   return first_;
 }

 Processor_i_batch_up operator*(inspector_view first_, inspector_view second_) {
   auto ret = __make_unique<Processor_i_batch>();
   first_.getValue(*ret);
   second_.getValue(*ret);
   return ret;
 }

 Processor_i_batch_up operator*(Processor_i_batch_up first_, inspector_view second_) {

   second_.getValue(*first_);
   return first_;
 }



 void helper_push_r_pointer(Processor_batch& batch, Processor_rp proc) {
    batch.pushProcessor(proc);
 }
 void helper_push_u_pointer(Processor_batch& batch, Processor_up proc) {
   batch.pushProcessor(std::move(proc));
 }

 void helper_push_r_pointer(Processor_batch_splitter& batch, Processor_rp proc) {
   batch.pushProcessor(proc);
 }


 void helper_push_i_r_pointer(Processor_i_batch& batch, processor_i_rp proc) {
   batch.pushProcessor(proc);
 }
 void helper_push_i_u_pointer(Processor_i_batch& batch, processor_i_up proc) {
   batch.pushProcessor(std::move(proc));
 }

 void helper_push_u_pointer(Processor_batch_splitter& batch, Processor_up proc) {
   batch.pushProcessor(std::move(proc));
 }
 Processor_up_splitter splitter() {
  
   return __make_unique<Processor_batch_splitter>();
   
 }



 Processor_up_splitter operator+(processor_view first_, processor_view secound_) {
   auto ret = splitter();
   first_.getValue(*ret);
   secound_.getValue(*ret);
   return ret;
 }

 Processor_up_splitter operator+(Processor_up_splitter splitter_, processor_view secound_) {
   secound_.getValue(*splitter_);
   return splitter_;
 }



 std::unique_ptr<Processor_batch> make_batch() {
   return __make_unique<Processor_batch>();
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

 Processor_i_batch::Processor_i_batch() {
   m_processors = __make_unique<std::vector<processor_i_up>>();
   
 }

 void Processor_i_batch::init() {
   for (auto& e : reverse(m_processors_rp)) {
     e->init();
   }
 
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
 }

 void Processor_i_batch::reset() {
   for (auto& e : m_processors_rp) {
     e->wait();
   }
   m_processors->clear();
   m_processors_rp.clear();
 }



 ReturnParam Processor_batch_splitter::ProcessEvent(event_sp ev, ConnectionName_ref con) {
   for (auto& e : m_processors_rp) {
     auto ret = e->ProcessEvent(ev, con);
     if (ret != ProcessorBase::sucess) {
       return ret;
     }
   }
   return processNext(std::move(ev), con);
 }



 void Processor_batch_splitter::pushProcessor(Processor_rp processor) {
   if (!processor) {
     EUDAQ_THROW("trying to push Empty Processor to batch");
   }

   m_processors_rp.push_back(processor);
 }

 void Processor_batch_splitter::pushProcessor(Processor_up processor) {
   if (!processor) {
     EUDAQ_THROW("trying to push Empty Processor to batch");
   }
   pushProcessor(processor.get());
   m_processors->push_back(std::move(processor));
 }

 void Processor_batch_splitter::init() {
   for (auto& e : reverse(m_processors_rp)) {
     e->AddNextProcessor(m_next);
     e->init();
   }
 }

 void Processor_batch_splitter::end() {
   for (auto& e : m_processors_rp) {
     e->end();
   }
 }

}