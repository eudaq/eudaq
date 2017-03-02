#ifndef EUDAQ_INCLUDED_Serializable
#define EUDAQ_INCLUDED_Serializable
#include "eudaq/Platform.hh"
namespace eudaq {

  class Serializer;

  class DLLEXPORT Serializable {
  public:
    virtual void Serialize(Serializer &) const = 0;
    virtual ~Serializable();
  };
}

#endif // EUDAQ_INCLUDED_Serializable
