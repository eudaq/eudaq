#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileSerializer.hh"

class NativeFileWriter : public eudaq::FileWriter {
public:
  NativeFileWriter(const std::string &patt);
  NativeFileWriter(const std::string &patt, const std::string &filename);
  void WriteEvent(eudaq::EventSPC ev) override;
  uint64_t FileBytes() const override;
private:
  std::unique_ptr<eudaq::FileSerializer> m_ser;
  std::string m_filepattern;
  std::string m_filename;
  bool m_filename_passed;
  uint32_t m_run_n;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::FileWriter>::
    Register<NativeFileWriter, std::string&>(eudaq::cstr2hash("native"));
  auto dummy1 = eudaq::Factory<eudaq::FileWriter>::
    Register<NativeFileWriter, std::string&&>(eudaq::cstr2hash("native"));

  auto dummy2 = eudaq::Factory<eudaq::FileWriter>::
    Register<NativeFileWriter, std::string&, std::string &>(eudaq::cstr2hash("native"));
  auto dummy3 = eudaq::Factory<eudaq::FileWriter>::
    Register<NativeFileWriter, std::string&&, std::string &&>(eudaq::cstr2hash("native"));
}

NativeFileWriter::NativeFileWriter(const std::string &patt){
  m_filepattern = patt;
  m_filename_passed = false;
}

NativeFileWriter::NativeFileWriter(const std::string &patt, const std::string &filename){
  m_filepattern = patt;
  m_filename = filename;
  m_filename_passed = true;
}
  
void NativeFileWriter::WriteEvent(eudaq::EventSPC ev) {
  uint32_t run_n = ev->GetRunN();
  if (m_filename_passed && (!m_ser || m_run_n != run_n)){
    m_ser.reset(new eudaq::FileSerializer(m_filename));
    m_run_n = run_n;
  }
  if(!m_ser || m_run_n != run_n){
    std::time_t time_now = std::time(nullptr);
    char time_buff[13];
    time_buff[12] = 0;
    std::strftime(time_buff, sizeof(time_buff),
		  "%y%m%d%H%M%S", std::localtime(&time_now));
    std::string time_str(time_buff);
    m_ser.reset(new eudaq::FileSerializer((eudaq::FileNamer(m_filepattern).
					   Set('X', ".raw").
					   Set('R', run_n).
					   Set('D', time_str))));
    m_run_n = run_n;
  }
  if(!m_ser)
    EUDAQ_THROW("NativeFileWriter: Attempt to write unopened file");
  m_ser->write(*(ev.get())); //TODO: Serializer accepts EventSPC
  m_ser->Flush();
}
  
uint64_t NativeFileWriter::FileBytes() const {
  return m_ser ?m_ser->FileBytes() :0;
}
