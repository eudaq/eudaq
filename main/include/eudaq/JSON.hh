
#ifndef EUDAQ_INCLUDED_JSON
#define EUDAQ_INCLUDED_JSON

#include <string>
#include "jsoncons/json.hpp"
#include "eudaq/Platform.hh"

namespace eudaq {

class DLLEXPORT JSON {
  public:
	JSON() {};
//	JSON( jsoncons::json type ) : json( type ) {};
	jsoncons::json& get() { return json; };
  protected:
	friend class MetaData;
	jsoncons::json json;
};


}

#endif // EUDAQ_INCLUDED_JSON
