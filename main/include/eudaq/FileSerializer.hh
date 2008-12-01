#ifndef EUDAQ_INCLUDED_FileSerializer
#define EUDAQ_INCLUDED_FileSerializer

#include <string>
#include <cstdio>
#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  class FileSerializer : public Serializer {
  public:
    enum WPROT { WP_NONE, WP_ONCLOSE, WP_ONOPEN };
    FileSerializer(const std::string & fname, bool overwrite = false, int writeprotect = WP_ONCLOSE);
    virtual void Flush();
    unsigned long long FileBytes() const { return m_filebytes; }
    void WriteProtect();
    ~FileSerializer();
  private:
    virtual void Serialize(const unsigned char * data, size_t len);
    FILE * m_file;
    unsigned long long m_filebytes;
    int m_wprot;
  };

  class FileDeserializer : public Deserializer {
  public:
    FileDeserializer(const std::string & fname, bool faileof = false);
    virtual bool HasData();
  private:
    virtual void Deserialize(unsigned char * data, size_t len);
    FILE * m_file;
    bool m_faileof;
  };

}

#endif // EUDAQ_INCLUDED_FileSerializer
