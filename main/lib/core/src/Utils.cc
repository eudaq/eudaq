#include "eudaq/Utils.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include <cstring>
#include <string>
#include <cstdlib>
#include <iostream>
#include <cctype>

#include <algorithm>
#include <chrono>
#include <string_view>
#include <thread>
#include <unordered_map>

namespace eudaq {
  

  
  uint64_t hex2uint_64(const std::string& hex_string){

    auto ts = "0x" + hex_string;
    return std::stoull(ts, nullptr, 16);
  }

  std::string ucase(const std::string & str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::toupper(result[i]);
    }
    return result;
  }

  std::string lcase(const std::string &str) {
    std::string result(str);
    for (size_t i = 0; i < result.length(); ++i) {
      result[i] = std::tolower(result[i]);
    }
    return result;
  }

  /** Trims the leading and trainling white space from a string
   */
  std::string trim(const std::string &s) {
    static const std::string spaces = " \t\n\r\v";
    size_t b = s.find_first_not_of(spaces);
    size_t e = s.find_last_not_of(spaces);
    if (b == std::string::npos || e == std::string::npos) {
      return "";
    }
    return std::string(s, b, e - b + 1);
  }

  std::string escape(const std::string &s) {
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

  std::string firstline(const std::string &s) {
    size_t i = s.find('\n');
    return s.substr(0, i);
  }

  std::vector<std::string> split(const std::string &str,
                                 const std::string &delim) {
    return split(str, delim, false);
  }

  std::vector<std::string> split(const std::string &str,
                                 const std::string &delim, bool dotrim) {
    std::string s(str);
    std::vector<std::string> result;
    if (str == "")
      return result;
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

  template <> int64_t from_string(const std::string &x, const int64_t &def) {
    if (x == "")
      return def;
    const char *start = x.c_str();
    size_t end = 0;
    int base = 10;
    std::string bases("box");
    if (x.length() > 2 && x[0] == '0' &&
        bases.find(x[1]) != std::string::npos) {
      if (x[1] == 'b')
        base = 2;
      else if (x[1] == 'o')
        base = 8;
      else if (x[1] == 'x')
        base = 16;
      start += 2;
    }
    int64_t result = static_cast<int64_t>(std::stoll(start, &end, base));
    if (!x.substr(end).empty())
      throw std::invalid_argument("Invalid argument: " + x);
    return result;
  }

  template <> uint64_t from_string(const std::string &x, const uint64_t &def) {
    if (x == "")
      return def;
    const char *start = x.c_str();
    size_t end = 0;
    int base = 10;
    std::string bases("box");
    if (x.length() > 2 && x[0] == '0' &&
        bases.find(x[1]) != std::string::npos) {
      if (x[1] == 'b')
        base = 2;
      else if (x[1] == 'o')
        base = 8;
      else if (x[1] == 'x')
        base = 16;
      start += 2;
    }
    uint64_t result = static_cast<uint64_t>(std::stoull(start, &end, base));
    if (!x.substr(end).empty())
      throw std::invalid_argument("Invalid argument: " + x);
    return result;
  }
  
  template <> bool from_string(const std::string &x, const bool &def) {
    if (x == "")
      return def;
    
    static const std::unordered_map<std::string_view, bool> lookup = {
        {"1", true}, {"true", true}, {"yes", true}, {"on", true},
        {"0", false}, {"false", false}, {"no", false}, {"off", false}
    };

    std::string lowercase(x);
    std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(), ::tolower);

    if (auto it = lookup.find(lowercase); it != lookup.end()) {
        return it->second;
    }
    
    throw std::invalid_argument("Invalid boolean string: " + std::string(x));
  }

  void WriteStringToFile(const std::string &fname, const std::string &val) {
    std::ofstream file(fname.c_str());
    if (!file.is_open())
      EUDAQ_THROW("Unable to open file " + fname + " for writing");
    file << val << std::endl;
    if (file.fail())
      EUDAQ_THROW("Error writing to file " + fname);
  }

  std::string ReadLineFromFile(const std::string &fname) {
    std::ifstream file(fname.c_str());
    std::string result;
    if (file.is_open()) {
      std::getline(file, result);
      if (file.fail()) {
        EUDAQ_THROW("Error reading from file " + fname);
      }
    } else EUDAQ_THROW("Unable to open file " + fname + " for reading");
    return result;
  }

  void bool2uchar(const bool *inBegin, const bool *inEnd,
                  std::vector<unsigned char> &out) {
    int j = 0;
    unsigned char dummy = 0;
    // bool* d1=&in[0];
    size_t size = (inEnd - inBegin);
    if (size % 8) {
      size += 8;
    }
    size /= 8;
    out.reserve(size);
    for (auto i = inBegin; i < inEnd; ++i) {
      dummy += (unsigned char)(*i) << (j % 8);

      if ((j % 8) == 7) {
        out.push_back(dummy);
        dummy = 0;
      }
      ++j;
    }
  }

  void DLLEXPORT uchar2bool(const unsigned char *inBegin,
                            const unsigned char *inEnd,
                            std::vector<bool> &out) {
    auto distance = inEnd - inBegin;
    out.reserve(distance * 8);
    for (auto i = inBegin; i != inEnd; ++i) {
      for (int j = 0; j < 8; ++j) {
        out.push_back((*i) & (1 << j));
      }
    }
  }

  uint32_t DLLEXPORT str2hash(const std::string &stdstr){
    return cstr2hash(stdstr.c_str());
  }


  // not using boost requires custom character playing :)
  // stolen from https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
  std::vector<std::string> splitString(std::string str, char delimiter){
      std::vector<std::string> tokens;
       std::string token;
       std::istringstream tokenStream(str);
       while (std::getline(tokenStream, token, delimiter))
       {
          tokens.push_back(token);
       }
       return tokens;
  }


 }
