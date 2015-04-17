

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include <iostream>
#include <fstream>



namespace eudaq {

  class FileWriterMeta : public FileWriter {
    public:
		FileWriterMeta(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual uint64_t FileBytes() const;
	  virtual ~FileWriterMeta();
    private:
		std::ofstream *m_out;
		bool firstEvent;
		uint64_t DUT_start_time;
      
      
  };

 
  
  registerFileWriter(FileWriterMeta, "meta");

  FileWriterMeta::FileWriterMeta(const std::string & /*param*/)  {
  }

  void FileWriterMeta::StartRun(unsigned runnumber) {
    EUDAQ_INFO("Converting the inputfile into a TTree " );
    std::string foutput(FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);

	m_out = new std::ofstream(foutput);


  }

  void FileWriterMeta::WriteEvent(const DetectorEvent & ev) {
    if (ev.IsBORE()) {
      eudaq::PluginManager::Initialize(ev);
      return;
    } else if (ev.IsEORE()) {
    
    }
	for (size_t i = 0; i<ev.NumEvents();++i)
   {
		auto rof = ev.GetEvent(i);

		if (rof)
		{
      *m_out << Event::id2str(rof->get_id()) << "  " << rof->GetSubType() << " <timeStamps>";
			for (size_t j = 0; j < rof->GetSizeOfTimeStamps();++j)
				{
				*m_out << rof->GetTimestamp(j) << " ";
				}
				
      *m_out << "</timestamps>" << rof->GetTag("eventID") << "||";
		}
		else
		{
      *m_out <<Event::id2str(rof->get_id()) << "  " << rof->GetSubType() << " " << rof->GetTimestamp() << "  " << rof->GetTag("eventID") << "||";
		}
		
   }
	*m_out << std::endl;
  }

  FileWriterMeta::~FileWriterMeta(){


  }

  uint64_t FileWriterMeta::FileBytes() const { return 0; }

}
