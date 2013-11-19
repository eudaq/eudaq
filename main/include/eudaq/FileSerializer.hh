#ifndef EUDAQ_INCLUDED_FileSerializer
#define EUDAQ_INCLUDED_FileSerializer

#include <string>
#include <cstdio>
#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/BufferSerializer.hh"

namespace eudaq {

  class DLLEXPORT FileSerializer : public Serializer {
    public:
      FileSerializer(const std::string & fname, bool overwrite = false);
      virtual void Flush();
      unsigned long long FileBytes() const { return m_filebytes; }
      ~FileSerializer();
    private:
      virtual void Serialize(const unsigned char * data, size_t len);
      FILE * m_file;
      unsigned long long m_filebytes;
  };

  class DLLEXPORT FileDeserializer : public Deserializer {
    public:
      FileDeserializer(const std::string & fname, bool faileof = false, size_t buffersize = 65536);
      virtual bool HasData();
      template <typename T>
        T peek() {
          FillBuffer();
          BufferSerializer buf(m_start, m_stop);
          T result;
          buf.read(result);
          return result;
        }
    private:
      virtual void Deserialize(unsigned char * data, size_t len);
      size_t FillBuffer(size_t min = 0);
      size_t level() const { return m_stop - m_start; }
      typedef unsigned char *ptr_t;
      FILE * m_file;
      bool m_faileof;
      std::vector<unsigned char> m_buf;
      ptr_t m_start, m_stop;
  };

}

#endif // EUDAQ_INCLUDED_FileSerializer
