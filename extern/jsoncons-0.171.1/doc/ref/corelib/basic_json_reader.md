### jsoncons::basic_json_reader

```cpp
#include <jsoncons/json_reader.hpp>

template<
    class CharT,
    class Source=jsoncons::stream_source<CharT>,
    class TempAllocator=std::allocator<char>
>
class basic_json_reader 
```
`basic_json_reader` uses the incremental parser [basic_json_parser](json_parser.md) 
to read arbitrarily large files in chunks.
A `basic_json_reader` can read a sequence of JSON texts from a stream, using `read_next()`,
which omits the check for unconsumed non-whitespace characters. 

`basic_json_reader` is noncopyable and nonmoveable.

A number of specializations for common character types are defined:

Type                       |Definition
---------------------------|------------------------------
`json_string_reader`         |`basic_json_reader<char,string_source<char>>`       (since 0.164.0)
`wjson_string_reader`        |`basic_json_reader<wchar_t,string_source<wchar_t>>` (since 0.164.0)
`json_stream_reader`         |`basic_json_reader<char,stream_source<char>>`       (since 0.164.0)
`wjson_stream_reader`        |`basic_json_reader<wchar_t,stream_source<wchar_t>>` (since 0.164.0)
`json_reader`                |Constructible from either a string or stream                (deprecated since 0.164.0)
`wjson_reader`               |Constructible from either a wide character string or stream (deprecated since 0.164.0)

#### Member types

Type                       |Definition
---------------------------|------------------------------
char_type                  |CharT
source_type                |Source
string_view_type           |

#### Constructors

    template <class Sourceable>
    explicit basic_json_reader(Sourceable&& source, 
                               const TempAllocator& alloc = TempAllocator()); (1)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source, 
                      const basic_json_options<CharT>& options, 
                      const TempAllocator& alloc = TempAllocator()); (2)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source,
                      std::function<bool(json_errc,const ser_context&)> err_handler, 
                      const TempAllocator& alloc = TempAllocator()); (3)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source, 
                      const basic_json_options<CharT>& options,
                      std::function<bool(json_errc,const ser_context&)> err_handler, 
                      const TempAllocator& alloc = TempAllocator()); (4)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source, 
                      basic_json_visitor<CharT>& visitor, 
                      const TempAllocator& alloc = TempAllocator()); (5)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source, 
                      basic_json_visitor<CharT>& visitor,
                      const basic_json_options<CharT>& options, 
                      const TempAllocator& alloc = TempAllocator()); (6)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source,
                      basic_json_visitor<CharT>& visitor,
                      std::function<bool(json_errc,const ser_context&)> err_handler, 
                      const TempAllocator& alloc = TempAllocator()); (7)

    template <class Sourceable>
    basic_json_reader(Sourceable&& source,
                      basic_json_visitor<CharT>& visitor, 
                      const basic_json_options<CharT>& options,
                      std::function<bool(json_errc,const ser_context&)> err_handler, 
                      const TempAllocator& alloc = TempAllocator()); (8)

Constructors (1)-(4) use a default [basic_json_visitor](basic_json_visitor.md) that discards the JSON parse events, and are for validation only.

(1) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`, uses default [options](basic_json_options.md) and a default [JSON parsing error handling](err_handler.md).

(2) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`, 
uses the specified [options](basic_json_options.md)
and a default [JSON parsing error handling](err_handler.md).

(3) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`, 
uses default [options](basic_json_options.md)
and a specified [JSON parsing error handling](err_handler.md).

(4) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`, 
uses the specified [options](basic_json_options.md)
and a specified [JSON parsing error handling](err_handler.md).

Constructors (5)-(8) take a user supplied [basic_json_visitor](basic_json_visitor.md) that receives JSON parse events, such as a [json_decoder](json_decoder). 

(5) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`,
emits JSON parse events to the specified 
[basic_json_visitor](basic_json_visitor.md), and uses default [options](basic_json_options.md)
and a default [JSON parsing error handling](err_handler.md).

(6) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`,
emits JSON parse events to the specified [basic_json_visitor](basic_json_visitor.md) 
and uses the specified [options](basic_json_options.md)
and a default [JSON parsing error handling](err_handler.md).

(7) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`,
emits JSON parse events to the specified [basic_json_visitor](basic_json_visitor.md) 
and uses default [options](basic_json_options.md)
and a specified [JSON parsing error handling](err_handler.md).

(8) Constructs a `basic_json_reader` that reads from a character sequence or stream `source`,
emits JSON parse events to the specified [basic_json_visitor](basic_json_visitor.md) and
uses the specified [options](basic_json_options.md)
and a specified [JSON parsing error handling](err_handler.md).

Note: It is the programmer's responsibility to ensure that `basic_json_reader` does not outlive any source or 
visitor passed in the constuctor, as `basic_json_reader` holds pointers to but does not own these resources.

#### Parameters

`source` - a value from which a `source_type` is constructible.  

#### Member functions

    bool eof() const
Returns `true` when there are no more JSON texts to be read from the stream, `false` otherwise

    void read(); (1)
    void read(std::error_code& ec); (2)
Reads the next JSON text from the stream and reports JSON events to a [basic_json_visitor](basic_json_visitor.md), such as a [json_decoder](json_decoder.md).
Override (1) throws if parsing fails, or there are any unconsumed non-whitespace characters left in the input.
Override (2) sets `ec` to a [json_errc](jsoncons::json_errc.md) if parsing fails or if there are any unconsumed non-whitespace characters left in the input.

    void read_next()
    void read_next(std::error_code& ec)
Reads the next JSON text from the stream and reports JSON events to a [basic_json_visitor](basic_json_visitor.md), such as a [json_decoder](json_decoder.md).
Override (1) throws [ser_error](ser_error.md) if parsing fails.
Override (2) sets `ec` to a [json_errc](jsoncons::json_errc.md) if parsing fails.

    void check_done(); (1)
    void check_done(std::error_code& ec); (2)
Override (1) throws if there are any unconsumed non-whitespace characters in the input.
Override (2) sets `ec` to a [json_errc](jsoncons::json_errc.md) if there are any unconsumed non-whitespace characters left in the input.

    std::size_t line() const

    std::size_t column() const

### Examples

#### Parsing JSON text with exceptions
```
std::string input = R"({"field1"{}})";    

json_decoder<json> decoder;
json_string_reader reader(input,decoder);

try
{
    reader.read();
    json j = decoder.get_result();
}
catch (const ser_error& e)
{
    std::cout << e.what() << std::endl;
}

```
Output:
```
Expected name separator ':' at line 1 and column 10
```

#### Parsing JSON text with error codes
```
std::string input = R"({"field1":ru})";    
std::istringstream is(input);

json_decoder<json> decoder;
json_stream_reader reader(is,decoder);

std::error_code ec;
reader.read(ec);

if (!ec)
{
    json j = decoder.get_result();   
}
else
{
    std::cerr << ec.message() 
              << " at line " << reader.line() 
              << " and column " << reader.column() << std::endl;
}
```
Output:
```
Expected value at line 1 and column 11
```

#### Reading a sequence of JSON texts from a stream

`jsoncons` supports reading a sequence of JSON texts, such as shown below (`json-texts.json`):
```json
{"a":1,"b":2,"c":3}
{"a":4,"b":5,"c":6}
{"a":7,"b":8,"c":9}
```
This is the code that reads them: 
```cpp
std::ifstream is("json-texts.json");
if (!is.is_open())
{
    throw std::runtime_error("Cannot open file");
}

json_decoder<json> decoder;
json_stream_reader reader(is,decoder);

while (!reader.eof())
{
    reader.read_next();
    if (!reader.eof())
    {
        json j = decoder.get_result();
        std::cout << j << std::endl;
    }
}
```
Output:
```json
{"a":1,"b":2,"c":3}
{"a":4,"b":5,"c":6}
{"a":7,"b":8,"c":9}
```
