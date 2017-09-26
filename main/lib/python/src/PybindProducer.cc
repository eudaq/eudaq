#include "pybind11/pybind11.h"
#include "eudaq/Producer.hh"

namespace py = pybind11;

class PyProducer : public eudaq::Producer {
public:
  using eudaq::Producer::Producer;
  
  void DoInitialise() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoInitialise
		      );
  }
  void DoConfigure() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoConfigure
		      );
  }
  void DoStartRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoStartRun
		      );
  }
  void DoStopRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoStopRun
		      );
  }
  void DoReset() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoReset
		      );
  }
  void DoTerminate() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoTerminate
		      );
  }

  void Exec() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      Exec
		      );
  }
  
};


void init_pybind_producer(py::module &m){
  py::class_<eudaq::Producer, PyProducer, std::shared_ptr<eudaq::Producer>>
    producer_(m, "Producer");
  producer_.def(py::init<const std::string&, const std::string&>());
  producer_.def("Exec", &eudaq::Producer::Exec);
  producer_.def("DoInitialise", &eudaq::Producer::DoInitialise);
  producer_.def("DoConfigure", &eudaq::Producer::DoConfigure);
  producer_.def("DoStartRun", &eudaq::Producer::DoStartRun);
  producer_.def("DoStopRun", &eudaq::Producer::DoStopRun);
  producer_.def("DoReset", &eudaq::Producer::DoReset);
  producer_.def("DoTerminate", &eudaq::Producer::DoTerminate);
  producer_.def("SendEvent", &eudaq::Producer::SendEvent,
  		"Send an Event", py::arg("ev"));
}
