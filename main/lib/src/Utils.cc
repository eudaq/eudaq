#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cctype>

// for cross-platform sleep:
#include <chrono>
#include <thread>

#if EUDAQ_PLATFORM_IS(WIN32)
#ifndef __CINT__
#define WIN32_LEAN_AND_MEAN // causes some rarely used includes to be ignored
#define _WINSOCKAPI_
#define _WINSOCK2API_
#include <cstdio>  // HK
#include <cstdlib>  // HK
#if(_MSC_VER<1800)
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

#endif
#else
# include <unistd.h>
#endif
using namespace std;

namespace eudaq {

  std::string ucase(const std::string & str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::toupper(result[i]);
    }
    return result;
  }

  std::string lcase(const std::string & str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::tolower(result[i]);
    }
    return result;
  }

  /** Trims the leading and trainling white space from a string
   */
  std::string trim(const std::string & s) {
    static const std::string spaces = " \t\n\r\v";
    size_t b = s.find_first_not_of(spaces);
    size_t e = s.find_last_not_of(spaces);
    if (b == std::string::npos || e == std::string::npos) {
      return "";
    }
    return std::string(s, b, e - b + 1);
  }

  std::string escape(const std::string & s) {
    std::ostringstream ret;
    ret << std::setfill('0') << std::hex;
    for (size_t i = 0; i < s.length(); ++i) {
      if (s[i] == '\\')
        ret << "\\\\";
      else if (s[i] < 32)
        ret << "\\x" << std::setw(2) << int(s[i]);
      else
        ret << s[i];
    }
    return ret.str();
  }

  std::string firstline(const std::string & s) {
    size_t i = s.find('\n');
    return s.substr(0, i);
  }

  std::vector<std::string> split(const std::string & str, const std::string & delim) {
    return split(str, delim, false);
  }

  std::vector<std::string> split(const std::string & str, const std::string & delim, bool dotrim) {
    std::string s(str);
    std::vector<std::string> result;
    if (str == "") return result;
    size_t i;
    while ((i = s.find_first_of(delim)) != std::string::npos) {
      result.push_back(dotrim ? trim(s.substr(0, i)) : s.substr(0, i));
      s = s.substr(i + 1);
    }
    result.push_back(s);
    return result;
  }

  void mSleep(unsigned ms) {
    // use c++11 std sleep routine
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  }

  template<>
  int64_t from_string(const std::string & x, const int64_t & def) {
      if (x == "") return def;
      const char * start = x.c_str();
      size_t end = 0;
      int base = 10;
      std::string bases("box");
      if (x.length() > 2 && x[0] == '0' && bases.find(x[1]) != std::string::npos) {
        if (x[1] == 'b') base = 2;
        else if (x[1] == 'o') base = 8;
        else if (x[1] == 'x') base = 16;
        start += 2;
      }
      int64_t result = static_cast<int64_t>(std::stoll(start, &end, base));
      if (!x.substr(end).empty()) throw std::invalid_argument("Invalid argument: " + x);
      return result;
    }

  template<>
    uint64_t from_string(const std::string & x, const uint64_t & def) {
      if (x == "") return def;
      const char * start = x.c_str();
      size_t end = 0;
      int base = 10;
      std::string bases("box");
      if (x.length() > 2 && x[0] == '0' && bases.find(x[1]) != std::string::npos) {
        if (x[1] == 'b') base = 2;
        else if (x[1] == 'o') base = 8;
        else if (x[1] == 'x') base = 16;
        start += 2;
      }
      uint64_t result = static_cast<uint64_t>(std::stoull(start, &end, base));
      if (!x.substr(end).empty()) throw std::invalid_argument("Invalid argument: " + x);
      return result;
    }

  void WriteStringToFile(const std::string & fname, const std::string & val) {
    std::ofstream file(fname.c_str());
    if (!file.is_open()) EUDAQ_THROW("Unable to open file " + fname + " for writing");
    file << val << std::endl;
    if (file.fail()) EUDAQ_THROW("Error writing to file " + fname);
  }

  std::string ReadLineFromFile(const std::string & fname) {
    std::ifstream file(fname.c_str());
    std::string result;
    if (file.is_open()) {
      std::getline(file, result);
      if (file.fail()) {
        EUDAQ_THROW("Error reading from file " + fname);
      }
    }
    return result;
  }

}
