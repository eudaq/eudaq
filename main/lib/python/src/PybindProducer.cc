#include "pybind11/pybind11.h"
#include "pybind11/stl.h"

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
  int GetRunNumber() {
    PYBIND11_OVERLOAD(int, /* Return type */
                      eudaq::Producer, 
                      GetRunNumber);
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
  void DoStatus() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Producer,
		      DoStatus
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
  producer_.def(py::init([](const std::string &name,const std::string &runctrl){return PyProducer::Make("PyProducer", name, runctrl);}));
  producer_.def("SetStatusTag", &eudaq::Producer::SetStatusTag);
  producer_.def("SetStatusMsg", &eudaq::Producer::SetStatusMsg);
  producer_.def("GetRunNumber", &eudaq::Producer::GetRunNumber);
  producer_.def("RunLoop", &eudaq::Producer::RunLoop);
  producer_.def("SendEvent", &eudaq::Producer::SendEvent,
  		"Send an Event", py::arg("ev"));
  producer_.def("Connect", &eudaq::Producer::Connect);
  producer_.def("IsConnected", &eudaq::Producer::IsConnected);
  producer_.def("GetConfiguration", &eudaq::Producer::GetConfiguration);
  producer_.def("GetInitConfiguration", &eudaq::Producer::GetInitConfiguration);
//  producer_.def("GetConfigKeys",[](const eudaq::Producer &p){return p.GetConfiguration()->Keylist();});
//  producer_.def("GetConfigItem", &eudaq::Producer::GetConfigItem,
//		"Get an item from Producer's config section", py::arg("key"));
//  producer_.def("GetInitItem", &eudaq::Producer::GetInitItem,
//		"Get an item from Producer's init section", py::arg("key")
//		);
//
}
