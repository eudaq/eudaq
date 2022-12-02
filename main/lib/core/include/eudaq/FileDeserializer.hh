#ifndef EUDAQ_INCLUDED_FileDeserializer
#define EUDAQ_INCLUDED_FileDeserializer

#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Event.hh"
#include "eudaq/BufferSerializer.hh"
#include <memory>
#include <string>
#include <cstdio>

namespace eudaq{
  class DLLEXPORT FileDeserializer : public Deserializer {
  public:
    FileDeserializer(const std::string &fname, bool faileof = false,
                     size_t buffersize = 65536);
    ~FileDeserializer();
    virtual bool HasData();
    bool ReadEvent(int ver, EventSP &ev, size_t skip = 0);
    
  private:
    virtual void Deserialize(uint8_t *data, size_t len);
    virtual void PreDeserialize(uint8_t *data, size_t len);
    size_t FillBuffer(size_t min = 0);
    size_t level() const { return m_stop - m_start; }
    std::string m_filename;
    FILE *m_file;
    bool m_faileof;
    std::vector<uint8_t> m_buf;
    uint8_t *m_start;
    uint8_t *m_stop;
  };
}
#endif // EUDAQ_INCLUDED_FileSerializer
