#ifndef EUDAQ_INCLUDED_FileSerializer
#define EUDAQ_INCLUDED_FileSerializer

#include <string>
#include <fstream>
#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  class FileSerializer : public Serializer {
  public:
    FileSerializer(const std::string & fname, bool overwrite = false);
    virtual void Flush();
    unsigned long long FileBytes() const { return m_filebytes; }
  private:
    virtual void Serialize(const unsigned char * data, size_t len);
    std::ofstream m_stream;
    unsigned long long m_filebytes;
  };

  class FileDeserializer : public Deserializer {
  public:
    FileDeserializer(const std::string & fname, bool faileof = false);
    virtual bool HasData();
  private:
    virtual void Deserialize(unsigned char * data, size_t len);
    std::ifstream m_stream;
    bool m_faileof;
  };

}

#endif // EUDAQ_INCLUDED_FileSerializer
