


#include "ProcessorFileReader.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/baseFileReader.hh"
#include "Processors.hh"

namespace eudaq {
using ReturnParam = ProcessorBase::ReturnParam;

Processors::processor_up Processors::fileReader(const fileConfig & op) {
  return __make_unique<ProcessorFileReader>(op);
}


ProcessorFileReader::ProcessorFileReader(const fileConfig & op) : Processor_add2queue(random_connection()),  m_fName(op) {

}

ReturnParam ProcessorFileReader::add2queue(event_sp& ev) {
  ReturnParam ret = sucess;
  if (m_first) {
    ev = m_reader->getEventPtr();
    m_first = false;
    return sucess;
  }

  if (m_reader->NextEvent()) {
    ev = m_reader->getEventPtr();

    return sucess;

  }



  return stop;
}



void ProcessorFileReader::initialize() {
  m_first = true;
  if (!m_reader)
  {
    m_reader = FileReaderFactory::create(m_fName);
  }
}

void ProcessorFileReader::end() {

}

}
