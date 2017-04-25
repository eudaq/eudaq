#ifndef ProcessorFileWriter_h__
#define ProcessorFileWriter_h__
#include "Processor_inspector.hh"

namespace eudaq {

class FileWriter;
class DLLEXPORT  ProcessorFileWriter :public Processor_Inspector {
public:
  ProcessorFileWriter(const std::string & name, const std::string & params = "");

  ProcessorFileWriter();
  void init() override;
  void end() override;
  virtual ReturnParam inspectEvent(const Event&, ConnectionName_ref con);
   uint64_t FileBytes() const ;
   void SetFilePattern(const std::string & p);
private:
  FileWriter_up m_write;
  bool m_first = true;
  std::string m_name, m_params,m_pattern;
  bool m_default = false;
  unsigned m_runNumber = 0;
};
}


#endif // ProcessorFileWriter_h__
