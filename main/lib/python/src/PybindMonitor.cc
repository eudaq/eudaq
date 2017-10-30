#include "pybind11/pybind11.h"
#include "eudaq/Monitor.hh"

namespace py = pybind11;

class PyMonitor : public eudaq::Monitor{
public:
  using eudaq::Monitor::Monitor;
  void DoInitialise() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoInitialise
		      );
  }
  void DoConfigure() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoConfigure
		      );
  }
  void DoStartRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoStartRun
		      );
  }
  void DoStopRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoStopRun
		      );
  }
  void DoReset() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoReset
		      );
  }
  void DoTerminate() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoTerminate
		      );
  }
  void DoReceive(eudaq::EventSP ev) override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Monitor,
		      DoReceive,
		      ev
		      );
  }
};


void init_pybind_monitor(py::module &m){
  py::class_<eudaq::Monitor, PyMonitor, std::shared_ptr<eudaq::Monitor>>
    monitor_(m, "Monitor");
  monitor_.def(py::init<const std::string&, const std::string&>());
  monitor_.def("DoInitialise", &eudaq::Monitor::DoInitialise);
  monitor_.def("DoConfigure", &eudaq::Monitor::DoConfigure);
  monitor_.def("DoStartRun", &eudaq::Monitor::DoStartRun);
  monitor_.def("DoStopRun", &eudaq::Monitor::DoStopRun);
  monitor_.def("DoReset", &eudaq::Monitor::DoReset);
  monitor_.def("DoTerminate", &eudaq::Monitor::DoTerminate);
  monitor_.def("DoReceive", &eudaq::Monitor::DoReceive,
	       "Called when an event is recievied", py::arg("ev"));
  monitor_.def("SetServerAddress", &eudaq::Monitor::SetServerAddress,
	       "Set port of the data listening", py::arg("addr"));
  monitor_.def("Connect", &eudaq::Monitor::Connect);

}
