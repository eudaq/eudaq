#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "eudaq/Configuration.hh"

namespace py = pybind11;

class PyConfiguration : public eudaq::Configuration{
public:
  using eudaq::Configuration::Configuration;
/*
  class DLLEXPORT Configuration {
  public:
    Configuration(const std::string &config = "",
                  const std::string &section = "");
    Configuration(std::istream &conffile, const std::string &section = "");
    Configuration(const Configuration &other);
    Configuration(const Configuration &other, const std::string &section);

    static std::unique_ptr<Configuration> MakeUniqueReadFile(const std::string &path);


    bool Has(const std::string& key) const;
    bool HasSection(const std::string & section) const;

    std::vector<std::string> Keylist() const;

    void Save(std::ostream &file) const;
    void Load(std::istream &file, const std::string &section);
    bool SetSection(const std::string &section) const;
    bool SetSection(const std::string &section);
    std::string GetCurrentSectionName() const {return m_section;};
    std::string operator[](const std::string &key) const {
      return GetString(key);
    }
    std::string Get(const std::string &key, const std::string &def) const;
    double Get(const std::string &key, double def) const;
    int64_t Get(const std::string &key, int64_t def) const;
    uint64_t Get(const std::string &key, uint64_t def) const;
    template <typename T> T Get(const std::string &key, T def) const {
      return eudaq::from_string(Get(key, to_string(def)), def);
    }
    int Get(const std::string &key, int def) const;
    template <typename T>
    T Get(const std::string &key, const std::string fallback,
          const T &def) const {
      return Get(key, Get(fallback, def));
    }
    std::string Get(const std::string &key, const char *def) const {
      std::string ret(Get(key, std::string(def)));
      return ret;
    }
    std::string Get(const std::string &key, const std::string fallback,
                    std::string def) const {
      return Get(key, Get(fallback, def));
    }
    // std::string Get(const std::string & key, const std::string & def = "");
    template <typename T> void Set(const std::string &key, const T &val);
    std::string Name() const;
    Configuration &operator=(const Configuration &other);
    void Print(std::ostream &os, size_t offset=0) const;
    void Print() const;

    void SetString(const std::string &key, const std::string &val);
*/
};

void init_pybind_configuration(py::module &m){
  py::class_<eudaq::Configuration, eudaq::ConfigurationSP> configuration_(m, "Configuration");
  configuration_.def("Keylist", &eudaq::Configuration::Keylist);
  configuration_.def("Get",(std::string(eudaq::Configuration::*)(const std::string&,const char*)const)(&eudaq::Configuration::Get),py::arg("key"),py::arg("default")="");
  configuration_.def("as_dict",[](const eudaq::Configuration &c){
    std::map<std::string,std::string> m;
    for (auto k:c.Keylist()) m[k]=c.Get(k,"");
    return m;
  });
}

