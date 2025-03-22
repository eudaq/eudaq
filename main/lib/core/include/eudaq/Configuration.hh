#ifndef EUDAQ_INCLUDED_Configuration
#define EUDAQ_INCLUDED_Configuration

#include "Utils.hh"
#include "Exception.hh"
#include "Platform.hh"
#include <functional>
#include <string>
#include <map>

namespace eudaq {
  class Configuration;
  
  using ConfigurationUP = std::unique_ptr<Configuration, std::function<void(Configuration*)> >;
  using ConfigurationSP = std::shared_ptr<Configuration>;
  using ConfigurationWP = std::weak_ptr<Configuration>;
  using ConfigurationSPC = std::shared_ptr<const Configuration>;

  using ConfigUP = ConfigurationUP;
  using ConfigSP = ConfigurationSP;
  using ConfigWP = ConfigurationWP;
  using ConfigSPC = ConfigurationSPC;
  
  class DLLEXPORT Configuration {
  public:
    Configuration(const std::string &config = "",
                  const std::string &section = "");
    Configuration(std::istream &conffile, const std::string &section = "");
    Configuration(const Configuration &other);
    Configuration(const Configuration &other, const std::string &section);

    static std::unique_ptr<Configuration> MakeUniqueReadFile(const std::string &path);


    /**
     * @brief Check if key is defined
     * @param key Key to check for existence
     * @return True if key exists, false otherwise
     */
    bool Has(const std::string& key) const;

    /**
     * @brief Check if config file has section
     * @param section
     * @return True if section exists, false otherwise
     */
    bool HasSection(const std::string & section) const;

    /**
     * @brief get the list of keys in a section
     * @return list of keys in section
     */
    std::vector<std::string> Keylist() const;

    /**
     * @brief get the list of sections in the configuration
     * @return list of sections in configuration
     */
    std::vector<std::string> Sectionlist() const;

    void Save(std::ostream &file) const;
    void Load(std::istream &file, const std::string &section);
    bool SetSection(const std::string &section) const;
    bool SetSection(const std::string &section);
    std::string GetCurrentSectionName() const {return m_section;};
    std::string operator[](const std::string &key) const {
      std::string retval;
      if(GetString(key,retval)) return retval;
      throw Exception("unable to find key "+key);
    }
    std::string Get(const std::string &key, const std::string &def) const;
    float Get(const std::string &key, float def) const;
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

  private:
    bool GetString(const std::string &key, std::string& value) const;
    typedef std::map<std::string, std::string> section_t;
    typedef std::map<std::string, section_t> map_t;
    map_t m_config;
    mutable std::string m_section;
    mutable section_t *m_cur;
  };

  inline std::ostream &operator<<(std::ostream &os, const Configuration &c) {
    c.Save(os);
    return os;
  }

  template <typename T>
  inline void Configuration::Set(const std::string &key, const T &val) {
    SetString(key, to_string(val));
  }
}

#endif // EUDAQ_INCLUDED_Configuration
