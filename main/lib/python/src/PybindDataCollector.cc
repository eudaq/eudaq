#include "pybind11/pybind11.h"
#include "eudaq/DataCollector.hh"

namespace py = pybind11;

class PyDataCollector : public eudaq::DataCollector {
public:
  using eudaq::DataCollector::DataCollector;
   
  void DoInitialise() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoInitialise
		      );
  }
  void DoConfigure() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoConfigure
		      );
  }
  void DoStartRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoStartRun
		      );
  }
  void DoStopRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoStopRun
		      );
  }
  void DoReset() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoReset
		      );
  }
  void DoTerminate() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoTerminate
		      );
  }
  void DoConnect(eudaq::ConnectionSPC id) override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      DoConnect,
		      id
		      );
  }
  void DoDisconnect(eudaq::ConnectionSPC id) override {
    PYBIND11_OVERLOAD(void, /* Return type */
  		      eudaq::DataCollector,
  		      DoDisconnect,
  		      id
  		      );
  }
  void DoReceive(eudaq::ConnectionSPC id, eudaq::EventSP ev) override {
    PYBIND11_OVERLOAD(void, /* Return type */
  		      eudaq::DataCollector,
  		      DoReceive,
  		      id,
  		      ev
  		      );
  }
  
  void Exec() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::DataCollector,
		      Exec
		      );
  }
 
};

void init_pybind_datacollector(py::module &m){
  py::class_<eudaq::DataCollector, PyDataCollector, std::shared_ptr<eudaq::DataCollector>>
    datacollector_(m, "DataCollector");
  datacollector_.def(py::init<const std::string&, const std::string&>());
  datacollector_.def("Exec", &eudaq::DataCollector::Exec);
  datacollector_.def("DoInitialise", &eudaq::DataCollector::DoInitialise);
  datacollector_.def("DoConfigure", &eudaq::DataCollector::DoConfigure);
  datacollector_.def("DoStartRun", &eudaq::DataCollector::DoStartRun);
  datacollector_.def("DoStopRun", &eudaq::DataCollector::DoStopRun);
  datacollector_.def("DoReset", &eudaq::DataCollector::DoReset);
  datacollector_.def("DoTerminate", &eudaq::DataCollector::DoTerminate);
  datacollector_.def("DoConnect", &eudaq::DataCollector::DoConnect,
		     "Called when a producer is connecting", py::arg("id"));
  datacollector_.def("DoDisconnect", &eudaq::DataCollector::DoDisconnect,
		     "Called when a producer is disconnecting", py::arg("id"));
  datacollector_.def("DoReceive", &eudaq::DataCollector::DoReceive,
		     "Called when an event is recievied", py::arg("id"), py::arg("ev"));

}
