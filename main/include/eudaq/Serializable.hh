#ifndef EUDAQ_INCLUDED_Serializable
#define EUDAQ_INCLUDED_Serializable
#include "eudaq/Platform.hh"
//#include <string>
//#include <vector>
//#include <map>
//#include <iosfwd>

namespace eudaq {

  class Serializer; //$$ change

  class DLLEXPORT Serializable {
  public:
    virtual void Serialize(Serializer &) const = 0;
    virtual ~Serializable() {}
  };
}

#endif // EUDAQ_INCLUDED_Serializable
