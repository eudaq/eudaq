A big thanks to the following individuals for contributing:

- Andrew Hutko (early code review)        

- [Marc Chevrier](https://github.com/MarkaPola) (contributed clang port, build files, json is<T> and as<T> methods, 
and make_array template implementation.)

- [Pedro Larroy](https://github.com/larroy) and the developers of the clearskies_core project (contributed build 
system for posix systems, adding GCC to list of supported compilers, bug fixes, 
Android fix)

- [Cory Fields](https://github.com/theuni) for fixing warnings about unused variables

- [Vitaliy Gusev](https://github.com/gusev-vitaliy) (reported error in json object operator[size_t i])

- [Alex Merry](https://github.com/amerry) Reported errors with "typename" keyword experienced with gcc and provided 
workaround for gcc 4.8 regex issues. Fixed float128 feature macro names #273.

- [Ignatov Serguei](https://github.com/sergign60) (reported issues experienced with gcc for 0.95 and 
0.96 candidate and helped fix them)

- [Milan Burda](https://github.com/miniak) for fix for clang build error

- [Peter Tissen](https://github.com/Bigpet), for reporting and suggesting a fix for get(name,default_value)

- [Tom Bass](https://github.com/tbass) for assistance with clang build errors

- [Andrey Alifanov](https://github.com/AndreyAlifanov) and [Amit Naik](https://github.com/amitnaik1) for failing test cases for JSON Path

- [Yuri Plaksyuk](https://github.com/yplaksyuk) for contributing an extension to JsonPath to allow filter 
expressions over a single object. 

- [Nikolay Amiantov](https://github.com/abbradar) for fixing compilation errors and warnings by GCC and 
Clang, adding read support for std::array and, most appreciated,
adding Travis CI configuration.

- [jakalx](https://github.com/jakalx) contributed fix for operator== throws when comparing a string 
against an empty object

- [Alexander](https://github.com/rog13) for contributing fix to jsonpatch::diff

- [Stefano Sinigardi](https://github.com/cenit) for contributing workaround for vs2017 platform issue

- [xezon](https://github.com/danielaparker/jsoncons/pull/140) for proposing decode_csv and encode_csv functions, the
ignore_empty_lines option, fixes to mismatched allocator types, and fixes and improvements in string_view code. 
Also for contributing a refactoring of the basic_json variant as a union.

- Vojtech Fried for contributing patches to JSONCONS_DEFINE_LITERAL 
and to json::as_string to remove warnings

- [Joshua Pritikin](https://github.com/jpritikin), for reporting gcc ubsan runtime warnings about 
load of misaligned addresses, and verifying fix

- [Tobias Hermann](https://github.com/Dobiasd), for reporting issue with `UINT_MAX` not declared 
in `bigint.hpp`, and proposing fix.

- [Cebtenzzre](https://github.com/Cebtenzzre), for finding and fixing an issue with conversions on 
a basic_json value leading to an infinite recursion when the 
value is a bignum, and for fixing undefined behavior in the bignum 
class. 

- [massimo morara](https://github.com/massimomorara) for reporting numerous issues

- [Alexander B](https://github.com/bas524), for uncovering a bug in how json_parser validated
UTF-8 strings.

- [zhskyy](https://github.com/zhskyy), for contributing __FILE__ and __LINE__ macros removed 
from JSONCONS_ASSERT if not defined _DEBUG.

- [soberich](https://github.com/soberich), for contributing the jsonpath sum and prod functions,
and a proposal for aggregation functions that work outside a filter.

- [patternoia](https://github.com/patternoia) for fixing the installation script
to include copying the jsoncons_ext directory into the installation place

- [mikewallis](https://github.com/mikewallis) for removing redundant macro continuation character in JSONCONS_TYPE_TRAITS

- [KonstantinPlotnikov](https://github.com/KonstantinPlotnikov) for fixing GCC 9.2 warning: �class jsoncons::json_exception� 
has virtual functions and accessible non-virtual destructor

- [patternoia](https://github.com/patternoia), [Twan Springeling](https://github.com/Hippid), [Leroy.H.Y](https://github.com/lhy0403)
and [Rodrigo Broggi](https://github.com/rbroggi) for providing feedback and helpful suggestions regarding the `json_type_traits`
convenience macros.

- [dvzubarev](https://github.com/dvzubarev) for contributing suggestions for improving the `encode_traits` implementation.

- [David Korczynski](https://github.com/DavidKorczynski) for integrating jsoncons into OSS-fuzz and contributing fuzzers 

- [Sebi](https://github.com/PapeCoding) for [\#243](https://github.com/danielaparker/jsoncons/issues/243)
to fix BSON encoding error

- [Oleh Derevenko](https://github.com/oleh-derevenko) for [\#244](https://github.com/danielaparker/jsoncons/issues/244) 
and [\#245](https://github.com/danielaparker/jsoncons/pull/248) 
to make jsoncons work on QNX Neutrino target. 

- [Laurent Stacul](https://github.com/stac47) Fixed compilation error with gcc 11 #276

- [wbangna](https://github.com/wbangna) Contributed [\#279](https://github.com/danielaparker/jsoncons/pull/279)
