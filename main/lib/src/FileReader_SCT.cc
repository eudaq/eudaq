#include <list>
#include "eudaq/baseFileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/factory.hh"


const std::string EventID = "SCT";
const unsigned UnsetVariable = (unsigned) -1;
namespace eudaq {



  uint64_t hex2uint_64(const std::string& hex_string){

    auto ts = "0x" + hex_string;
    return std::stoull(ts, nullptr, 16);
  }

  class DLLEXPORT FileReaderSCT : public baseFileReader {
  public:
    using event_p = std::shared_ptr < eudaq::Event > ;
    using rawEvent_p = std::shared_ptr < eudaq::RawDataEvent > ;
    using detEvent_p = std::shared_ptr < eudaq::DetectorEvent > ;
    FileReaderSCT(const std::string & filename, const std::string & filepattern = "");

    FileReaderSCT(Parameter_ref param);


    ~FileReaderSCT();
    virtual  unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual std::shared_ptr<eudaq::Event> getEventPtr() { return m_ev; }
    virtual std::shared_ptr<eudaq::Event> GetNextEvent();
    virtual void Interrupt() { }



    const eudaq::Event & GetEvent() const;
    const DetectorEvent & Event() const { return GetDetectorEvent(); } // for backward compatibility
    const DetectorEvent & GetDetectorEvent() const;
    const StandardEvent & GetStandardEvent() const;
    std::shared_ptr<eudaq::DetectorEvent> GetDetectorEvent_ptr(){ return std::dynamic_pointer_cast<eudaq::DetectorEvent>(m_ev); };

  private:

    void process_event(){
      event_process_ODD_line();
      event_process_even_lines();

    }
    void event_process_ODD_line();
    void event_process_even_lines();
    void process_trigger(){
      Trigger_process_TDC();
      Trigger_process_TLU();
      Trigger_process_Timestamps();
    }
    void Trigger_process_TDC();
    void Trigger_process_TLU();
    void Trigger_process_Timestamps();
    void checkForEventNumberMissmatch();
    detEvent_p m_ev;
    rawEvent_p m_raw_ev;
    unsigned m_ver, m_eventNumber = 0, m_runNum = 0;
    size_t m_subevent_counter = 0;
    std::string m_triggerFileName;
    std::unique_ptr<std::ifstream> m_eventFile, m_triggerFile;

  };




  FileReaderSCT::FileReaderSCT(const std::string & file, const std::string & filepattern)
    : baseFileReader(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
    m_ver(1)
  {
    event_p dummy = std::make_shared<eudaq::RawDataEvent>(RawDataEvent::BORE(EventID, 0));

    m_ev = std::make_shared<eudaq::DetectorEvent>(0, 0, 0);
    m_ev->AddEvent(dummy);

    auto name = baseFileReader::Filename();

    auto splittedName = split(name, "_.");

    auto filePathIndex = name.find_last_of("\\/");
    auto filepath = name.substr(0, filePathIndex);
    std::string triggername("TriggerData_");
    m_triggerFileName = filepath + "/" + triggername + splittedName.at(splittedName.size() - 3) + "_" + splittedName.at(splittedName.size() - 2) + ".dat";
    m_eventFile = std::unique_ptr<std::ifstream>(new std::ifstream(name));
    m_triggerFile = std::unique_ptr<std::ifstream>(new std::ifstream(m_triggerFileName));

  }

  FileReaderSCT::FileReaderSCT(Parameter_ref param) :FileReaderSCT(param.Get(getKeyFileName(), ""), param.Get(getKeyFileName(),""))
  {
    std::cout << param.Get("offset",(int)-1) << std::endl;
  }

  FileReaderSCT::~FileReaderSCT() {

  }

  bool FileReaderSCT::NextEvent(size_t skip) {



    m_raw_ev = std::make_shared<RawDataEvent>(EventID, 0, m_eventNumber);

    bool result = true;// m_des.ReadEvent(m_ver, ev, skip);



    if (
      !m_eventFile->eof()
      &&
      !m_triggerFile->eof()
      )
    {
      try{
        process_event();
        process_trigger();

        m_ev = std::make_shared<DetectorEvent>(m_runNum, m_eventNumber++, m_raw_ev->GetTimestamp());

        m_ev->AddEvent(std::dynamic_pointer_cast<eudaq::Event>(m_raw_ev));


      }
      catch (...){
#ifdef _DEBUG
        std::cout << "unexpected end of file " << std::endl;
#endif //_DEBUG
        m_ev = nullptr;
        return false;
      }
#ifdef _DEBUG
   //   checkForEventNumberMissmatch();
#endif
      return true;
    }
    else{
      m_ev = nullptr;
      
#ifdef _DEBUG
      std::cout << "end of file" << std::endl;
#endif //_DEBUG
      return false;
    }
    
  }

  unsigned FileReaderSCT::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const Event & FileReaderSCT::GetEvent() const {
    return *m_ev;
  }

  const DetectorEvent & FileReaderSCT::GetDetectorEvent() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

  const StandardEvent & FileReaderSCT::GetStandardEvent() const {
    return dynamic_cast<const StandardEvent &>(*m_ev);
  }

  std::shared_ptr<eudaq::Event> FileReaderSCT::GetNextEvent(){

    if (!NextEvent()) {
      return nullptr;
    }

    return m_ev;


  }

  void FileReaderSCT::event_process_ODD_line()
  {
    std::string line;
    std::getline(*m_eventFile, line);
    if (line.empty())
    {
      EUDAQ_THROW("unexpected end of file");
    }
    auto splittedline = split(line, " ");
    m_raw_ev->SetTag("L0ID", hex2uint_64(splittedline.at(5)));
    m_raw_ev->SetTag("BCID", hex2uint_64(splittedline.at(7)));
  }

  void FileReaderSCT::event_process_even_lines()
  {
    std::string line;
    std::getline(*m_eventFile, line);
    if (line.empty())
    {
      EUDAQ_THROW("unexpected end of file");
    }

    std::vector<unsigned char> data; //this needs to be replaced with a bool in future
    for (auto& e : line)
    {
      if (e == '1')
      {
        data.push_back(1);
      }
      else if (e == '0')
      {
        data.push_back(0);
      }

    }

    m_raw_ev->AddBlock(0, data);
  }

  void FileReaderSCT::Trigger_process_TDC()
  {
    std::string line;
    std::getline(*m_triggerFile, line);
    if (line.empty())
    {
      EUDAQ_THROW("unexpected end of file");
    }
    auto splittedline = split(line, " ");
    m_raw_ev->SetTag("tdc", hex2uint_64(*(splittedline.end() - 1)));
    m_raw_ev->SetTag("tdc_ev_num", hex2uint_64(*(splittedline.end() - 2)));

  }

  void FileReaderSCT::Trigger_process_TLU()
  {
    std::string line;
    std::getline(*m_triggerFile, line);
    if (line.empty())
    {
      EUDAQ_THROW("unexpected end of file");
    }
    auto splittedline = split(line, " ");
    m_raw_ev->SetTag("TLUID", hex2uint_64(*(splittedline.end() - 1)));
    m_raw_ev->SetTag("TLU_ev_num", hex2uint_64(*(splittedline.end() - 3)));
  }

  void FileReaderSCT::Trigger_process_Timestamps()
  {
    std::string line;
    std::getline(*m_triggerFile, line);
    if (line.empty())
    {
      EUDAQ_THROW("unexpected end of file");
    }
    auto splittedline = split(line, " ");
    m_raw_ev->SetTag("TS_data", hex2uint_64(*(splittedline.end() - 1)));
    m_raw_ev->SetTag("TS_ev_num", hex2uint_64(*(splittedline.end() - 2)));
    m_raw_ev->setTimeStamp(hex2uint_64(splittedline.back()));
  }

  void FileReaderSCT::checkForEventNumberMissmatch()
  {
    if (m_raw_ev->GetTag("tdc_ev_num", UnsetVariable) != m_raw_ev->GetEventNumber()
        ||
        m_raw_ev->GetTag("TLU_ev_num", UnsetVariable) != m_raw_ev->GetEventNumber()
        ||
        m_raw_ev->GetTag("TS_ev_num", UnsetVariable) != m_raw_ev->GetEventNumber()
        )
    {
      EUDAQ_THROW("event number missmatch");

    }
  }





  RegisterFileReader(FileReaderSCT, "sct");

}
