
#ifndef EUDAQ_INCLUDED_JSON
#define EUDAQ_INCLUDED_JSON

#include <string>
#include "eudaq/Platform.hh"

namespace eudaq {

  class DLLEXPORT JSON {
    public:
	  static std::shared_ptr<JSON> Create();
	  const std::string & to_string();
    protected:
	  friend class MetaData;
	  JSON() {};
	  std::string jsonString;
  };

  typedef std::shared_ptr<JSON> JSONp;

}

#endif // EUDAQ_INCLUDED_JSON
