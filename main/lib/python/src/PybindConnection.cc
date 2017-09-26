#include "pybind11/pybind11.h"
#include "eudaq/TransportBase.hh"

namespace py = pybind11;

class PyConnection : public eudaq::Connection {
public:
  using eudaq::Connection::Connection;
  void Print(std::ostream& os, size_t offset) const override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Connection,
		      Print,
		      os,
		      offset
		      );
  }  
};

void init_pybind_connection(py::module &m){
  py::class_<eudaq::Connection, eudaq::ConnectionSP> connection_(m, "Connection");
  connection_.def("__repr__",
		  [](const eudaq::ConnectionSP id){
		    std::ostringstream oss;
		    id->Print(oss);
		    return oss.str();
		  });
  connection_.def("GetState", &eudaq::Connection::GetState);
  connection_.def("GetType", &eudaq::Connection::GetType);
  connection_.def("GetRemote", &eudaq::Connection::GetRemote);
  connection_.def("GetName", &eudaq::Connection::GetName);
}
