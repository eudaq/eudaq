#include <jsoncons/json.hpp>
#include <jsoncons_ext/ubjson/ubjson.hpp>
#include <sstream>
#include <vector>
#include <utility>
#include <ctime>
#include <limits>
#include <catch/catch.hpp>

using namespace jsoncons;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, std::size_t size)
{
    std::string input(reinterpret_cast<const char*>(data), size);
    std::istringstream is(input);
    try {
       json j2 = ubjson::decode_ubjson<json>(is);
    }
    catch(const jsoncons::ser_error&) {}

    return 0;
}

