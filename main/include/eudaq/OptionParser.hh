#ifndef EUDAQ_INCLUDED_OptionParser
#define EUDAQ_INCLUDED_OptionParser

#include "eudaq/Utils.hh"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iosfwd>
#include <iostream>

namespace eudaq {

  class OptionException : public std::runtime_error {
  public:
    OptionException(const std::string & msg) : std::runtime_error(msg) {}
  };

  class MessageException : public std::runtime_error {
  public:
    MessageException(const std::string & msg) : std::runtime_error(msg) {}
  };

  class OptionBase;

  class OptionParser {
  public:
    OptionParser(const std::string & name, const std::string & version, const std::string & desc="",
                 int minargs = -1, int maxargs = -1)
      : m_name(name), m_ver(version), m_desc(desc), m_minargs(minargs), m_maxargs(maxargs)
      {}
    OptionParser & Parse(const char ** args);
    OptionParser & Parse(char ** args) { return Parse(const_cast<const char **>(args)); }
    size_t NumArgs() const { return m_args.size(); }
    std::string GetArg(size_t i) const { return m_args[i]; }
    void ShowHelp(std::ostream &);

    void AddOption(OptionBase * opt);
    void ExtraHelpText(const std::string &);
    int HandleMainException(std::ostream & err = std::cerr, std::ostream & out = std::cout);
  private:
    void LogException(const std::string & msg, std::ostream & = std::cerr) const;
    std::string m_name, m_ver, m_desc, m_cmd, m_extra;
    std::vector<OptionBase *> m_options;
    std::map<std::string, size_t> m_names;
    std::vector<std::string> m_args;
    size_t m_minargs, m_maxargs;
  };

  class OptionBase {
  public:
    OptionBase(OptionParser & p, const std::string & shortname, const std::string & longname,
               const std::string & deflt = "", const std::string & argname = "", const std::string & desc = "")
      : m_argname(argname), m_deflt(deflt), m_desc(desc), m_hasarg(argname != ""), m_isset(false)
      {
        if (shortname != "") m_names.push_back("-" + shortname);
        if (longname  != "") m_names.push_back("--" + longname);
        p.AddOption(this);
      }
    virtual ~OptionBase() {}
    virtual void Print(std::ostream &) const;
    bool IsSet() const { return m_isset; }
    //bool HasArg() const { return m_hasarg; }
  private:
    friend class OptionParser;
    void ParseOption(const std::string & name, const std::string & arg) {
      DoParsing(name, arg);
      m_isset = true;
    }
    virtual void DoParsing(const std::string & name, const std::string & arg) = 0;

    std::vector<std::string> m_names;
    std::string m_argname, m_deflt, m_desc;
    bool m_hasarg, m_isset;
  };

  class OptionFlag : public OptionBase {
  public:
    OptionFlag(OptionParser & p, const std::string & shortname, const std::string & longname,
               const std::string & desc = "")
      : OptionBase(p, shortname, longname, "", "", desc)
      {}
    bool Value() const { return IsSet(); }
  private:
    virtual void DoParsing(const std::string & /*name*/, const std::string & /*arg*/) {
    }
  };

  template <typename T>
  class Option : public OptionBase {
  public:
    Option(OptionParser & p, const std::string & shortname, const std::string & longname,
           const T & deflt = T(), const std::string & argname = "", const std::string & desc = "")
      : OptionBase(p, shortname, longname, to_string(deflt), argname, desc), m_value(deflt)
      {}
    const T & Value() const { return m_value; }
    void SetValue(const T & val) { m_value = val; }
  private:
    virtual void DoParsing(const std::string & /*name*/, const std::string & arg) {
      m_value = from_string(arg, m_value);
    }
    T m_value;
  };

  template <typename T>
  class Option<std::vector<T> > : public OptionBase {
  public:
    Option(OptionParser & p, const std::string & shortname, const std::string & longname,
           const std::string & argname = "", const std::string & sep = "", const std::string & desc = "")
      : OptionBase(p, shortname, longname, "", argname, desc), m_sep(sep.length() ? sep : ",")
      {}
    const std::vector<T> & Value() const { return m_value; }
    void SetValue(const std::vector<T> & val = std::vector<T>()) { m_value = val; }
    void Resize(size_t size) { m_value.resize(size); }
    size_t NumItems() const { return m_value.size(); }
    const T & Item(size_t i) const { return m_value[i]; }
  private:
    virtual void DoParsing(const std::string & /*name*/, const std::string & arg) {
      size_t i = 0;
      do {
        size_t start = i;
        i = arg.find(m_sep, start);
        //std::cout << '"' << arg << '"' << ".find(" << m_sep << ", " << start << ") = " << i << std::endl;
        std::string valstr(arg, start, (i == std::string::npos ? arg.length() : i) - start);
        m_value.push_back(from_string(valstr, T()));
        if (i != std::string::npos) {
          i += m_sep.length();
        }
      } while (i != std::string::npos);
    }
    std::string m_sep;
    std::vector<T> m_value;
  };

}

#endif // EUDAQ_INCLUDED_OptionParser
