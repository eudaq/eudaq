#include "pybind11/pybind11.h"

namespace py = pybind11;

void init_pybind_event(py::module &);
void init_pybind_status(py::module &);
void init_pybind_connection(py::module &);
void init_pybind_configuration(py::module &);
void init_pybind_producer(py::module &);
void init_pybind_datacollector(py::module &);
void init_pybind_runcontrol(py::module &);
void init_pybind_monitor(py::module &);
void init_pybind_filereader(py::module &);
void init_pybind_filewriter(py::module &);
void init_pybind_logger(py::module &);

PYBIND11_MODULE(pyeudaq, m){
  m.doc() = "EUDAQ library for Python";
  init_pybind_event(m);
  init_pybind_status(m);
  init_pybind_connection(m);
  init_pybind_producer(m);
  init_pybind_datacollector(m);
  init_pybind_runcontrol(m);
  init_pybind_monitor(m);
  init_pybind_filereader(m);
  init_pybind_filewriter(m);
  init_pybind_configuration(m);
  init_pybind_logger(m);
}
