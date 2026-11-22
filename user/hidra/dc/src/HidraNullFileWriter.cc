#include <eudaq/FileWriter.hh>

class HidraNullFileWriter : public eudaq::FileWriter {
public:
  explicit HidraNullFileWriter(const std::string& patt) : eudaq::FileWriter() {}
  void WriteEvent(eudaq::EventSPC /*ev*/) override {}
  uint64_t FileBytes() const override { return 0; }
};

namespace {
auto dummy_fw0 =
    eudaq::Factory<eudaq::FileWriter>::Register<HidraNullFileWriter, std::string&>(eudaq::cstr2hash("HidraNullFileWriter"));
auto dummy_fw1 =
    eudaq::Factory<eudaq::FileWriter>::Register<HidraNullFileWriter, std::string&&>(eudaq::cstr2hash("HidraNullFileWriter"));
}
