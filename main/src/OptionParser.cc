#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Time.hh"
#include <ostream>
#include <sstream>
#include <fstream>

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

  void OptionParser::ExtraHelpText(const std::string & str) {
    m_extra += str;
  }

  void OptionParser::ShowHelp(std::ostream & os) {
    os << m_name << " version " << m_ver << "\n";
    if (m_desc != "") os << m_desc << "\n\n";
    os << "usage: " << m_cmd << " [options]";
    if (m_minargs != (size_t)-1 && m_maxargs != 0) {
      os << " [" << m_minargs;
      if (m_maxargs == (size_t)-1) {
        os << " or more";
      } else if (m_maxargs != m_minargs) {
        os << "-" << m_maxargs;
      }
      os << ((m_minargs == 1 && m_maxargs == 1) ? " argument]" : " arguments]");
    }
    os << "\n\noptions:\n";
    for (size_t i = 0; i < m_options.size(); ++i) {
      os << " " << *m_options[i] << "\n";
    }
    if (m_extra.length()) {
      os << "\n" << m_extra << "\n";
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

  void OptionParser::LogException(const std::string & msg, std::ostream & os) const {
    {
      std::ofstream logfile("crashlog.txt", std::ios::app);
      logfile << "===============================================\n"
        << "Abnormal exit of " << m_name << "(" << m_ver << ") at " << Time::Current().Formatted() << "\n"
        << msg << "\n"
        << "-----------------------------------------------" << std::endl;
    }
    GetLogger().SendLogMessage(LogMessage(msg, LogMessage::LVL_ERROR), false);
    os << msg << "\n"
      << "Please report this to the developers." << std::endl;
    //std::string str;
    //std::getline(std::cin, str);
  }

  int OptionParser::HandleMainException(std::ostream & err, std::ostream & out) {
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
      LogException("Uncaught exception:\n" + std::string(e.what()));
      return 1;
    } catch (...) {
      LogException("Unknown exception");
      return 1;
    }
    return 0;
  }

}
