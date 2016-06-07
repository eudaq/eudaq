#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>
#include <fstream>



namespace eudaq {

class dump2file {
public:
  dump2file(const char* name,const double* value_) :m_value(value_),m_name(name) {}

  void fillHeader(std::ostream& out) {
    out << m_name;
  }
  void fill(std::ostream& out) {
    out << *m_value;
  }
private:
 const double* m_value = nullptr;
  std::string m_name;
  
};
class dumphub {
public:
  
  void push_back(const char* name, const double* value) {
    m_values.emplace_back(name, value);
  }
  void fillHeader(std::ostream& out) {

    for (size_t i = 0; i < m_values.size() - 1; ++i) {

      m_values[i].fillHeader(out);
      out << " ; ";
    }
    m_values.back().fillHeader(out);
    out << "\n";
  }
  void fill(std::ostream& out) {
    
    for (size_t i = 0; i < m_values.size()-1;++i)
    {
      
      m_values[i].fill(out);
      out << " ; ";
    }
    m_values.back().fill(out);
    out << "\n";
  }
private:
  std::vector<dump2file> m_values;
};
  class FileWriterTextCompact : public FileWriter {
  public:
    FileWriterTextCompact(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterTextCompact();
  private:
    //  std::FILE * m_file;
    dumphub m_dumpHub;

    double m_tlu_time_stamp = 0,
      m_TLU_event_nr = 0,
      m_tlu_trigger = 0,
      m_DUT_TDC = 0,
      m_dut_timestamp = 0,
      m_dut_timestamp_l0id = 0,
      m_dut_event_nr = 0,
      m_dut_TLU_ID = 0,
      m_dut_TLU_L0ID = 0,
      m_dut_Event_L0ID = 0,
      m_dut_createdAtTime = 0,
      m_dut_sendAtTime =0;

    double DUT_start_time = 0;


    std::ofstream *m_out=nullptr;
    bool firstEvent=false;
    uint64_t TLU_start_Time=0;
  };

  
  registerFileWriter(FileWriterTextCompact, "textc");
  
  FileWriterTextCompact::FileWriterTextCompact(const std::string & param)
  {
    std::cout << "EUDAQ_DEBUG: This is FileWriterTextCompact::FileWriterTextCompact(" << param << ")" << std::endl;

    m_dumpHub.push_back("tlu_event_nr", &m_TLU_event_nr);
    m_dumpHub.push_back("DUT_TLU_ID_READBACK", &m_dut_TLU_ID);
   // m_dumpHub.push_back("DUT_TLU_L0ID", &m_dut_TLU_L0ID);
    
   // m_dumpHub.push_back("DUT_Event_L0ID", &m_dut_Event_L0ID);
    
    m_dumpHub.push_back("TimeStamp_L0ID", &m_dut_timestamp_l0id);
    m_dumpHub.push_back("DUT_EVENT_nr", &m_dut_event_nr);
    m_dumpHub.push_back("tlu_time_stamp",&m_tlu_time_stamp);
    m_dumpHub.push_back("DUT_TDC", &m_DUT_TDC);
    m_dumpHub.push_back("DUT_TimeStamp", &m_dut_timestamp);
    m_dumpHub.push_back("tlu_trigger", &m_tlu_trigger);
    m_dumpHub.push_back("createdAtTime", &m_dut_createdAtTime);
    m_dumpHub.push_back("sendAtTime", &m_dut_sendAtTime);
  
  }

  void FileWriterTextCompact::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterTextCompact::StartRun(" << runnumber << ")" << std::endl;
    // close an open file
    if (m_out)
    {
      m_out->close();
      delete m_out;
      m_out = nullptr;

    }



    // open a new file
    std::string fname(FileNamer(m_filepattern).Set('X', ".txt").Set('R', runnumber));
    m_out = new std::ofstream(fname.c_str());

    if (!m_out) EUDAQ_THROW("Error opening file: " + fname);
  }

  void FileWriterTextCompact::WriteEvent(const DetectorEvent & devent) {


    if (devent.IsBORE()) {
      eudaq::PluginManager::Initialize(devent);
      firstEvent = true;
    
      return;
    }
    else if (devent.IsEORE()) {
      return;
    }
    StandardEvent sev = eudaq::PluginManager::ConvertToStandard(devent);

    if (firstEvent)
    {



      m_dumpHub.fillHeader(*m_out);
      DUT_start_time = sev.GetTag("Timestamp.data", double(0));
      TLU_start_Time = sev.GetTimestamp();
      firstEvent = false;
    }

    m_tlu_time_stamp = static_cast<double>(sev.GetTimestamp() - TLU_start_Time);
    m_TLU_event_nr = sev.GetTag("TLU_event_nr", (double)0);
    m_tlu_trigger = sev.GetTag("TLU_trigger", 0);
    m_DUT_TDC = sev.GetTag("TDC.data", double(0));
    m_dut_timestamp = sev.GetTag("Timestamp.data", double(0)) - DUT_start_time;
    m_dut_event_nr = sev.GetTag("SCT_EVENT_NR", double(0));
    m_dut_timestamp_l0id = sev.GetTag("Timestamp.L0ID", double(0));
    m_dut_TLU_ID = sev.GetTag("TLU.TLUID", double(0));
    m_dut_TLU_L0ID = sev.GetTag("TDC.L0ID", double(0));
    m_dut_Event_L0ID = sev.GetTag("Event.L0ID", double(0));
    m_dut_createdAtTime = sev.GetTag("createdAtTime", double(0));
    m_dut_sendAtTime = sev.GetTag("sendAtTime", double(0));
    m_dumpHub.fill(*m_out);
  }

  FileWriterTextCompact::~FileWriterTextCompact() {
    if (m_out) {
      m_out->close();
      delete m_out;
      m_out = nullptr;
    }
  }

  uint64_t FileWriterTextCompact::FileBytes() const { return m_out->tellp(); }

}
