#include <jsoncons/json.hpp>
#include <jsoncons_ext/msgpack/msgpack.hpp>
#include <jsoncons_ext/msgpack/msgpack_reader.hpp>
#include <catch/catch.hpp>
#include <sstream>

using namespace jsoncons;
using namespace msgpack;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size)
{
	std::string s(reinterpret_cast<const char*>(data), size);
	std::istringstream is(s);

    default_json_visitor visitor;
	msgpack_options options;
	options.max_nesting_depth(std::numeric_limits<int>::max());

	msgpack_stream_reader reader(is, visitor, options);
	std::error_code ec;
	reader.read(ec);

	return 0;
}
