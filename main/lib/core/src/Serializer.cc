#include "eudaq/Serializer.hh"

namespace eudaq{
  Serializer::~Serializer(){
  }
  
  void Serializer::Flush(){
  };

  void Serializer::write(const Serializable &t){
    t.Serialize(*this);
  }

  void Serializer::append(const uint8_t *data, size_t size) {
    Serialize(data, size);
  }

  uint64_t Serializer::GetCheckSum(){
    return 0;
  }

}
