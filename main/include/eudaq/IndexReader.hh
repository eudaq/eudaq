#ifndef EUDAQ_INCLUDED_IndexReader
#define EUDAQ_INCLUDED_IndexReader

#include <string>
#include "eudaq/Platform.hh"

namespace eudaq {

class FileDeserializer;

  class DLLEXPORT IndexReader {
    public:
      IndexReader(const std::string & filename );

      ~IndexReader();
      std::string Filename() const { return m_filename; }
      unsigned long long RunNumber() const { return m_runNumber; };
      std::string getJsonConfig() { return m_json_config; };
      
    private:
      std::string m_filename;
      unsigned long long m_runNumber;
      FileDeserializer * m_des;
      std::string m_json_config;
  };
 


}

#endif // EUDAQ_INCLUDED_IndexReader
