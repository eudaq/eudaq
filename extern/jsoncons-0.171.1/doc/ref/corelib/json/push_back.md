### jsoncons::basic_json::push_back

```cpp
template <class T>
void push_back(T&& val)
```
Adds a new json element at the end of a json array. The argument `val` is forwarded to the `json` constructor as `std::forward<T>(val)`.
Throws `std::domain_error` if not an array.

### Examples

#### Creating an array of elements 
```cpp
json cities(json_array_arg);       // an empty array
std::cout << cities << std::endl;  // output is "[]"

cities.push_back("Toronto");  
cities.push_back("Vancouver");
// Insert "Montreal" at beginning of array
cities.insert(cities.array_range().begin(),"Montreal");  

std::cout << cities << std::endl;
```
Output:
```
[]
["Montreal","Toronto","Vancouver"]
```
#### Creating an array of elements with reserved storage 
```cpp
json cities(json_array_arg);  
cities.reserve(10);  // storage is reserved
std::cout << "capacity=" << cities.capacity() 
          << ", size=" << cities.size() << std::endl;

cities.push_back("Toronto");  
cities.push_back("Vancouver");
cities.insert(cities.array_range().begin(),"Montreal");
std::cout << "capacity=" << cities.capacity() 
          << ", size=" << cities.size() << std::endl;

std::cout << cities << std::endl;
```
Output:
```
capacity=10, size=0
capacity=10, size=3
["Montreal","Toronto","Vancouver"]
```

