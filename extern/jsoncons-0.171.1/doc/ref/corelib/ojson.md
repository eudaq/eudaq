### jsoncons::ojson

```cpp
#include <jsoncons/json.hpp>

typedef basic_json<char,
                   Policy = order_preserving_policy,
                   Allocator = std::allocator<char>> ojson
```
The `ojson` class is an instantiation of the [basic_json](basic_json.md) class template that uses `char` as the character type. The original insertion order of an object's name/value pairs is preserved. 


`ojson` behaves similarly to [json](json.md), with these particularities:

- `ojson`, like `json`, supports object member `insert_or_assign` methods that take an `object_iterator` as the first parameter. But while with `json` that parameter is just a hint that allows optimization, with `ojson` it is the actual location where to insert the member.

- In `ojson`, the `insert_or_assign` members that just take a name and a value always insert the member at the end.

### Examples
```cpp
ojson o = ojson::parse(R"(
{
    "street_number" : "100",
    "street_name" : "Queen St W",
    "city" : "Toronto",
    "country" : "Canada"
}
)");

std::cout << pretty_print(o) << std::endl;
```
Output:
```json
{
    "street_number": "100",
    "street_name": "Queen St W",
    "city": "Toronto",
    "country": "Canada"
}
```
Insert "postal_code" at end
```cpp
o.insert_or_assign("postal_code", "M5H 2N2");

std::cout << pretty_print(o) << std::endl;
```
Output:
```json
{
    "street_number": "100",
    "street_name": "Queen St W",
    "city": "Toronto",
    "country": "Canada",
    "postal_code": "M5H 2N2"
}
```
Insert "province" before "country"
```cpp
auto it = o.find("country");
o.insert_or_assign(it,"province","Ontario");

std::cout << pretty_print(o) << std::endl;
```
Output:
```json
{
    "street_number": "100",
    "street_name": "Queen St W",
    "city": "Toronto",
    "province": "Ontario",
    "country": "Canada",
    "postal_code": "M5H 2N2"
}
```

### See also

[json](json.md) constructs a json value that sorts name-value members alphabetically  

[wjson](wjson.md) constructs a wide character json value that sorts name-value members alphabetically  

[wojson](wojson.md) constructs a wide character json value that preserves the original insertion order of an object's name/value pairs  

