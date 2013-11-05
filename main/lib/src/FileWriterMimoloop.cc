#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"

#include <fstream>

namespace eudaq {

  class FileWriterMimoloop : public FileWriter {
    public:
      FileWriterMimoloop(const std::string &);
      virtual void StartRun(unsigned);
      virtual void WriteEvent(const DetectorEvent &);
      virtual unsigned long long FileBytes() const;
      virtual ~FileWriterMimoloop();
    private:
      std::ofstream * m_file;
  };

  namespace {
    static RegisterFileWriter<FileWriterMimoloop> reg("mimoloop");
  }

  FileWriterMimoloop::FileWriterMimoloop(const std::string & /*param*/) : m_file(0) {
    //EUDAQ_DEBUG("Constructing FileWriterNative(" + to_string(param) + ")");
  }

  void FileWriterMimoloop::StartRun(unsigned runnumber) {
    delete m_file;
    m_file = new std::ofstream(std::string(FileNamer(m_filepattern).Set('X', ".txt").Set('R', runnumber)).c_str());
  }

  void FileWriterMimoloop::WriteEvent(const DetectorEvent & dev) {
    if (!m_file) EUDAQ_THROW("FileWriterNative: Attempt to write unopened file");
    //std::cout << "Event " << dev.GetRunNumber() << "." << dev.GetEventNumber() << std::endl;
    for (size_t i = 0; i < dev.NumEvents(); ++i) {
      const RawDataEvent * ev = dynamic_cast<const RawDataEvent *>(dev.GetEvent(i));
      if (!ev || ev->GetSubType() != "EUDRB") continue;
      //std::cout << " EUDRB " << ev->NumBlocks() << " boards" << std::endl;
      for (size_t j = 0; j < ev->NumBlocks(); ++j) {
        const std::vector<unsigned char> & alldata = ev->GetBlock(j);
        //std::cout << "  board " << j << ", (" << ev->GetID(j) << ") " << alldata.size() << " bytes" << std::endl;
        if (alldata.size() < 4) break;
        //std::cout << "  BaseAddress: " << to_hex(getbigendian<unsigned>(&alldata[0]) & 0xff000000 | 0x00400000) << std::endl;
        (*m_file) << "BaseAddress: " << to_hex((getbigendian<unsigned>(&alldata[0]) & 0xff000000) | 0x00400000) << std::endl;
        for (size_t k = 0; k < alldata.size()-3; k += 4) {
          (*m_file) << to_hex(k/4+1) << " : " << to_hex(getbigendian<unsigned>(&alldata[k]), 0) << std::endl;
        }
      }
    }
  }

  FileWriterMimoloop::~FileWriterMimoloop() {
    delete m_file;
  }

  unsigned long long FileWriterMimoloop::FileBytes() const { if (!m_file) return 0; return m_file->tellp(); }

}
