#include "eudaq/FileWriter.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

namespace eudaq {

  class FileWriterTextTimeStamps : public FileWriter {
    public:
      FileWriterTextTimeStamps(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual uint64_t FileBytes() const;
      virtual ~FileWriterTextTimeStamps();
    private:
    //  std::FILE * m_file;
	  std::ofstream *m_out;
	  bool firstEvent;
	  vector<uint64_t> m_start_time;
  };

  namespace {
    static RegisterFileWriter<FileWriterTextTimeStamps> reg("textTS");
  }

  FileWriterTextTimeStamps::FileWriterTextTimeStamps(const std::string & param)
    :firstEvent(false),m_out(nullptr)
  {
    std::cout << "EUDAQ_DEBUG: This is FileWriterTextTimeStamps::FileWriterTextTimeStamps("<< param <<")"<< std::endl;
  }

  void FileWriterTextTimeStamps::StartRun(unsigned runnumber) {
    std::cout << "EUDAQ_DEBUG: FileWriterTextTimeStamps::StartRun("<< runnumber << ")" << std::endl;
    // close an open file
	if (m_out)
	{
		m_out->close();
		m_out=nullptr;

	}
	


    // open a new file
    std::string fname(FileNamer(m_filepattern).Set('X', ".txt").Set('R', runnumber));
	m_out=new std::ofstream(fname.c_str());
    
    if (!m_out) EUDAQ_THROW("Error opening file: " + fname);
  }

  void FileWriterTextTimeStamps::WriteEvent(const DetectorEvent & devent) {
	  

	  if (devent.IsBORE()) {
		  eudaq::PluginManager::Initialize(devent);
		  firstEvent =true;
		  return;
	  } else if (devent.IsEORE()) {
		return;
	  }
	 


	  if (firstEvent)
	  {

		  for (size_t i = 0; i < devent.NumEvents();++i)
		  {
			  auto ts = devent.GetEventPtr(i)->GetTimestamp();
			  m_start_time.push_back(ts);

		  }
		 
		  firstEvent=false;
	  }
	
	  for (size_t i = 0; i < devent.NumEvents(); ++i)
	  {
		  auto ts = devent.GetEventPtr(i)->GetTimestamp();
		  *m_out << ts - m_start_time.at(i) << "; " ;

	  }
	  *m_out << std::endl;

	  
  }

  FileWriterTextTimeStamps::~FileWriterTextTimeStamps() {
    if (m_out) {
		m_out->close();
      
      m_out=nullptr;
    }
  }

  uint64_t FileWriterTextTimeStamps::FileBytes() const { return m_out->tellp(); }

}
