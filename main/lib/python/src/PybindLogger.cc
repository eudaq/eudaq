#include "pybind11/pybind11.h"
#include "eudaq/Logger.hh"

namespace py = pybind11;

void PyEUDAQ_DEBUG(const std::string &msg) {
  EUDAQ_DEBUG(msg);
}

void PyEUDAQ_EXTRA(const std::string &msg) {
  EUDAQ_EXTRA(msg);
}

void PyEUDAQ_INFO(const std::string &msg) {
  EUDAQ_INFO(msg);
}

void PyEUDAQ_WARN(const std::string &msg) {
  EUDAQ_WARN(msg);
}

void PyEUDAQ_ERROR(const std::string &msg) {
  EUDAQ_ERROR(msg);
}

void PyEUDAQ_USER(const std::string &msg) {
  EUDAQ_USER(msg);
}

void init_pybind_logger(py::module &m){
  m.def("EUDAQ_DEBUG", &PyEUDAQ_DEBUG);
  m.def("EUDAQ_EXTRA", &PyEUDAQ_EXTRA);
  m.def("EUDAQ_INFO",  &PyEUDAQ_INFO);
  m.def("EUDAQ_WARN",  &PyEUDAQ_WARN);
  m.def("EUDAQ_ERROR", &PyEUDAQ_ERROR);
  m.def("EUDAQ_USER",  &PyEUDAQ_USER);
}
