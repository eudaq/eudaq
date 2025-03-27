#ifndef EUDAQ_INCLUDED_Utils
#define EUDAQ_INCLUDED_Utils

#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <fstream>

#include "Platform.hh"

namespace eudaq {

  std::string DLLEXPORT ucase(const std::string &);
  std::string DLLEXPORT lcase(const std::string &);
  std::string DLLEXPORT trim(const std::string &s);
  std::string DLLEXPORT firstline(const std::string &s);
  std::string DLLEXPORT escape(const std::string &);
  std::vector<std::string> DLLEXPORT
  split(const std::string &str, const std::string &delim = "\t");
  std::vector<std::string> DLLEXPORT
  split(const std::string &str, const std::string &delim, bool dotrim);

  void DLLEXPORT bool2uchar(const bool *inBegin, const bool *inEnd,
                            std::vector<unsigned char> &out);
  void DLLEXPORT uchar2bool(const unsigned char *inBegin,
                            const unsigned char *inEnd, std::vector<bool> &out);


  constexpr uint32_t cstr2hash(const char* str, uint32_t h = 0){
    return !str[h] ? 5381 : (cstr2hash(str, h+1) * 33ULL) ^ str[h];
  }

  uint32_t DLLEXPORT str2hash(const std::string &stdstr);
  std::vector<std::string> splitString(std::string str, char delimiter);

  /** Sleep for a specified number of milliseconds.
   * \param ms The number of milliseconds
   */
  void DLLEXPORT mSleep(unsigned ms);

  /** Converts any type to a string.
   * There must be a compatible streamer defined, which this function will make
   * use of.
   * \param x The value to be converted.
   * \return A string representing the passed in parameter.
   */
  template <typename T>
  inline std::string to_string(const T &x, int digits = 0) {
    std::ostringstream s;
    s << std::setfill('0') << std::setw(digits) << x;
    return s.str();
  }

  template <typename T>
  inline std::string to_string(const std::vector<T> &x, const std::string &sep,
                               int digits = 0) {
    std::ostringstream s;
    if (x.size() > 0)
      s << to_string(x[0], digits);
    for (size_t i = 1; i < x.size(); ++i) {
      s << sep << to_string(x[i], digits);
    }
    return s.str();
  }

  template <typename T>
  inline std::string to_string(const std::vector<T> &x, int digits = 0) {
    return to_string(x, ",", digits);
  }

  template <typename T, size_t N>
  inline std::string to_string(const std::array<T, N> &x, const std::string &sep,
                               int digits = 0) {
    return to_string(std::vector<T>(x.begin(), x.end()), sep, digits);
  }

  template <typename T, size_t N>
  inline std::string to_string(const std::array<T, N> &x, int digits = 0) {
    return to_string(x, ",", digits);
  }

  inline std::string to_string(const std::string &x, int /*digits*/ = 0) {
    return x;
  }
  inline std::string to_string(const char *x, int /*digits*/ = 0) { return x; }

  /** Converts any type that has an ostream streamer to a string in hexadecimal.
   * \param x The value to be converted.
   * \param digits The minimum number of digits, shorter numbers are padded with
   * zeroes.
   * \return A string representing the passed in parameter in hex.
   */
  template <typename T> inline std::string to_hex(const T &x, int digits = 0) {
    std::ostringstream s;
    s << std::hex << std::setfill('0') << std::setw(digits) << x;
    return s.str();
  }
  uint64_t hex2uint_64(const std::string& hex_string);

  template <> inline std::string to_hex(const unsigned char &x, int digits) {
    return to_hex((int)x, digits);
  }

  template <> inline std::string to_hex(const signed char &x, int digits) {
    return to_hex((int)x, digits);
  }

  template <> inline std::string to_hex(const char &x, int digits) {
    return to_hex((unsigned char)x, digits);
  }

  /** Converts a string to any type.
   * \param x The string to be converted.
   * \param def The default value to be used in case of an invalid string,
   *            this can also be useful to select the correct template type
   *            without having to specify it explicitly.
   * \return An object of type T with the value represented in x, or if
   *         that is not valid then the value of def.
   */
  template <typename T>
  inline T DLLEXPORT from_string(const std::string &x, const T &def = 0) {
    if (x == "")
      return def;
    T ret = def;
    std::istringstream s(x);
    s >> ret;
    char remain = '\0';
    s >> remain;
    if (remain)
      throw std::invalid_argument("Invalid argument: " + x);
    return ret;
  }

  template <>
  inline std::string DLLEXPORT
  from_string(const std::string &x, const std::string &def) {
    return x == "" ? def : x;
  }

  template <>
  int64_t DLLEXPORT from_string(const std::string &x, const int64_t &def);
  template <>
  uint64_t DLLEXPORT from_string(const std::string &x, const uint64_t &def);
  template <>
  inline int32_t DLLEXPORT
  from_string(const std::string &x, const int32_t &def) {
    return static_cast<int32_t>(from_string(x, (int64_t)def));
  }
  template <>
  inline uint32_t DLLEXPORT
  from_string(const std::string &x, const uint32_t &def) {
    return static_cast<uint32_t>(from_string(x, (uint64_t)def));
  }

  template <>
  inline int16_t DLLEXPORT
  from_string(const std::string &x, const int16_t &def) {
    return static_cast<int16_t>(from_string(x, (int64_t)def));
  }
  template <>
  inline uint16_t DLLEXPORT
  from_string(const std::string &x, const uint16_t &def) {
    return static_cast<uint16_t>(from_string(x, (uint64_t)def));
  }
  
  template <>
  bool DLLEXPORT from_string(const std::string &x, const bool &def);

  template <typename T> struct hexdec_t {
    enum { DIGITS = 2 * sizeof(T) };
    hexdec_t(T val, unsigned hexdigits) : m_val(val), m_dig(hexdigits) {}
    T m_val;
    unsigned m_dig;
  };

  template <typename T>
  inline hexdec_t<T> hexdec(T val, unsigned hexdigits = hexdec_t<T>::DIGITS) {
    return hexdec_t<T>(val, hexdigits);
  }

  template <typename T>
  inline std::ostream &operator<<(std::ostream & os, const hexdec_t<T> &h) {
    return os << "0x" << to_hex(h.m_val, h.m_dig) << " (" << h.m_val << ")";
  }

  template <>
  inline std::ostream &operator<<(std::ostream & os,
                                  const hexdec_t<unsigned char> &h) {
    return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <>
  inline std::ostream &operator<<(std::ostream & os,
                                  const hexdec_t<signed char> &h) {
    return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <>
  inline std::ostream &operator<<(std::ostream & os, const hexdec_t<char> &h) {
    return os << (int)(unsigned char)h.m_val << " (0x"
              << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <typename T> unsigned char *uchar_cast(T * x) {
    return reinterpret_cast<unsigned char *>(x);
  }

  template <typename T> unsigned char *uchar_cast(std::vector<T> & x) {
    return uchar_cast(&x[0]);
  }

  template <typename T> const unsigned char *constuchar_cast(const T *x) {
    return reinterpret_cast<const unsigned char *>(x);
  }

  template <typename T>
  const unsigned char *constuchar_cast(const std::vector<T> &x) {
    return constuchar_cast(&x[0]);
  }

  template <typename T> inline T getbigendian(const unsigned char *ptr) {
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) ||                 \
    (defined(__DARWIN_BYTE_ORDER) &&                                           \
     __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN)
    return *reinterpret_cast<const T *>(ptr);
#else
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
      result <<= 8;
      result += *ptr++;
    }
    return result;
#endif
  }

  template <typename T> inline T getlittleendian(const unsigned char *ptr) {
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||              \
    (defined(__DARWIN_BYTE_ORDER) &&                                           \
     __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
    return *reinterpret_cast<const T *>(ptr);
#else
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
      result += *ptr++ << (8 * i);
    }
    return result;
#endif
  }



  template <typename T>
  inline void setbigendian(unsigned char *ptr, const T &val) {
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) ||                 \
    (defined(__DARWIN_BYTE_ORDER) &&                                           \
     __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN)
    *reinterpret_cast<T *>(ptr) = val;
#else
    T tmp = val;
    ptr += sizeof(T);
    for (size_t i = 0; i < sizeof(T); ++i) {
      *--ptr = tmp & 0xff;
      tmp >>= 8;
    }
#endif
  }

  template <typename T>
  inline void setlittleendian(unsigned char *ptr, const T &val) {
#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||              \
    (defined(__DARWIN_BYTE_ORDER) &&                                           \
     __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN)
    *reinterpret_cast<T *>(ptr) = val;
#else
    T tmp = val;
    for (size_t i = 0; i < sizeof(T); ++i) {
      *ptr++ = tmp & 0xff;
      tmp >>= 8;
    }
#endif
  }

  std::string DLLEXPORT ReadLineFromFile(const std::string &fname);

  template <typename T>
  inline T ReadFromFile(const std::string &fname, const T &def = 0) {
    return from_string(ReadLineFromFile(fname), def);
  }

  void DLLEXPORT
  WriteStringToFile(const std::string &fname, const std::string &val);

  template <typename T>
  inline void WriteToFile(const std::string &fname, const T &val) {
    WriteStringToFile(fname, to_string(val));
  }
}

#endif // EUDAQ_INCLUDED_Utils
