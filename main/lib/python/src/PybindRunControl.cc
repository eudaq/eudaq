#include "pybind11/pybind11.h"
#include "eudaq/RunControl.hh"

namespace py = pybind11;

class PyRunControl : public eudaq::RunControl{
public:
  using eudaq::RunControl::RunControl;

  void Initialise() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      Initialise
		      );
  }
  void Configure() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      Configure
		      );
  }
  void StartRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      StartRun
		      );
  }
  void StopRun() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      StopRun
		      );
  }
  void Reset() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      Reset
		      );
  }
  void Terminate() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      Terminate
		      );
  }
  void DoConnect(eudaq::ConnectionSPC id) override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      DoConnect,
		      id
		      );
  }
  void DoDisconnect(eudaq::ConnectionSPC id) override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      DoDisconnect,
		      id
		      );
  }
  void DoStatus(eudaq::ConnectionSPC id, eudaq::StatusSPC st) override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      DoStatus,
		      id,
		      st
		      );
  }
  void Exec() override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::RunControl,
		      Exec
		      );
  }

};


void init_pybind_runcontrol(py::module &m){
  py::class_<eudaq::RunControl, PyRunControl, std::shared_ptr<eudaq::RunControl>>
    runcontrol_(m, "RunControl");
  runcontrol_.def(py::init<const std::string&>());
  runcontrol_.def("Exec", &eudaq::RunControl::Exec);
  runcontrol_.def("Initialise", &eudaq::RunControl::Initialise);
  runcontrol_.def("Configure", &eudaq::RunControl::Configure);
  runcontrol_.def("StartRun", &eudaq::RunControl::StartRun);
  runcontrol_.def("StopRun", &eudaq::RunControl::StopRun);
  runcontrol_.def("Reset", &eudaq::RunControl::Reset);
  runcontrol_.def("Terminate", &eudaq::RunControl::Terminate);
  runcontrol_.def("DoConnect", &eudaq::RunControl::DoConnect,
		  "Called when a client is connecting", py::arg("id"));
  runcontrol_.def("DoDisconnect", &eudaq::RunControl::DoDisconnect,
		  "Called when a client is disconnecting", py::arg("id"));
  runcontrol_.def("DoStatus", &eudaq::RunControl::DoStatus,
		  "Called when a status object is recievied", py::arg("id"), py::arg("st"));
}
