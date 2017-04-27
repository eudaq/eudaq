#ifndef ProcessorFileReader_h__
#define ProcessorFileReader_h__
#include "eudaq/baseFileReader.hh"
#include "Processor_add2queue.hh"
#include "fileName.hh"
namespace eudaq{
  class DLLEXPORT ProcessorFileReader : public Processor_add2queue
  {
  public:
    ProcessorFileReader(const fileConfig & op);
    void setFileName(const fileName& fName);
    virtual ReturnParam add2queue(event_sp& ev);
    virtual void initialize() override;
    virtual void end() override;
  private:

    FileReader_up m_reader;
    bool m_first = true;
    fileConfig m_fName;

  };
}
#endif // ProcessorFileReader_h__
