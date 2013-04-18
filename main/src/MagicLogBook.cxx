#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/TLUEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"

#include <fstream>
#include <iostream>

using eudaq::to_string;
using eudaq::from_string;
using eudaq::lcase;
using eudaq::Event;
using eudaq::DetectorEvent;
using eudaq::EUDRBEvent;
using eudaq::TLUEvent;

  struct Field {
    Field(const std::string & n, const std::string & we, const std::string & wa)
      : name(n), where(we), what(wa) {}
    std::string name, where, what;
  };

static const Field fields[] = {
  Field("Run", "run", ""),
  Field("Start", "bt", "STARTTIME"),
};

/*
   datafile

   From BORE:
   run number
   config name
   start time
   eudrb.boards .det .mode
   tlu.{dut|and|or|veto}mask .triggerinterval
   config[Producer.EUDRB].Unsynchronized .Thresh .Ped

   From EORE:
   stop time
   num events

   From log:
   Comment
   Start time
   Stop time
   num events
   errors/warnings

   From Data:
   synchronized
 */

class RunInfo {
  public:
    RunInfo(const std::string & ofile, const std::vector<std::string> & fields,
        const std::string & sep, const std::string & head)
      : m_fields(fields),
      m_sep(sep),
      m_file(ofile == "" ? 0 : new std::ofstream(ofile.c_str())),
      m_out(m_file ? *m_file : std::cout),
      m_events(0) {
        if (m_file && !m_file->is_open()) EUDAQ_THROWX(eudaq::FileWriteException,
            "Unable to open '" + ofile + "'");
        m_out << head;
        if (m_fields.size() > 0) m_out << getname(m_fields[0]);
        for (size_t i = 1; i < m_fields.size(); ++i) {
          m_out << m_sep << getname(m_fields[i]);
        }
        m_out << std::endl;
      }
    void Process(const std::string & fname) {
      m_des = new eudaq::FileDeserializer(fname, true);
      //std::cerr << "Processing " << fname << std::endl;
      try {
        m_bore = NextEvent();
      } catch (const eudaq::FileReadException &) {
        // probably empty file, ignore it
        return;
      }
      m_eore = 0;
      m_config = 0;
      //    m_dec = 0; //new eudaq::EUDRBDecoder(*m_bore);
      m_log = 0;
      m_events = 0;
      if (m_fields.size() > 0) m_out << GetField(m_fields[0]);
      for (size_t i = 1; i < m_fields.size(); ++i) {
        m_out << m_sep << GetField(m_fields[i]);
      }
      m_out << std::endl;
    }
    std::string GetField(const std::string & field) {
      std::string name = lcase(field), param;
      size_t i = field.find(':');
      if (i != std::string::npos) {
        name = lcase(std::string(field, 0, i));
        param = std::string(field, i+1);
      }
      if (name == "bore") {
        return GetTag(*m_bore, param);
      } else if (name == "eore") {
        if (!m_eore) GetEORE();
        return GetTag(*m_eore, param);
      } else if (name == "eudrb") {
        try {
          return GetTag(GetSubEvent<EUDRBEvent>(*m_bore), param);
        } catch (const eudaq::Exception &) {
          return "";
        }
      } else if (name == "tlu") {
        try {
          return GetTag(GetSubEvent<TLUEvent>(*m_bore), param);
        } catch (const eudaq::Exception &) {
          return "";
        }
      } else if (name == "config") {
        return GetConfig(param);
      } else if (name == "log") {
        return GetLog(param);
      } else if (name == "events") {
        if (!m_eore) GetEORE();
        return to_string(m_events);
      } else {
        EUDAQ_THROW("Unknown field: " + name);
      }
    }
    std::string GetConfig(const std::string & str) {
      if (!m_config) m_config = new eudaq::Configuration(m_bore->GetTag("CONFIG"));
      std::string section, name, fallback, def;
      size_t i = str.find(':');
      if (i != std::string::npos) {
        section = std::string(str, 0, i);
        name = std::string(str, i+1);
      }
      i = name.find(':');
      if (i != std::string::npos) {
        def = std::string(name, i+1);
        name = std::string(name, 0, i);
      }
      i = def.find(':');
      if (i != std::string::npos) {
        fallback = std::string(def, 0, i);
        def = std::string(def, i+1);
      }
      if (section == "" && name == "") {
        return m_config->Name();
      } else if (name == "") {
        EUDAQ_THROW("Must specify config:section:name[[:fallback]:default]");
      } else {
        m_config->SetSection(section);
        if (fallback == "") {
          return m_config->Get(name, def);
        } else {
          return m_config->Get(name, fallback, def);
        }
      }
    }
    std::string GetLog(const std::string & /*param*/) {
      if (!m_log) {
        m_log = new std::vector<eudaq::LogMessage>();
      }
      EUDAQ_THROW("Not yet implemented");
    }
    ~RunInfo() {
      delete m_file;
    }
  private:
    template <typename T>
      static const T & GetSubEvent(const DetectorEvent & dev) {
        for (size_t i = 0; i < dev.NumEvents(); i++) {
          const T * sev = dynamic_cast<const T*>(dev.GetEvent(i));
          if (sev) return *sev;
        }
        EUDAQ_THROW("Unable to get sub event");
      }
    std::string getname(std::string & str) {
      size_t i = str.find('=');
      if (i != std::string::npos) {
        std::string result(str, 0, i);
        str = str.substr(i+1);
        return result;
      }
      return str;
    }
    void GetEORE() {
      try {
        for (;;) {
          counted_ptr<DetectorEvent> dev = NextEvent();
          if (dev->IsEORE()) {
            m_eore = dev;
            break;
          } else if (!dev->IsBORE()) {
            m_events++;
          }
        }
      } catch (const eudaq::FileReadException &) {
        // unexpected end of file, ignore it
      }
    }
    std::string GetTag(const Event & evt, const std::string & tag) {
      std::string name = tag, fallback, def;
      if (name == ".Run") {
        return to_string(evt.GetRunNumber());
      } else if (name == ".Event") {
        return to_string(evt.GetEventNumber());
      } else if (name == ".Timestamp") {
        return to_string(evt.GetTimestamp());
      }
      size_t i = name.find(':');
      if (i != std::string::npos) {
        def = std::string(name, i+1);
        name = std::string(name, 0, i);
      }
      i = def.find(':');
      if (i != std::string::npos) {
        fallback = std::string(def, 0, i);
        def = std::string(def, i+1);
      }
      std::string result = evt.GetTag(name);
      if (result == "" && fallback != "") result = evt.GetTag(fallback);
      if (result == "") return def;
      return result;
    }
    counted_ptr<DetectorEvent> NextEvent() {
      Event * ev = eudaq::EventFactory::Create(*m_des);
      DetectorEvent * dev = dynamic_cast<DetectorEvent *>(ev);
      if (!dev) {
        delete ev;
        EUDAQ_THROW("Bad data file");
      }
      return counted_ptr<DetectorEvent>(dev);
    }
    std::vector<std::string> m_fields;
    std::string m_sep;
    std::ofstream * m_file;
    std::ostream & m_out;
    counted_ptr<eudaq::FileDeserializer> m_des;
    counted_ptr<DetectorEvent> m_bore, m_eore;
    counted_ptr<eudaq::Configuration> m_config;
    //  counted_ptr<eudaq::EUDRBDecoder> m_dec;
    counted_ptr<std::vector<eudaq::LogMessage> > m_log;
    int m_events;
};

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Magic Log Book", "1.0",
      "Uses powerful black magic to create the log book you forgot to write",
      1);
  eudaq::Option<std::vector<std::string> > fields(op, "f", "fields", "name", ",",
      "A list of fields to include in the output");
  eudaq::Option<std::string> sep(op, "s", "separator", "\t", "string",
      "String to separate output fields");
  eudaq::Option<std::string> head(op, "h", "header", "", "string",
      "String to precede the header line (blank=no header)");
  eudaq::Option<std::string> pdef(op, "p", "predefined", "", "name",
      "Predefined set of fields (normal, fast, full...)");
  eudaq::Option<std::string> ofile(op, "o", "output", "", "file",
      "File name for storing the output (default=stdout)");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(eudaq::Status::LVL_NONE);
    std::vector<std::string> flds;
    if (pdef.Value() == "normal") {
      std::string vals[] = {
        "Run=bore:.Run",
        "Config=config",
        "Mode=eudrb:MODE",
        "Det=eudrb:DET",
        "Start=bore:STARTTIME",
        "Unsync=config:Producer.EUDRB:Unsynchronized",
        "Planes=config:Producer.EUDRB:NumBoards",
        "TriggerInterval=tlu:TriggerInterval",
        "AndMask=tlu:AndMask",
        "DUTMask=tlu:DutMask",
        "TLUfw=tlu:FirmwareID",
        "EUDRBfw=eudrb:VERSION",
      };
      flds.insert(flds.end(), vals, vals + sizeof vals / sizeof *vals);
    } else if (pdef.Value() == "full") {
      std::string vals[] = {
        "Run=bore:.Run",
        "Config=config",
        "Mode=eudrb:MODE",
        "Det=eudrb:DET",
        "Start=bore:STARTTIME",
        "Events=events",
        "Unsync=config:Producer.EUDRB:Unsynchronized",
        "Planes=config:Producer.EUDRB:NumBoards",
        "TriggerInterval=tlu:TriggerInterval",
        "AndMask=tlu:AndMask",
        "DUTMask=tlu:DutMask",
        "TLUfw=tlu:FirmwareID",
        "EUDRBfw=eudrb:VERSION",
      };
      flds.insert(flds.end(), vals, vals + sizeof vals / sizeof *vals);
    } else if (pdef.Value() != "") {
      EUDAQ_THROW("Unknown predefined fields: " + pdef.Value());
    }
    flds.insert(flds.end(), fields.Value().begin(), fields.Value().end());
    RunInfo info(ofile.Value(), flds, sep.Value(), head.Value());
    //EUDAQ_LOG_LEVEL("INFO");
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string datafile = op.GetArg(i);
      if (datafile.find_first_not_of("0123456789") == std::string::npos) {
        info.Process("../data/run" + to_string(from_string(datafile, 0), 6) + ".raw");
      } else if (datafile.find_first_not_of("0123456789-") == std::string::npos) {
        size_t hyphen = datafile.find('-');
        if (datafile.find('-', hyphen+1) != std::string::npos) throw eudaq::OptionException("Bad range: " + datafile);
        int first = from_string(std::string(datafile, 0, hyphen), 0);
        int last = from_string(std::string(datafile, hyphen+1), 0);
        if (last != 0 && last < first) throw eudaq::OptionException("Bad range: " + datafile);
        for (int i = first; i <= last || last == 0; ++i) {
          try {
            info.Process("../data/run" + to_string(i, 6) + ".raw");
          } catch (const eudaq::FileNotFoundException &) {
            if (last != 0) continue;
            throw;
          }
        }
      } else {
        info.Process(datafile);
      }
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
