#include "pybind11/pybind11.h"
#include "eudaq/Producer.hh"

namespace py = pybind11;

class PyProducer;


namespace{
  auto dummy = eudaq::Factory<eudaq::Producer>::
    Register<PyProducer, const std::string&, const std::string&>
    (eudaq::cstr2hash("PyProducer"));
}

class PyProducer : public eudaq::Producer {
public:
  using eudaq::Producer::Producer;

  static eudaq::ProducerSP Make(const std::string &code_name, const std::string &name, const std::string &runctrl){
    if(code_name != "PyProducer"){
      EUDAQ_THROW("The code_name of Python producer is not PyProducer.");
    }
    return eudaq::Producer::Make(code_name, name, runctrl);
  };


  void DoStatus() override {
	  PYBIND11_OVERLOAD(void, /* Return type */
	                    eudaq::Producer,
	                    DoStatus
	                    );

  }
	
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

  void RunLoop() override{
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      RunLoop
		      );
  }
  
};


void init_pybind_producer(py::module &m){
  py::class_<eudaq::Producer, PyProducer, std::shared_ptr<eudaq::Producer>>
    producer_(m, "Producer");
  producer_.def(py::init(&eudaq::Producer::Make, &PyProducer::Make));
  producer_.def("DoStatus", &eudaq::Producer::DoStatus);
  producer_.def("DoInitialise", &eudaq::Producer::DoInitialise);
  producer_.def("DoConfigure", &eudaq::Producer::DoConfigure);
  producer_.def("DoStartRun", &eudaq::Producer::DoStartRun);
  producer_.def("DoStopRun", &eudaq::Producer::DoStopRun);
  producer_.def("DoReset", &eudaq::Producer::DoReset);
  producer_.def("DoTerminate", &eudaq::Producer::DoTerminate);
  producer_.def("RunLoop", &eudaq::Producer::RunLoop);
  producer_.def("SendEvent", &eudaq::Producer::SendEvent,
  		"Send an Event", py::arg("ev"));
  producer_.def("Connect", &eudaq::Producer::Connect);
  producer_.def("IsConnected", &eudaq::Producer::IsConnected);
  producer_.def("GetConfigItem", &eudaq::Producer::GetConfigItem,
		"Get an item from Producer's config section", py::arg("key"));
  producer_.def("GetInitItem", &eudaq::Producer::GetInitItem,
		"Get an item from Producer's init section", py::arg("key")
		);
  producer_.def("SetStatusTag", &eudaq::Producer::SetStatusTag,
                "", py::arg("key"), py::arg("val"));


}
