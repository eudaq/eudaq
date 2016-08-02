#include "Processor_batch.hh"
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
