#include "pybind11/pybind11.h"
#include "eudaq/Status.hh"

namespace py = pybind11;

class PybindStatus : public eudaq::Status {
public:
  using eudaq::Status::Status;
  void Print(std::ostream& os, size_t offset) const override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Status,
		      Print,
		      os,
		      offset
		      );
  }  
};

void  init_pybind_status(py::module &m){
  py::class_<eudaq::Status, eudaq::StatusSP> status_(m, "Status");
  py::enum_<eudaq::Status::State>(status_, "State")
    .value("STATE_ERROR", eudaq::Status::State::STATE_ERROR)
    .value("STATE_UNINIT", eudaq::Status::State::STATE_UNINIT)
    .value("STATE_UNCONF", eudaq::Status::State::STATE_UNCONF)
    .value("STATE_CONF", eudaq::Status::State::STATE_CONF)
    .value("STATE_RUNNING", eudaq::Status::State::STATE_RUNNING)
    .export_values();
  
  status_.def("__repr__",
	     [](const eudaq::StatusSP st){
	       std::ostringstream oss;
	       st->Print(oss);
	       return oss.str();
	     });
  status_.def("GetState", &eudaq::Status::GetState);
  status_.def("GetMessage", &eudaq::Status::GetMessage);
  status_.def("SetTag", &eudaq::Status::SetTag,
	      "Set a tag", py::arg("key"), py::arg("val"));
  status_.def("SetTag", &eudaq::Status::SetTag,
	      "Set a tag", py::arg("key"), py::arg("val"));
  status_.def("GetTag", &eudaq::Status::GetTag,
	      "Get a tag", py::arg("key"), py::arg("defval") = "");
  status_.def("GetTags", &eudaq::Status::GetTags);
}
