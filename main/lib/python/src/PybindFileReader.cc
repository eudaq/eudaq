#include "pybind11/pybind11.h"
#include "eudaq/FileReader.hh"

namespace py = pybind11;

class PyFileReader : public eudaq::FileReader {
public:
  using eudaq::FileReader::FileReader;
  eudaq::EventSPC GetNextEvent() override {
    PYBIND11_OVERLOAD(eudaq::EventSPC,
		      eudaq::FileReader,
		      GetNextEvent
		      );
  }
};

void init_pybind_filereader(py::module &m){
  py::class_<eudaq::FileReader, PyFileReader, std::shared_ptr<eudaq::FileReader>>
    filereader_(m, "FileReader");
  filereader_.def(py::init(&eudaq::FileReader::Make));
  filereader_.def("GetNextEvent", &eudaq::FileReader::GetNextEvent);
}
