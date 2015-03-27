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

  
  registerFileWriter(FileWriterTextTimeStamps, "textTS");
  
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
    delete m_out;
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
        for (size_t j = 0; j< PluginManager::GetTimeStamp_size(*devent.GetEvent(i));++j)
        {

          auto ts = PluginManager::GetTimeStamp(*devent.GetEvent(i),j);
			    m_start_time.push_back(ts);
          *m_out << Event::id2str(devent.GetEventPtr(i)->get_id()) << "_" << devent.GetEventPtr(i)->GetSubType() <<"_TS_" << j<< "; ";

        }
			  
        if (PluginManager::isTLU(*devent.GetEvent(i)))
        {
          *m_out << "TLU_trigger; ";
        }
		  }
      *m_out << '\n';
		  firstEvent=false;
	  }
	
	  for (size_t i = 0; i < devent.NumEvents(); ++i)
	  {
      for (size_t j = 0; j< PluginManager::GetTimeStamp_size(*devent.GetEvent(i)); ++j)
      {
        auto ts = PluginManager::GetTimeStamp(*devent.GetEvent(i), j);
        *m_out << ts - m_start_time.at(i+j) << "; ";
      }

      if (PluginManager::isTLU(*devent.GetEvent(i)))
      {
        *m_out << devent.GetEvent(i)->GetTag("trigger", 0) << "; ";
      }
	  }
	  *m_out << '\n';

	  
  }

  FileWriterTextTimeStamps::~FileWriterTextTimeStamps() {
    if (m_out) {
		m_out->close();
      delete m_out;
      m_out=nullptr;
    }
  }

  uint64_t FileWriterTextTimeStamps::FileBytes() const { return m_out->tellp(); }

}
