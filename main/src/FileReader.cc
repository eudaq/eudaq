#include "eudaq/FileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"

#include <list>

#define DBGT(a) std::cout << "DEBUG: " << a << std::endl;
#define DBGB(a) DBGT(#a << " = " << ((a)?"yes":"no"))
#define DBG(a) DBGT(#a << " = " << a)

namespace eudaq {

  struct FileReader::eventqueue_t;
  std::ostream & operator << (std::ostream & os, const FileReader::eventqueue_t & q);
  struct FileReader::eventqueue_t {
    struct item_t {
      item_t(DetectorEvent * ev = 0) : event(ev) {
        if (ev) {
          for (size_t i = 0; i < ev->NumEvents(); ++i) {
            triggerids.push_back(PluginManager::GetTriggerID(*ev->GetEvent(i)));
          }
        }
      }
      eudaq::DetectorEvent * event;
      std::vector<unsigned>  triggerids;
    };
    eventqueue_t(unsigned numproducers = 0) : offsets(numproducers, items.end())/*, expected(0)*/ {}
    bool isempty(bool dbg = false) const {
      for (size_t i = 0; i < offsets.size(); ++i) {
	if (dbg) std::cout << "i " << i << std::flush;
        //if (offsets[i] == items.begin()) {
        if (events(i, dbg*((i==2)+1)) == 0) {
	  if (dbg) std::cout << "Empty" << std::endl;
          return true;
        }
      }
      if (dbg) std::cout << "not empty" << std::endl;
      return false;
    }
    size_t events(size_t producer, int dbg = 0) const {
      std::list<item_t>::const_iterator it = iter(producer, -1);
      if (dbg) std::cout << "." << std::endl;
      if (it == items.begin()) return 0;
      return std::distance(items.begin(), it);
    }
    size_t fullevents() const {
      size_t min = events(0);
      for (size_t i = 1; i < offsets.size(); ++i) {
	size_t evts = events(i);
	if (evts < min) min = evts;
      }
      return min;
    }
    void push(eudaq::Event * ev) {
      DetectorEvent * dev = dynamic_cast<DetectorEvent *>(ev);
      items.push_front(item_t(dev));
    }
    void discardevent(size_t producer) {
      --offsets[producer];
    }
    int clean_back() {
      int result = 0;
      bool done = false;
      while (!done) {
	for (size_t i = 0; i < producers(); ++i) {
	  std::list<item_t>::const_iterator it = offsets.at(i);
	  if (it == items.end()) return result;
	  ++it;
	  if (it == items.end()) {
	    done = true;
	    offsets[i] = items.end();
	  }
	}
	++result;
	items.pop_back();
      }
      return result;
    }
    eudaq::DetectorEvent * popevent() {
      unsigned run = getevent(0).GetRunNumber();
      unsigned evt = getevent(0).GetEventNumber();
      unsigned long long ts = NOTIMESTAMP;
      for (size_t i = 0; i < producers(); ++i) {
	if (getevent(i).get_id() == eudaq::Event::str2id("_TLU")) {
	  run = getevent(i).GetRunNumber();
	  evt = getevent(i).GetEventNumber();
	  ts = getevent(i).GetTimestamp();
	  break;
	}
      }
      //std::cout << "dev " << run << ", " << evt << ", " << ts << std::endl;
      DetectorEvent * dev = new DetectorEvent(run, evt, ts);
      for (size_t i = 0; i < producers(); ++i) {
	dev->AddEvent(iter(i)->event->GetEventPtr(i));
      }
      bool more = true;
      do {
	//std::cout << "DBG2a " << (*this) << std::endl;
	//std::cout << "*** " << std::distance(items.begin(), items.end()) << std::endl;
	for (size_t i = 0; i < producers(); ++i) {
	  std::list<item_t>::const_iterator it = iter(i, -1);
	  if (0) std::cout << i << ": "
		    << (it == items.begin() ? "b":".")
		    << (it == items.end() ? "e":".")
		    << " " << std::flush
		    << std::distance(((const std::list<item_t> &)items).begin(), it)
		    << ", "
		    << std::distance(it, ((const std::list<item_t> &)items).end())
		    << std::endl;
	}
	clean_back();
	//std::cout << "*** " << std::distance(items.begin(), items.end()) << std::endl;
	for (size_t i = 0; i < producers(); ++i) {
	  //std::cout << "foo" << std::endl;
	  std::list<item_t>::const_iterator it = iter(i, -1); //, true);
	  //std::cout << "bar" << std::endl;
	  if (0) std::cout << i << ": "
		    << (it == items.begin() ? "b":".")
		    << (it == items.end() ? "e":".")
		    << " " << std::flush
		    << std::distance(((const std::list<item_t> &)items).begin(), it)
		    << ", " << std::flush
		    << std::distance(it, ((const std::list<item_t> &)items).end())
		    << std::endl;
	}
	//std::cout << "DBG2b " << std::flush;
	//debug(std::cout, true);
	//std::cout << std::endl;
	more = true;
	for (size_t i = 0; i < producers(); ++i) {
	  if (iter(i, -1) == items.end()) {
	    more = false;
	    break;
	  }
	}
      } while (more);
      //std::cout << "hmm" << std::endl;
      return dev;
    }
    void debug(std::ostream & os, bool moredbg = false) const {
      os << "empty=" << (isempty(moredbg)?"yes":"no") << std::flush;
      os << " fullevents=" << fullevents() << std::flush
	 << " events=" << events(0) << std::flush;
      for (size_t i = 1; i < producers(); ++i) {
	os << "," << events(i) << std::flush;
      }
    }
    std::list<item_t>::const_iterator iter(size_t producer, int offset = 0, bool dbg = false) const {
      std::list<item_t>::const_iterator it = offsets.at(producer);
      //std::advance(it, -offset - 1);
      if (dbg) std::cout << "iter " << producer << ", " << offset << std::endl;
      for (int i = 0; i <= offset; ++i) {
	if (dbg) std::cout << "..." << std::endl;
	if (it == items.begin()) EUDAQ_THROW("Bad offset in ResyncTLU routine");
	--it;
      }
      if (dbg) std::cout << "ok" << std::endl;
      return it;
    }
    unsigned getid(size_t producer) const {
      return iter(producer)->triggerids[producer];
    }
    const eudaq::Event & getevent(size_t producer, int offset = 0) const {
      return *iter(producer, offset)->event->GetEvent(producer);
    }
    unsigned producers() const {
      return offsets.size();
    }
    std::list<item_t> items;
    std::vector<std::list<item_t>::const_iterator> offsets;
    //unsigned expected;
  };

  std::ostream & operator << (std::ostream & os, const FileReader::eventqueue_t & q) {
    q.debug(os);
    return os;
  }

  namespace {

    static bool ReadEvent(FileDeserializer & des, int ver, eudaq::Event * & ev, size_t skip = 0) {
      if (!des.HasData()) {
        return false;
      }
      if (ver < 2) {
        for (size_t i = 0; i <= skip; ++i) {
          if (!des.HasData()) break;
          ev = EventFactory::Create(des);
        }
      } else {
        BufferSerializer buf;
        for (size_t i = 0; i <= skip; ++i) {
          if (!des.HasData()) break;
          des.read(buf);
        }
        ev = eudaq::EventFactory::Create(buf);
      }
      return true;
    }

    static bool SyncEvent(FileReader::eventqueue_t & queue, FileDeserializer & des, int ver, eudaq::Event * & ev) {
      //DBGT("");
      static const int MAXTRIES = 3;
      for (int itry = 0; itry < MAXTRIES; ++itry) {
	//std::cout << "DBGa " << queue << std::endl;
        if (queue.isempty()) {
          eudaq::Event * ev = 0;
          if (!ReadEvent(des, ver, ev)) {
            break;
          }
          queue.push(ev);
        }
	//std::cout << "DBGb " << queue << std::endl;
	bool isbore = queue.getevent(0).IsBORE();
	//DBGB(isbore);
	bool iseore = queue.getevent(0).IsEORE();
	//DBGB(iseore);
	bool haszero = PluginManager::GetTriggerID(queue.getevent(0)) == 0;
	//DBGB(haszero);
	bool hasother = false;
	unsigned eventnum = PluginManager::GetTriggerID(queue.getevent(0));
	//DBG(eventnum);
	//DBG("event " << eventnum);
	for (size_t i = 1; i < queue.producers(); ++i) {
	  //std::cout << "blah " << i << std::endl;
	  unsigned evnum = PluginManager::GetTriggerID(queue.getevent(i));
	  //DBG(evnum);
	  //DBG(eventnum << ", " << evnum);
	  if (!queue.getevent(i).IsBORE()) isbore = false;
	  if (!queue.getevent(i).IsEORE()) iseore = false;
	  if (evnum == 0) haszero = true;
	  if (eventnum == 0) eventnum = evnum;
	  if (evnum != 0 && evnum != eventnum) hasother = true;
	}
	if (isbore || iseore || (!haszero && !hasother) || eventnum == 0) {
	  //std::cout << "OK" << std::endl;
	  ev = queue.popevent();
	  return true;
	}
	//DBG(eventnum);
	//DBGB(isbore);
	//DBGB(iseore);
	//DBGB(haszero);
	//DBGB(hasother);
	if (hasother) {
	  EUDAQ_THROW("Unable to synchronize - please report this run to the EUDAQ developers.");
	}
	//std::cout << "DBGc " << queue << std::endl;
        if (queue.fullevents() < 2) {
          eudaq::Event * ev = 0;
          if (!ReadEvent(des, ver, ev)) {
            break;
          }
          queue.push(ev);
        }
	//std::cout << "DBGd " << queue << std::endl;
	for (size_t i = 0; i < queue.producers(); ++i) {
	  unsigned evnum = PluginManager::GetTriggerID(queue.getevent(i));
	  if (evnum == 0) {
	    unsigned evnum1 = PluginManager::GetTriggerID(queue.getevent(i, 1));
	    if (evnum1 == eventnum) {
	      EUDAQ_INFO("Discarded extra 'zero' event");
	      queue.discardevent(i);
	    } else if (evnum1 == (eventnum + 1) & 0x7fff) {
	      EUDAQ_INFO("Detected 'zero' event");
	    } else {
	      EUDAQ_THROW("Unable to synchronize 'zero' event - please report this run to the EUDAQ developers.");
	    }
	  }
	}
	ev = queue.popevent();
	return true;
      }
      EUDAQ_WARN("Unable to synchronize after " + to_string(MAXTRIES) + " events.");
      return false;
    }

  }

  FileReader::FileReader(const std::string & file, const std::string & filepattern, bool synctriggerid)
    : m_filename(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
      m_des(m_filename),
      m_ev(EventFactory::Create(m_des)),
      m_ver(1),
      m_queue(0) {
    //unsigned versiontag = m_des.peek<unsigned>();
    //if (versiontag == Event::str2id("VER2")) {
    //  m_ver = 2;
    //  m_des.read(versiontag);
    //} else if (versiontag != Event::str2id("_DET")) {
    //  EUDAQ_WARN("Unrecognised native file (tag=" + Event::id2str(versiontag) + "), assuming version 1");
    //}
    //EUDAQ_INFO("FileReader, version = " + to_string(m_ver));
    //NextEvent();
    if (synctriggerid) {
      m_queue = new eventqueue_t(Event().NumEvents());
    }
    //DBGB(synctriggerid);
    //DBGB(m_queue);
  }

  FileReader::~FileReader() {
    delete m_queue;
  }

  bool FileReader::NextEvent(size_t skip) {
    eudaq::Event * ev = 0;
    if (m_queue) {
      bool result = false;
      for (size_t i = 0; i <= skip; ++i) {
        if (!SyncEvent(*m_queue, m_des, m_ver, ev)) break;
        result = true;
      }
      if (ev) m_ev = ev;
      return result;
    }
    bool result = ReadEvent(m_des, m_ver, ev, skip);
    if (ev) m_ev = ev;
    return result;
  }

  unsigned FileReader::RunNumber() const {
    return m_ev->GetRunNumber();
  }

  const DetectorEvent & FileReader::Event() const {
    return dynamic_cast<const DetectorEvent &>(*m_ev);
  }

//   const StandardEvent & FileReader::GetStandardEvent() const {
//     if (!m_sev) {
//       counted_ptr<StandardEvent> sevent(new StandardEvent);
//       const DetectorEvent & dev = GetDetectorEvent();
//       for (size_t i = 0; i < dev.NumEvents(); ++i) {
//         const eudaq::Event * subevent = dev.GetEvent(i);

//         try {
//           const DataConverterPlugin * converterplugin = PluginManager::GetInstance().GetPlugin(subevent->GetType());
//           converterplugin->GetStandardSubEvent(*sevent, *subevent);
//           //std::fprintf(m_file, "Event %d %d\n", devent.GetEventNumber(), standardevent->m_x.size());
//         } catch(eudaq::Exception & e) {
//           //std::cout <<  e.what() << std::endl;
//           std::cout <<  "FileWriterText::WriteEvent(): Ignoring event type "
//                     <<  subevent->GetType() << std::endl;
//           continue;
//         }
//       }
//       m_sev = sevent;
//     }
//     return *m_sev;
//   }

}
