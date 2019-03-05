#include "eudaq/Configuration.hh"
#include "eudaq/Platform.hh"

#include <fstream>
#include <iostream>
#include <cstdlib>

namespace eudaq {

  Configuration::Configuration(const std::string &config,
                               const std::string &section)
      : m_cur(&m_config[""]) {
    std::istringstream confstr(config);
    Load(confstr, section);
  }

  Configuration::Configuration(std::istream &conffile,
                               const std::string &section)
      : m_cur(&m_config[""]) {
    Load(conffile, section);
  }

  Configuration::Configuration(const Configuration &other)
      : m_config(other.m_config) {
    SetSection(other.m_section);
  }

  Configuration::Configuration(const Configuration &other, const std::string &section){
    auto it = other.m_config.find("");
    if(it != other.m_config.end())
      m_config[""] = it->second;
    else
      m_config[""];
    for(auto &e: other.m_config){
      if(e.first == section){
    m_config[section] = e.second;
    SetSection(section);
      }
    }

  }

  std::unique_ptr<Configuration> Configuration::MakeUniqueReadFile(const std::string &path){
    std::unique_ptr<Configuration> conf;
    std::ifstream file(path);
    if(file.is_open()){
      conf.reset(new Configuration(file));
      conf->Set("Name", path);
    }
    return conf;
  }

  std::string Configuration::Name() const {
    map_t::const_iterator it = m_config.find("");
    if (it == m_config.end())
      return "";
    section_t::const_iterator it2 = it->second.find("Name");
    if (it2 == it->second.end())
      return "";
    return it2->second;
  }

  void Configuration::Save(std::ostream &stream) const {
    for (map_t::const_iterator i = m_config.begin(); i != m_config.end(); ++i) {
      if (i->first != "") {
        stream << "[" << i->first << "]\n";
      }
      for (section_t::const_iterator j = i->second.begin();
           j != i->second.end(); ++j) {
        stream << j->first << " = " << j->second << "\n";
      }
      stream << "\n";
    }
  }

  Configuration &Configuration::operator=(const Configuration &other) {
    m_config = other.m_config;
    SetSection(other.m_section);
    return *this;
  }

  void Configuration::Load(std::istream &stream, const std::string &section) {
    map_t config;
    section_t *cur_sec = &config[""];
    for (;;) {
      std::string line;
      if (stream.eof())
        break;
      std::getline(stream, line);
      size_t equals = line.find('=');
      if (equals == std::string::npos) {
        line = trim(line);
        if (line == "" || line[0] == ';' || line[0] == '#')
          continue;
        if (line[0] == '[' && line[line.length() - 1] == ']') {
          line = std::string(line, 1, line.length() - 2);
      cur_sec = &config[line];
        }
      } else {
        std::string key = trim(std::string(line, 0, equals));
        line = trim(std::string(line, equals + 1));
        if ((line[0] == '\'' && line[line.length() - 1] == '\'') ||
            (line[0] == '\"' && line[line.length() - 1] == '\"')) {
          line = std::string(line, 1, line.length() - 2);
        } else {
          size_t i = line.find_first_of(";#");
          if (i != std::string::npos)
            line = trim(std::string(line, 0, i));
        }
        (*cur_sec)[key] = line;
      }
    }

    if(section.empty()){
      m_config = config;
      SetSection(section);
    }
    else{
        m_config[""] = config[""];//the un-named section is always copied.
        for(auto &e: config){
            if(e.first == section){
                m_config[section] = e.second;
                SetSection(section);
            }
        }
    }
  }

  bool Configuration::SetSection(const std::string &section) const {
    map_t::const_iterator i = m_config.find(section);
    if (i == m_config.end())
      return false;
    m_section = section;
    m_cur = const_cast<section_t *>(&i->second);
    return true;
  }

  bool Configuration::SetSection(const std::string &section) {
    m_section = section;
    m_cur = &m_config[section];
    return true;
  }

  std::string Configuration::Get(const std::string &key,
                                 const std::string &def) const {
    try {
      return GetString(key);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  double Configuration::Get(const std::string &key, double def) const {
    try {
      return from_string(GetString(key), def);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  int64_t Configuration::Get(const std::string &key, int64_t def) const {
    try {
      std::string s = GetString(key);
      return std::strtoll(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }
  uint64_t Configuration::Get(const std::string &key, uint64_t def) const {
    try {
      std::string s = GetString(key);
      return std::strtoull(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  int Configuration::Get(const std::string &key, int def) const {
    try {
      std::string s = GetString(key);
      return std::strtol(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  void Configuration::Print(std::ostream &os, size_t offset) const {
    os << std::string(offset, ' ') << "<Configuration>\n";
    for(auto &sect: m_config){
      os << std::string(offset + 2, ' ') << "<Section title=\""<< sect.first<<"\">\n";
      for(auto &key: sect.second){
    os << std::string(offset + 4, ' ') << "<"<<key.first<< ">"<< key.second<< "</"<<key.first <<">\n";
      }
      os << std::string(offset + 2, ' ') << "</Section>\n";
    }
    os << std::string(offset, ' ') << "</Configuration>\n";
  }

  void Configuration::Print() const { Print(std::cout); }

  std::string Configuration::GetString(const std::string &key) const {
    section_t::const_iterator i = m_cur->find(key);
    if (i != m_cur->end()) {
      return i->second;
    }
    throw Exception("Configuration: key not found");
  }

  bool Configuration::Has(const std::string& key) const {
      return m_cur->find(key) != m_cur->cend();
  }

  bool Configuration::HasSection(const std::string &section) const
  {
    return m_config.find(section) != m_config.end();
  }

  void Configuration::SetString(const std::string &key,
                                const std::string &val) {
    (*m_cur)[key] = val;
  }
}
