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
  private:
    virtual void Serialize(const unsigned char * data, size_t len);
    std::ofstream m_stream;
  };

  class FileDeserializer : public Deserializer {
  public:
    FileDeserializer(const std::string & fname);
    virtual bool HasData();
  private:
    virtual void Deserialize(unsigned char * data, size_t len);
    std::ifstream m_stream;
  };

}

#endif // EUDAQ_INCLUDED_FileSerializer
