#include "pybind11/pybind11.h"
#include "eudaq/Event.hh"

namespace py = pybind11;

class PyEvent : public eudaq::Event {
public:
  using eudaq::Event::Event;
  void Print(std::ostream& os, size_t offset) const override {
    PYBIND11_OVERLOAD(void, /* Return type */
		      eudaq::Event,
		      Print,
		      os,
		      offset
		      );
  }  
};

void  init_pybind_event(py::module &m){
  py::class_<eudaq::Event, PyEvent, eudaq::EventSP> event_(m, "Event");
  py::enum_<eudaq::Event::Flags>(event_, "Flags")
    .value("FLAG_BORE", eudaq::Event::Flags::FLAG_BORE)
    .value("FLAG_EORE", eudaq::Event::Flags::FLAG_EORE)
    .value("FLAG_FAKE", eudaq::Event::Flags::FLAG_FAKE)
    .value("FLAG_PACK", eudaq::Event::Flags::FLAG_PACK)
    .value("FLAG_TRIG", eudaq::Event::Flags::FLAG_TRIG)
    .value("FLAG_TIME", eudaq::Event::Flags::FLAG_TIME)
    .export_values();

  event_.def(py::init(&eudaq::Event::MakeShared));
  event_.def("__repr__",
	     [](const eudaq::EventSP ev){
	       std::ostringstream oss;
	       ev->Print(oss);
	       return oss.str();
	     });
  event_.def("SetTag",
	     (void (eudaq::Event::*)(const std::string&, const std::string&))
	     &eudaq::Event::SetTag,
	     "Set a tag", py::arg("key"), py::arg("val"));
  event_.def("GetTag",
	     (std::string (eudaq::Event::*)(const std::string&, const std::string&) const)
	     &eudaq::Event::GetTag,
	     "Get a tag", py::arg("key"), py::arg("defval") = "");
  event_.def("GetTags", &eudaq::Event::GetTags);
  event_.def("SetFlagBit", &eudaq::Event::SetFlagBit,
	     "Set Flag Bit", py::arg("f"));
  event_.def("ClearFlagBit", &eudaq::Event::ClearFlagBit,
	     "Clear Flag Bit", py::arg("f"));
  event_.def("IsFlagBit", &eudaq::Event::IsFlagBit,
	     "if the flag bit is set", py::arg("f"));
  event_.def("SetBORE", &eudaq::Event::SetBORE);
  event_.def("SetEORE", &eudaq::Event::SetEORE);
  event_.def("SetFlagFake", &eudaq::Event::SetFlagFake);
  event_.def("SetFlagPacket", &eudaq::Event::SetFlagPacket);
  event_.def("SetFlagTimestamp", &eudaq::Event::SetFlagTimestamp);
  event_.def("SetFlagTrigger", &eudaq::Event::SetFlagTrigger);
  event_.def("IsBORE", &eudaq::Event::IsBORE);
  event_.def("IsEORE", &eudaq::Event::IsEORE);
  event_.def("IsFlagFake", &eudaq::Event::IsFlagFake);
  event_.def("IsFlagPacket", &eudaq::Event::IsFlagPacket);
  event_.def("IsFlagTimeStamp", &eudaq::Event::IsFlagTimestamp);
  event_.def("IsFlagTrigger", &eudaq::Event::IsFlagTrigger);
  event_.def("AddSubEvent", &eudaq::Event::AddSubEvent,
	     "Add sub Event", py::arg("ev"));
  event_.def("GetNumSubEvent", &eudaq::Event::GetNumSubEvent);
  event_.def("GetSubEvent",&eudaq::Event::GetSubEvent,
	     "Get sub Event", py::arg("i"));
  event_.def("GetSubEvents", &eudaq::Event::GetSubEvents);
  event_.def("SetType", &eudaq::Event::SetType,
	     "Set Event type", py::arg("id"));
  event_.def("SetVersion", &eudaq::Event::SetVersion,
	     "Set version", py::arg("version"));
  event_.def("SetFlag", &eudaq::Event::SetFlag,
	     "Set version", py::arg("flag"));
  event_.def("SetRunN", &eudaq::Event::SetRunN,
	     "Set run number", py::arg("run"));
  event_.def("SetEventN", &eudaq::Event::SetEventN,
	     "Set Event number", py::arg("num"));
  event_.def("SetDeviceN", &eudaq::Event::SetDeviceN,
	     "Set device number", py::arg("dev"));
  event_.def("SetTriggerN", &eudaq::Event::SetTriggerN,
	     "Set trigger number", py::arg("tg"), py::arg("flag")=true);
  event_.def("SetExtendWord", &eudaq::Event::SetExtendWord,
	     "Set extend word", py::arg("ext"));
  event_.def("SetTimestamp", &eudaq::Event::SetTimestamp,
	     "Set time stamp", py::arg("begin"), py::arg("end"), py::arg("flag")=true);
  event_.def("SetDescription", &eudaq::Event::SetDescription,
	     "Set Description", py::arg("dspt"));
  event_.def("GetDescription", &eudaq::Event::GetDescription);
  event_.def("GetBlock", &eudaq::Event::GetBlock,
	     "Get block", py::arg("n"));
  event_.def("GetNumBlock", &eudaq::Event::GetNumBlock);
  event_.def("GetNumBlockList", &eudaq::Event::GetBlockNumList);
  event_.def("AddBlock",
	     (size_t (eudaq::Event::*)(uint32_t, const std::vector<uint8_t>&))
	     &eudaq::Event::AddBlock<uint8_t>,
	     "Add data block", py::arg("index"), py::arg("data"));
}
