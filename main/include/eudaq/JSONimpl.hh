
#ifndef EUDAQ_INCLUDED_JSONimpl
#define EUDAQ_INCLUDED_JSONimpl

#include <string>
#include "jsoncons/json.hpp"
#include "eudaq/Platform.hh"
#include "eudaq/JSON.hh"

namespace eudaq {

  class DLLEXPORT JSONimpl : public JSON {
    public:
	  JSONimpl() {};
	  static jsoncons::json& get( JSON * );

    protected:
	  jsoncons::json json;
};

}

#endif // EUDAQ_INCLUDED_JSONimpl

