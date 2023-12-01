### jsoncons::basic_json::merge

```cpp
void merge(const basic_json& source); (1)
void merge(basic_json&& source); (2)
void merge(object_iterator hint, const basic_json& source); (3)
void merge(object_iterator hint, basic_json&& source); (4)
```

Copies the key-value pairs in source json object into json object. If there is a member in source json object with key equivalent to the key of a member in json object, 
then that member is not copied. 

The `merge` function performs only a one-level-deep shallow merge, not a deep merge of nested objects.

#### Parameters

<table>
  <tr>
    <td><code>source</code></td>
    <td>`json` object value</td> 
  </tr>
</table>

#### Return value

None

#### Exceptions

Throws `std::domain_error` if source or *this are not json objects.

### Examples

#### Merge `json`

```cpp
json j = json::parse(R"(
{
    "a" : 1,
    "b" : 2
}
)");

const json source = json::parse(R"(
{
    "a" : 2,
    "c" : 3
}
)");

j1.merge(source);

std::cout << j << endl;
```
Output:

```json
{"a":1,"b":2,"c":3}
```

