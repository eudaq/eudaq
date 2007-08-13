#include "eudaq/Configuration.hh"

#include <fstream>
#include <iostream>
#include <cstdlib>

namespace eudaq {

  Configuration::Configuration(const std::string & config, const std::string & section)
    : m_cur(&m_config[""]) {
    std::istringstream confstr(config);
    Load(confstr, section);
  }

  Configuration::Configuration(std::istream & conffile, const std::string & section)
  : m_cur(&m_config[""]) {
    Load(conffile, section);
  }

  void Configuration::Save(std::ostream & stream) const {
    for (map_t::const_iterator i = m_config.begin(); i != m_config.end(); ++i) {
      if (i->first != "") {
        stream << "[" << i->first << "]\n";
      }
      for (section_t::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
        stream << j->first << " = " << j->second << "\n";
      }
      stream << "\n";
    }
  }

  void Configuration::Load(std::istream & stream, const std::string & section) {
    map_t config;
    section_t * cur_sec = &config[""];
    for (;;) {
      std::string line;
      std::getline(stream, line);
      if (stream.eof()) break;
      size_t equals = line.find('=');
      if (equals == std::string::npos) {
        line = trim(line);
        if (line == "" || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[' && line[line.length()-1] == ']') {
          line = std::string(line, 1, line.length()-2);
          // TODO: check name is alphanumeric?
          //std::cerr << "Section " << line << std::endl;
          cur_sec = &config[line];
        }
      } else {
        std::string key = trim(std::string(line, 0, equals-1));
        // TODO: check key does not already exist
        // handle lines like: blah = "foo said ""bar""; ok." # not "baz"
        line = trim(std::string(line, equals+1));
        if ((line[0] == '\'' && line[line.length()-1] == '\'') ||
            (line[0] == '\"' && line[line.length()-1] == '\"')) {
          line = std::string(line, 1, line.length()-2);
        } else {
          size_t i = line.find_first_of(";#");
          if (i != std::string::npos) line = trim(std::string(line, 0, i-1));
        }
        //std::cerr << "Key " << key << " = " << line << std::endl;
        (*cur_sec)[key] = line;
      }
    }
    m_config = config;
    SetSection(section);
  }

  bool Configuration::SetSection(const std::string & section) const {
    map_t::const_iterator i = m_config.find(section);
    if (i == m_config.end()) return false;
    m_cur = const_cast<section_t*>(&i->second);
    return true;
  }

  bool Configuration::SetSection(const std::string & section) {
    m_cur = &m_config[section];
    return true;
  }

  std::string Configuration::Get(const std::string & key, const std::string & def) const {
    try {
      return GetString(key);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  double Configuration::Get(const std::string & key, double def) const {
    try {
      return from_string(GetString(key), def);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  long long Configuration::Get(const std::string & key, long long def) const {
    try {
      std::string s = GetString(key);
      return std::strtoll(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  int Configuration::Get(const std::string & key, int def) const {
    try {
      std::string s = GetString(key);
      return std::strtol(s.c_str(), 0, 0);
    } catch (const Exception &) {
      // ignore: return default
    }
    return def;
  }

  std::string Configuration::GetString(const std::string & key) const {
    section_t::const_iterator i = m_cur->find(key);
    if (i != m_cur->end()) {
      return i->second;
    }
    EUDAQ_THROW("Configuration: key not found");
  }

  void Configuration::SetString(const std::string & key, const std::string & val) {
    (*m_cur)[key] = val;
  }

}
