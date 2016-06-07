#include "eudaq/FileWriter.hh"
#include <iostream>

#include "ProcessorFileWriter.hh"
#include "Processors.hh"

namespace eudaq{
  using ReturnParam = ProcessorBase::ReturnParam;

  Processors::processor_i_up Processors::fileWriter() {
    return __make_unique<ProcessorFileWriter>();
  }
  Processors::processor_i_up fileWriter(const std::string& name, const std::string& param_/*=""*/) {
    return __make_unique<ProcessorFileWriter>(name, param_);
  }

  ProcessorFileWriter::ProcessorFileWriter(const std::string & name, const std::string & params /*= ""*/):m_default(false),m_name(name),m_params(params)  {

  }

  ProcessorFileWriter::ProcessorFileWriter() : m_default(true) {

  }



  void ProcessorFileWriter::init() {
    if (m_default||m_name.empty()) {
      m_write = FileWriterFactory::Create();
    } else {
      m_write = FileWriterFactory::Create(m_name, m_params);
    }
    m_write->SetFilePattern(m_pattern);
  }



  void ProcessorFileWriter::end() {
    m_first = true;
  }


 



  ReturnParam ProcessorFileWriter::inspectEvent(const Event& ev, ConnectionName_ref con) {

    if (m_first&&m_write) {
      m_first = false;
      m_write->StartRun(ev.GetRunNumber());
    }

    try {
      m_write->WriteBaseEvent(ev);
    } catch (...) {
      return ProcessorBase::stop;
    }


    return sucess;
  }

  uint64_t ProcessorFileWriter::FileBytes() const {
    if (!m_write)
    {
      return 0;
    }
    return m_write->FileBytes();
  }

  void ProcessorFileWriter::SetFilePattern(const std::string & p) {
    m_pattern = p;
  }


}
