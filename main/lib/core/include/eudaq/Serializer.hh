#ifndef EUDAQ_INCLUDED_Serializer
#define EUDAQ_INCLUDED_Serializer


#include "eudaq/Serializable.hh"
#include "eudaq/Time.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Platform.hh"

#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace eudaq {

  class InterruptedException : public std::exception {
    const char *what() const throw() { return "InterruptedException"; }
  };

  class DLLEXPORT Serializer {
  public:
    virtual ~Serializer();
    virtual void Flush();
    void write(const Serializable &t);
    template <typename T> void write(const T &t);
    template <typename T> void write_tgn(const T &t);
    template <typename T> void write(const std::vector<T> &t);
    template <typename T, typename U> void write(const std::map<T, U> &t);
    template <typename T, typename U> void write(const std::pair<T, U> &t);

    void append(const uint8_t *data, size_t size);
    virtual uint64_t GetCheckSum();
  private:
    template <typename T> friend struct WriteHelper;
    virtual void Serialize(const uint8_t *, size_t) = 0;
    //void Serialize_tgn(const uint8_t *, size_t);
  };

  template <typename T> struct WriteHelper {
    typedef void (*writer)(Serializer &ser, const T &v);
    static writer GetFunc(Serializable *) { return write_ser; }
    static writer GetFunc(float *) { return write_float; }
    static writer GetFunc(double *) { return write_double; }
    static writer GetFunc(bool *) { return write_char; }
    static writer GetFunc(uint8_t *) {/*std::cout << "Serializer::write(uint8_t)" << std::endl;*/ return write_char;  }
    static writer GetFunc(int8_t *) { return write_char; }
    static writer GetFunc(uint16_t *) {/*std::cout << "Serializer::write(uint16_t)" << std::endl;*/ return write_int; }
    static writer GetFunc(int16_t *) { return write_int; }
    static writer GetFunc(uint32_t *) {/*std::cout << "Serializer::write(uint32_t)" << std::endl;*/ return write_int; }
    static writer GetFunc(int32_t *) { return write_int; }
    static writer GetFunc(uint64_t *) {/*std::cout << "Serializer::write(uint64_t)" << std::endl;*/ return write_int; }
    static writer GetFunc(int64_t *) { return write_int; }

    static void write_ser(Serializer &sr, const T &v) { v.Serialize(sr); }
    static void write_char(Serializer &sr, const T &v) {
      static_assert(sizeof(v) == 1, "Called write_char() in Serializer.hh "
                                    "which only supports integers of size == 1 "
                                    "byte!");
      uint8_t buf[sizeof(char)];
      buf[0] = static_cast<uint8_t>(v & 0xff);
      sr.Serialize(buf, sizeof(char));
    }

    static void write_int(Serializer &sr, const T &v) {
      static_assert(sizeof(v) > 1, "Called write_int() in Serializer.hh which "
                                   "only supports integers of size > 1 byte!");
      T t = v;
      uint8_t buf[sizeof v];
      for (size_t i = 0; i < sizeof v; ++i) {
        buf[i] = static_cast<uint8_t>(t & 0xff);
        t >>= 8;
      }
      sr.Serialize(buf, sizeof v);
    }
    static void write_float(Serializer &sr, const float &v) {
      unsigned t = *(unsigned *)&v;
      uint8_t buf[sizeof t];
      for (size_t i = 0; i < sizeof t; ++i) {
        buf[i] = t & 0xff;
        t >>= 8;
      }
      sr.Serialize(buf, sizeof t);
    }
    static void write_double(Serializer &sr, const double &v) {
      uint64_t t = *(uint64_t *)&v;
      uint8_t buf[sizeof t];
      for (size_t i = 0; i < sizeof t; ++i) {
        buf[i] = t & 0xff;
        t >>= 8;
      }
      sr.Serialize(buf, sizeof t);
    }
  };

  template <typename T> inline void Serializer::write(const T &v) {
    //std::cout << "Serializer::write(const T &v)" << std::endl;
    WriteHelper<T>::GetFunc(static_cast<T *>(0))(*this, v);
  }

  /*
  template <typename T> inline void Serializer::write_tgn(const T &v) {
      static_assert(sizeof(v) > 1, "Called write_int() in Serializer.hh which "
                                   "only supports integers of size > 1 byte!");
      T t = v;
      uint8_t buf[sizeof v];
      for (size_t i = 0; i < sizeof v; ++i) {
        buf[i] = static_cast<uint8_t>(t & 0xff);
        t >>= 8;
      }
      std::cout<<"Serializer::write_tgn()" << std::endl;
      this->Serialize_tgn(buf, sizeof v);
    }
    */

  template <> inline void Serializer::write(const std::string &t) {
    //std::cout << "Serializer::write(const std::string &t)" << std::endl;
    write((unsigned)t.length());
    Serialize(reinterpret_cast<const uint8_t *>(&t[0]), t.length());
  }

  template <> inline void Serializer::write(const Time &t) {
    //std::cout << "Serializer::write(const Time &t)" << std::endl;
    write((int)t.GetTimeval().tv_sec);
    write((int)t.GetTimeval().tv_usec);
  }

  template <> inline void Serializer::write(const std::vector<bool> &t) {
    //std::cout << "Serializer::write(const std::vector<bool> &t)" << std::endl;
    unsigned len = t.size();
    write(len);
    for (size_t i = 0; i < len; ++i) {
      write((uint8_t)t[i]);
    }
  }

  template <typename T> inline void Serializer::write(const std::vector<T> &t) {
  //std::cout << "Serializer::write(const std::vector<T> &t)" << std::endl;
    unsigned len = t.size();
    write(len);
    for (size_t i = 0; i < len; ++i) {
      write(t[i]);
    }
  }

  template <>
  inline void
  Serializer::write<uint8_t>(const std::vector<uint8_t> &t) {
    write((unsigned)t.size());
    Serialize(&t[0], t.size());
  }

  template <> inline void Serializer::write<char>(const std::vector<char> &t) {
    write((unsigned)t.size());
    Serialize(reinterpret_cast<const uint8_t *>(&t[0]), t.size());
  }

  template <typename T, typename U>
  inline void Serializer::write(const std::map<T, U> &t) {
    unsigned len = (unsigned)t.size();
    write(len);
    for (typename std::map<T, U>::const_iterator i = t.begin(); i != t.end();
         ++i) {
      write(i->first);
      write(i->second);
    }
  }

  template <typename T, typename U>
  inline void Serializer::write(const std::pair<T, U> &t) {
    write(t->first);
    write(t->second);
  }
}

#endif // EUDAQ_INCLUDED_Serializer
