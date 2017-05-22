#include "eudaq/Deserializer.hh"

namespace eudaq{
  Deserializer::Deserializer()
    :m_interrupting(false){
  }
  Deserializer::~Deserializer(){
  }
  
  void Deserializer::Interrupt(){
    m_interrupting = true;
  }

  void Deserializer::read(unsigned char *dst, size_t size){
    Deserialize(dst, size);
  }

  void Deserializer::PreRead(uint32_t &t){
      unsigned char buf[sizeof(uint32_t)];
      PreDeserialize(buf, sizeof(uint32_t)); // 1.x serializer is little-endian (same to intel)
      t = 0;
      for (size_t i = 0; i < sizeof t; ++i) {
	t <<= 8;
	t += buf[sizeof t - 1 - i];
      }
      //TODO:: ntohl htonl
  }
  
  void Deserializer::PreRead(unsigned char *dst, size_t size) {
    PreDeserialize(dst, size);
  }

}
