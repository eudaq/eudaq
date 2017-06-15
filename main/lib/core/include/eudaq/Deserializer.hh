#ifndef EUDAQ_INCLUDED_Deserializer
#define EUDAQ_INCLUDED_Deserializer

#include "eudaq/Serializable.hh"
#include "eudaq/Time.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Platform.hh"

#include <string>
#include <vector>
#include <map>

namespace eudaq{
  class DLLEXPORT Deserializer {
  public:
    Deserializer();
    virtual ~Deserializer();
    virtual bool HasData() = 0;
    void Interrupt();

    template <typename T> void read(T &t);

    template <typename T> void read(std::vector<T> &t);

    template <typename T, typename U> void read(std::map<T, U> &t);

    template <typename T, typename U> void read(std::pair<T, U> &t);

    template <typename T> T read() {
      T t;
      read(t);
      return t;
    }
      
    void read(unsigned char *dst, size_t size);
    void PreRead(uint32_t &t);
    void PreRead(uint8_t *dst, size_t size);
  protected:
    bool m_interrupting;

  private:
    template <typename T> friend struct ReadHelper;
    virtual void Deserialize(unsigned char *, size_t) = 0;
    virtual void PreDeserialize(unsigned char *, size_t) = 0;
  };

  template <typename T> struct ReadHelper {
    typedef T (*reader)(Deserializer &ser);
    static reader GetFunc(Serializable *) { return read_ser; }
    static reader GetFunc(float *) { return read_float; }
    static reader GetFunc(double *) { return read_double; }
    static reader GetFunc(bool *) { return read_char; }
    static reader GetFunc(uint8_t *) { return read_char; }
    static reader GetFunc(int8_t *) { return read_char; }
    static reader GetFunc(uint16_t *) { return read_int; }
    static reader GetFunc(int16_t *) { return read_int; }
    static reader GetFunc(uint32_t *) { return read_int; }
    static reader GetFunc(int32_t *) { return read_int; }
    static reader GetFunc(uint64_t *) { return read_int; }
    static reader GetFunc(int64_t *) { return read_int; }

    static T read_ser(Deserializer &ds) { return T(ds); }
    static T read_char(Deserializer &ds) {
      unsigned char buf[sizeof(char)];
      ds.Deserialize(buf, sizeof(char));
      T t = buf[0];
      return t;
    }
    static T read_int(Deserializer &ds) {
      // protect against types of 8 bit (or less) -- would cause indefined
      // behaviour in bit shift below
      static_assert(sizeof(T) > 1, "Called read_int() in Serializer.hh which "
                                   "only supports integers of size > 1 byte!");
      unsigned char buf[sizeof(T)];
      ds.Deserialize(buf, sizeof(T));
      T t = 0;
      for (size_t i = 0; i < sizeof t; ++i) {
        t <<= 8;
        t += buf[sizeof t - 1 - i];
      }
      return t;
    }
    static float read_float(Deserializer &ds) {
      unsigned char buf[sizeof(float)];
      ds.Deserialize(buf, sizeof buf);
      unsigned t = 0;
      for (size_t i = 0; i < sizeof t; ++i) {
        t <<= 8;
        t += buf[sizeof t - 1 - i];
      }
      return *(float *)&t;
    }
    static double read_double(Deserializer &ds) {
      union {
        double d;
        uint64_t i;
        unsigned char b[sizeof(double)];
      } u;
      // unsigned char buf[sizeof (double)];
      ds.Deserialize(u.b, sizeof u.b);
      uint64_t t = 0;
      for (size_t i = 0; i < sizeof t; ++i) {
        t <<= 8;
        t += u.b[sizeof t - 1 - i];
      }
      u.i = t;
      return u.d;
    }
  };

  template <typename T> inline void Deserializer::read(T &t) {
    t = ReadHelper<T>::GetFunc(static_cast<T *>(0))(*this);
  }

  template <> inline void Deserializer::read(std::string &t) {
    unsigned len = 0;
    read(len);
    t = std::string(len, ' ');
    if (len)
      Deserialize(reinterpret_cast<unsigned char *>(&t[0]), len);
  }

  template <> inline void Deserializer::read(Time &t) {
    int sec, usec;
    read(sec);
    read(usec);
    t = Time(sec, usec);
  }

  template <typename T> inline void Deserializer::read(std::vector<T> &t) {
    unsigned len = 0;
    read(len);
    t.reserve(len);
    for (size_t i = 0; i < len; ++i) {
      t.push_back(read<T>());
    }
  }

  template <>
  inline void Deserializer::read<unsigned char>(std::vector<unsigned char> &t) {
    unsigned len = 0;
    read(len);
    t.resize(len);
    Deserialize(&t[0], len);
  }

  template <> inline void Deserializer::read<char>(std::vector<char> &t) {
    unsigned len = 0;
    read(len);
    t.resize(len);
    Deserialize(reinterpret_cast<unsigned char *>(&t[0]), len);
  }

  template <typename T, typename U>
  inline void Deserializer::read(std::map<T, U> &t) {
    unsigned len = 0;
    read(len);
    for (size_t i = 0; i < len; ++i) {
      auto&& key = read<T>();
      t[key] = read<U>();
    }
  }

  template <typename T, typename U>
  inline void Deserializer::read(std::pair<T, U> &t) {
    t.first = read<T>();
    t.second = read<U>();
  }
}

#endif
