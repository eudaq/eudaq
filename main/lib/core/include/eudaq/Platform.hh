#ifndef EUDAQ_INCLUDED_Platform
#define EUDAQ_INCLUDED_Platform

#include <cstddef>
#include <cstdint>
using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

#ifdef _WIN32
#include <crtdefs.h>
#else
using std::size_t;
#endif

#define PF_LINUX 1
#define PF_MACOSX 2
#define PF_CYGWIN 3
#define PF_WIN32 4

#define EUDAQ_PLATFORM_IS(P) (EUDAQ_PLATFORM == PF_##P)

#if EUDAQ_PLATFORM_IS(WIN32)

#ifdef EUDAQ_CORE_EXPORTS
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT __declspec(dllimport)
#endif
#else

#define DLLEXPORT
#endif


#include <memory>

#endif // EUDAQ_INCLUDED_Platform
