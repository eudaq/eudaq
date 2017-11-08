#include "pybind11/pybind11.h"
#include "eudaq/FileWriter.hh"

namespace py = pybind11;

class PyFileWriter : public eudaq::FileWriter {
public:
  using eudaq::FileWriter::FileWriter;
  void WriteEvent(eudaq::EventSPC ev) override {
    PYBIND11_OVERLOAD(void,
  		      eudaq::FileWriter,
  		      WriteEvent,
		      ev
  		      );
  }
};

void init_pybind_filewriter(py::module &m){
  py::class_<eudaq::FileWriter, PyFileWriter, std::shared_ptr<eudaq::FileWriter>>
    filewriter_(m, "FileWriter");

}
