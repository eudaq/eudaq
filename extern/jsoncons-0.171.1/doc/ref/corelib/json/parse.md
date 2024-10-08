### jsoncons::basic_json::parse

```cpp
template <class Source>
static basic_json parse(const Source& source, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (1) 

static basic_json parse(const char_type* str, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (2) 

static basic_json parse(std::basic_istream<char_type>& is, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (3) 

template <class InputIt>
static basic_json parse(InputIt first, InputIt last, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (4) 

template <class Source, class TempAllocator>
static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, 
    const Source& source, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (5) (since 0.171.0)

template <class TempAllocator>
static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set,
    const char_type* str, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (6) (since 0.171.0)

template <class TempAllocator>
static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set, 
    std::basic_istream<char_type>& is, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (7) (since 0.171.0)

template <class InputIt,class TempAllocator>
static basic_json parse(const allocator_set<allocator_type,TempAllocator>& alloc_set,
    InputIt first, InputIt last, 
    const basic_json_decode_options<char_type>& options = basic_json_decode_options<CharT>());   (8) (since 0.171.0)

template <class Source>
static basic_json parse(const Source& source, 
    const basic_json_decode_options<char_type>& options,                                         (9) (deprecated since 0.171.0)
    std::function<bool(json_errc,const ser_context&)> err_handler);                              

template <class Source>
static basic_json parse(const Source& source, 
    std::function<bool(json_errc,const ser_context&)> err_handler);                              (10) (deprecated since 0.171.0)

static basic_json parse(const char_type* str, 
    const basic_json_decode_options<char_type>& options,                                         (11) (deprecated since 0.171.0)
    std::function<bool(json_errc,const ser_context&)> err_handler);                              

static basic_json parse(const char_type* str, 
    std::function<bool(json_errc,const ser_context&)> err_handler);                              (12) (deprecated since 0.171.0)

static basic_json parse(std::basic_istream<char_type>& is, 
    const basic_json_decode_options<char_type>& options,                                         (13) (deprecated since 0.171.0)
    std::function<bool(json_errc,const ser_context&)> err_handler);                              
              
static basic_json parse(std::istream& is, 
    std::function<bool(json_errc,const ser_context&)> err_handler);                              (14) (deprecated since 0.171.0)

template <class InputIt>
static basic_json parse(InputIt first, InputIt last, 
    const basic_json_decode_options<char_type>& options,                                         (15) (deprecated since 0.171.0)
    std::function<bool(json_errc,const ser_context&)> err_handler);           
              
template <class InputIt>
static basic_json parse(InputIt first, InputIt last,                                             (16) (deprecated since 0.171.0)
    std::function<bool(json_errc,const ser_context&)> err_handler);                              
```
(1) Parses JSON data from a contiguous character sequence provided by `source` and returns a `basic_json` value. 
Throws a [ser_error](../ser_error.md) if parsing fails.

(2) Parses JSON data from a null-terminated character string and returns a `basic_json` value. 
Throws a [ser_error](../ser_error.md) if parsing fails.

(3) Parses JSON data from an input stream and returns a `basic_json` value. 
Throws a [ser_error](../ser_error.md) if parsing fails.

(4) Parses JSON data from the range [`first`,`last`) and returns a `basic_json` value. 
Throws a [ser_error](../ser_error.md) if parsing fails.

(5)-(8) Same as (1)-(4), except they accept an [allocator_set](allocator_set.md) argument.

#### Parameters

`source` = a contigugous character source, such as a `std::string` or `std::string_view`

`str` - a null terminated character string  

`is` - an input stream  

`first`, `last` - pair of [LegacyInputIterators](https://en.cppreference.com/w/cpp/named_req/InputIterator) that specify a character sequence  

`options` - a [basic_json_options](../basic_json_options.md)  

`err_handler` - an error handler. Since 0.171.0, an error handler may be provided as a member of a [basic_json_options](../basic_json_options.md).  

### Examples

#### Parse from string

```cpp
try 
{
    json val = json::parse("[1,2,3,4,]");
} 
catch(const jsoncons::ser_error& e) 
{
    std::cout << e.what() << std::endl;
}
```
Output:
```
Extra comma at line 1 and column 10
```

#### Parse from string with options

```cpp
std::string s = R"({"field1":"NaN","field2":"PositiveInfinity","field3":"NegativeInfinity"})";

json_options options;
options.nan_to_str("NaN")
       .inf_to_str("PositiveInfinity")
       .neginf_to_str("NegativeInfinity");

json j = json::parse(s,options);

std::cout << "\n(1)\n" << pretty_print(j) << std::endl;

std::cout << "\n(2)\n" << pretty_print(j,options) << std::endl;
```
Output:
```
(1)
{
    "field1": null,
    "field2": null,
    "field3": null
}

(2)
{
    "field1": "NaN",
    "field2": "PositiveInfinity",
    "field3": "NegativeInfinity"
}
```

#### Parse from stream

Input JSON file `example.json`:

```json
{"File Format Options":{"Color Spaces":["sRGB","AdobeRGB","ProPhoto RGB"]}}
```

```cpp
std::ifstream is("example.json");
json j = json::parse(is);

std::cout << pretty_print(j) << std::endl;
```

Output:

```json
{
    "File Format Options": {
        "Color Spaces": ["sRGB","AdobeRGB","ProPhoto RGB"]
    }
}
```

#### Parse from pair of input iterators

```cpp
#include <jsoncons/json.hpp>

class MyIterator
{
    const char* p_;
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = char;
    using difference_type = std::ptrdiff_t;
    using pointer = const char*; 
    using reference = const char&;

    MyIterator(const char* p)
        : p_(p)
    {
    }

    reference operator*() const
    {
        return *p_;
    }

    pointer operator->() const 
    {
        return p_;
    }

    MyIterator& operator++()
    {
        ++p_;
        return *this;
    }

    MyIterator operator++(int) 
    {
        MyIterator temp(*this);
        ++*this;
        return temp;
    }

    bool operator!=(const MyIterator& rhs) const
    {
        return p_ != rhs.p_;
    }
};

int main()
{
    char source[] = {'[','\"', 'f','o','o','\"',',','\"', 'b','a','r','\"',']'};

    MyIterator first(source);
    MyIterator last(source + sizeof(source));

    json j = json::parse(first, last);

    std::cout << j << "\n\n";
}
```

Output:
```json
["foo","bar"]
```

#### Parse a JSON text using a `std::pmr::polymorphic_allocator` allocator (since 0.171.0)

```cpp
#include <jsoncons/json.hpp>

using namespace jsoncons; // for convenience

using pmr_json = jsoncons::pmr::json;

int main()
{
    char buffer[1024] = {}; // a small buffer on the stack
    std::pmr::monotonic_buffer_resource pool{std::data(buffer), std::size(buffer)};
    std::pmr::polymorphic_allocator<char> alloc(&pool);

    std::string json_text = R"(
    {
        "street_number" : "100",
        "street_name" : "Queen St W",
        "city" : "Toronto",
        "country" : "Canada"
    }
    )";

    try
    {
        auto doc = pmr_json::parse(combine_allocators(alloc), json_text);
        std::cout << pretty_print(doc) << "\n\n";
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}
```

Output:

```json
{
    "city": "Toronto",
    "country": "Canada",
    "street_name": "Queen St W",
    "street_number": "100"
}
```
