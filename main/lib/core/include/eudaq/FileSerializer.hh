#ifndef EUDAQ_INCLUDED_FileSerializer
#define EUDAQ_INCLUDED_FileSerializer

#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Event.hh"
#include "eudaq/BufferSerializer.hh"
#include <memory>
#include <string>
#include <cstdio>

namespace eudaq {
  class DLLEXPORT FileSerializer : public Serializer {
  public:
    FileSerializer(const std::string &fname, bool overwrite = false);
    virtual void Flush();
    uint64_t FileBytes() const { return m_filebytes; }
    ~FileSerializer();

  private:
    virtual void Serialize(const uint8_t *data, size_t len);
    //virtual void Serialize_tgn(const uint8_t *data, size_t len);
    FILE *m_file;
    uint64_t m_filebytes;
    std::string m_fname;
  };

}

#endif // EUDAQ_INCLUDED_FileSerializer
