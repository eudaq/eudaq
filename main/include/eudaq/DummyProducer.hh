#ifndef EUDAQ_INCLUDED_DummyProducer
#define EUDAQ_INCLUDED_DummyProducer

#include "eudaq/Producer.hh"

namespace eudaq {

  class DummyProducer : public Producer {
  public:
    DummyProducer(const std::string & name, const std::string & runcontrol);
  };

}

#endif // EUDAQ_INCLUDED_DummyProducer
