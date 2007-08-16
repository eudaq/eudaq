#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include <ostream>
#include <sstream>

namespace eudaq {

  inline std::ostream & operator << (std::ostream & os, const OptionBase & opt) {
    opt.Print(os);
    return os;
  }

  void OptionParser::AddOption(OptionBase * opt) {
    m_options.push_back(opt);
    for (size_t i = 0; i < opt->m_names.size(); ++i) {
      m_names[opt->m_names[i]] = m_options.size() - 1;
    }
  }

  OptionParser & OptionParser::Parse(const char ** args) {
    m_cmd = *args;
    bool argsonly = false;
    while (*++args) {
      std::string str = *args;
      if (argsonly) {
        if (m_minargs == (size_t)-1 || (m_maxargs != (size_t)-1 && m_args.size() >= m_maxargs))
          throw OptionException("Too many arguments: " + str);
        m_args.push_back(str);
      } else {
        if (str == "--") {
          argsonly = true;
          continue;
        }
        std::map<std::string, size_t>::const_iterator it = m_names.find(str);
        if (it == m_names.end()) {
          if (str == "-h" || str == "--help") {
            std::ostringstream s;
            ShowHelp(s);
            throw MessageException(s.str());
          } else if (str == "-v" || str == "--version") {
            throw MessageException(m_name + " version " + m_ver);
          }
          if (str[0] == '-') throw OptionException("Unrecognised option: " + str);
          if (m_minargs == (size_t)-1 || (m_maxargs != (size_t)-1 && m_args.size() >= m_maxargs))
            throw OptionException("Too many arguments: " + str);
          m_args.push_back(str);
        } else {
          std::string arg = "";
          if (m_options[it->second]->m_hasarg) {
            if (!*++args) throw OptionException("Missing argument for option: " + str);
            arg = *args;
          }
          m_options[it->second]->ParseOption(it->first, arg);
        }
      }
    }
    if ((m_minargs == (size_t)-1 && m_args.size() > 0) ||
        (m_minargs != (size_t)-1 && m_args.size() < m_minargs))
      throw OptionException("Not enough arguments");
    return *this;
  }

  void OptionParser::ShowHelp(std::ostream & os) {
    os << m_name << " version " << m_ver << "\n";
    if (m_desc != "") os << m_desc << "\n\n";
    os << "usage: " << m_cmd << " [options]";
    if (m_minargs != (size_t)-1 && m_maxargs != 0) {
      os << " [" << m_minargs;
      if (m_maxargs == (size_t)-1) {
        os << " or more";
      } else {
        os << "-" << m_maxargs;
      }
      os << " arguments]";
    }
    os << "\n\noptions:\n";
    for (size_t i = 0; i < m_options.size(); ++i) {
      os << " " << *m_options[i] << "\n";
    }
  }

  void OptionBase::Print(std::ostream & os) const {
    for (size_t j = 0; j < m_names.size(); ++j) {
      os << " " << m_names[j];
    }
    if (m_hasarg) {
      os << " <" << m_argname << ">\t(default = " << m_deflt << ")";
    }
    if (m_desc != "") os << "\n     " << m_desc;
  }

  int OptionParser::HandleMainException(std::ostream & err, std::ostream & out) {
    //if (!err) err = &std::cerr;
    //if (!out) err = &std::cout;
    try {
      throw;
    } catch (const eudaq::MessageException & e) {
      out << e.what() << std::endl;
    } catch (const eudaq::OptionException & e) {
      err << e.what() << "\n\n";
      ShowHelp(err);
      err << std::endl;
      return 1;
    } catch (const std::exception & e) {
      out << "Uncaught Exception:\n" << e.what() << std::endl;
      EUDAQ_ERROR(std::string("Uncaught exception: ") + e.what());
      return 1;
    } catch (...) {
      out << "Unknown Exception." << std::endl;
      EUDAQ_ERROR("Unknown exception");
      return 1;
    }
    return 0;
  }

}
