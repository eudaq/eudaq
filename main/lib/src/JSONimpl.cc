
#include <iostream>
#include <map>

#include "jsoncons/json.hpp"
#include "eudaq/JSONimpl.hh"

using jsoncons::json;

namespace eudaq {

// Define factory method
  std::shared_ptr<JSON> JSON::Create() {
    return std::shared_ptr<JSON>( new JSONimpl() );
  }

  const std::string & JSON::to_string() {
	auto json = JSONimpl::get( this );
	jsonString = json.to_string();
	return jsonString;
  }


  jsoncons::json& JSONimpl::get( JSON * j ) {
	JSONimpl *impl =  (JSONimpl *)j;
	return impl->json;
  }

}
