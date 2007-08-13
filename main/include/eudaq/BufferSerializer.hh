#ifndef EUDAQ_INCLUDED_BufferSerializer
#define EUDAQ_INCLUDED_BufferSerializer

#include <deque>
#include <algorithm>
#include <iostream>
#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"

namespace eudaq {

  class BufferSerializer : public Serializer, public Deserializer {
  public:
    BufferSerializer() : m_offset(0) {}
    template<typename InIt>
    BufferSerializer(InIt first, InIt last) : m_data(first, last), m_offset(0) {
      //std::string tmp(begin(), end());
      //std::cout << "BufferSerializer: " << tmp.length() << /*" \"" << tmp << "\"" <<*/ std::endl;
    }

//     typedef std::vector<unsigned char>::iterator iterator;
//     typedef std::vector<unsigned char>::const_iterator const_iterator;
//     iterator begin() { return m_data.begin(); }
//     iterator end() { return m_data.end(); }
//     const_iterator begin() const { return m_data.begin(); }
//     const_iterator end() const { return m_data.end(); }
    const unsigned char & operator [] (size_t i) const { return m_data[i]; }
    size_t size() const { return m_data.size(); }
    virtual bool HasData() { return m_data.size() != 0; }
  private:
    virtual void Serialize(const unsigned char * data, size_t len) {
      m_data.insert(m_data.end(), data, data+len);
    }
    virtual void Deserialize(unsigned char * data, size_t len);
    std::vector<unsigned char> m_data;
    size_t m_offset;
  };

}

#endif // EUDAQ_INCLUDED_BufferSerializer
