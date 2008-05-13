#ifndef EUDAQ_INCLUDED_Utils
#define EUDAQ_INCLUDED_Utils

/**
 * \file Utils.hh
 * Contains generally useful utility functions.
 */

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace eudaq {

  std::string ucase(const std::string &);
  std::string lcase(const std::string &);
  std::string trim(const std::string & s);
  std::string firstline(const std::string & s);
  std::string escape(const std::string &);
  std::vector<std::string> split(const std::string & str, const std::string & delim = "\t");

  /** Sleep for a specified number of milliseconds.
   * \param ms The number of milliseconds
   */
  void mSleep(unsigned ms);

  /** Converts any type to a string.
   * There must be a compatible streamer defined, which this function will make use of.
   * \param x The value to be converted.
   * \return A string representing the passed in parameter.
   */
  template <typename T>
  inline std::string to_string(const T & x, int digits = 0) {
    std::ostringstream s;
    s << std::setfill('0') << std::setw(digits) << x;
    return s.str();
  }

  inline std::string to_string(const std::string & x, int /*digits*/ = 0) {
    return x;
  }
  inline std::string to_string(const char * x, int /*digits*/ = 0) {
    return x;
  }

  /** Converts any type that has an ostream streamer to a string in hexadecimal.
   * \param x The value to be converted.
   * \param digits The minimum number of digits, shorter numbers are padded with zeroes.
   * \return A string representing the passed in parameter in hex.
   */
  template <typename T>
  inline std::string to_hex(const T & x, int digits = 0) {
    std::ostringstream s;
    s << std::hex << std::setfill('0') << std::setw(digits) << x;
    return s.str();
  }

  template<>
  inline std::string to_hex(const unsigned char & x, int digits) {
    return to_hex((int)x, digits);
  }

  template<>
  inline std::string to_hex(const signed char & x, int digits) {
    return to_hex((int)x, digits);
  }

  template<>
  inline std::string to_hex(const char & x, int digits) {
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
  inline T from_string(const std::string & x, const T & def = 0) {
    if (x == "") return def;
    T ret = def;
    std::istringstream s(x);
    s >> ret;
    char remain = '\0';
    s >> remain;
    if (remain) throw std::invalid_argument("Invalid argument: " + x);
    return ret;
  }

  template<>
  inline std::string from_string(const std::string & x, const std::string & def) {
    return x == "" ? def : x;
  }

  template<>
  long from_string(const std::string & x, const long & def);
  template<>
  unsigned long from_string(const std::string & x, const unsigned long & def);
  template<>
  inline int from_string(const std::string & x, const int & def) {
    return from_string(x, (long)def);
  }
  template<>
  inline unsigned int from_string(const std::string & x, const unsigned int & def) {
    return from_string(x, (unsigned long)def);
  }

  template <typename T>
  struct Holder {
    Holder(T val) : m_val(val) {}
    T m_val;
  };

  template <typename T>
  struct hexdec_t {
    enum { DIGITS = 2 * sizeof (T) };
    hexdec_t(T val, unsigned hexdigits) : m_val(val), m_dig(hexdigits) {}
    T m_val;
    unsigned m_dig;
  };

  template <typename T>
  inline hexdec_t<T> hexdec(T val, unsigned hexdigits = hexdec_t<T>::DIGITS) {
    return hexdec_t<T>(val, hexdigits);
  }

  template <typename T>
  inline std::ostream & operator << (std::ostream & os, const hexdec_t<T> & h) {
    return os << h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <>
  inline std::ostream & operator << (std::ostream & os, const hexdec_t<unsigned char> & h) {
    return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <>
  inline std::ostream & operator << (std::ostream & os, const hexdec_t<signed char> & h) {
    return os << (int)h.m_val << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <>
  inline std::ostream & operator << (std::ostream & os, const hexdec_t<char> & h) {
    return os << (int)(unsigned char)h.m_val
	      << " (0x" << to_hex(h.m_val, h.m_dig) << ")";
  }

  template <typename T> unsigned char * uchar_cast(T * x) {
    return reinterpret_cast<unsigned char *>(x);
  }

  template <typename T> unsigned char * uchar_cast(std::vector<T> & x) {
    return uchar_cast(&x[0]);
  }

  template <typename T> const unsigned char * constuchar_cast(const T * x) {
    return reinterpret_cast<const unsigned char *>(x);
  }

  template <typename T> const unsigned char * constuchar_cast(const std::vector<T> & x) {
    return constuchar_cast(&x[0]);
  }

}

#endif // EUDAQ_INCLUDED_Utils
