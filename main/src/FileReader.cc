#include "eudaq/FileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Event.hh"
#include "eudaq/Logger.hh"

#include <list>

namespace eudaq {

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
    eventqueue_t(unsigned numproducers = 0) : offsets(numproducers, items.end()), expected(0) {}
    bool isempty() const {
      for (size_t i = 0; i < offsets.size(); ++i) {
        if (offsets[i] == items.begin()) {
          return true;
        }
      }
      return false;
    }
    size_t events(size_t producer) const {
      return std::distance(items.begin(), iter(producer));
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
      DetectorEvent * dev = new DetectorEvent(run, evt, ts);
      for (size_t i = 0; i < producers(); ++i) {
	dev->AddEvent(iter(i)->event->GetEventPtr(i));
      }
      bool more = true;
      do {
	items.pop_back();
	more = true;
	for (size_t i = 0; i < producers(); ++i) {
	  if (iter(i) == items.end()) {
	    more = false;
	    break;
	  }
	}
      } while (more);
      return dev;
    }
    std::list<item_t>::const_iterator iter(size_t producer, int offset = 0) const {
      std::list<item_t>::const_iterator it = offsets[producer];
      std::advance(it, offset - 1);
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
    unsigned expected;
  };

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
      static const int MAXTRIES = 3;
      for (int itry = 0; itry < MAXTRIES; ++itry) {
        if (queue.isempty()) {
          eudaq::Event * ev = 0;
          if (!ReadEvent(des, ver, ev)) {
            break;
          }
          queue.push(ev);
        }
	bool isbore = queue.getevent(0).IsBORE();
	bool iseore = queue.getevent(0).IsEORE();
	bool haszero = PluginManager::GetTriggerID(queue.getevent(0)) == 0;
	bool hasother = false;
	unsigned eventnum = PluginManager::GetTriggerID(queue.getevent(0));
	for (size_t i = 1; i < queue.producers(); ++i) {
	  unsigned evnum = PluginManager::GetTriggerID(queue.getevent(i));
	  if (!queue.getevent(i).IsBORE()) isbore = false;
	  if (!queue.getevent(i).IsEORE()) iseore = false;
	  if (evnum == 0) haszero = true;
	  if (eventnum == 0) eventnum = evnum;
	  if (eventnum != 0 && eventnum != evnum) hasother = true;
	}
	if (isbore || iseore || (!haszero && !hasother)) {
	  ev = queue.popevent();
	}
	if (hasother) {
	  EUDAQ_THROW("Unable to synchronize - please report this run to the EUDAQ developers.");
	}
        if (queue.fullevents() < 2) {
          eudaq::Event * ev = 0;
          if (!ReadEvent(des, ver, ev)) {
            break;
          }
          queue.push(ev);
        }
	for (size_t i = 1; i < queue.producers(); ++i) {
	  unsigned evnum = PluginManager::GetTriggerID(queue.getevent(i));
	  if (evnum == 0) {
	    unsigned evnum1 = PluginManager::GetTriggerID(queue.getevent(i, 1));
	    if (evnum1 == eventnum) {
	      EUDAQ_INFO("Discarded extra 'zero' event");
	      queue.discardevent(i);
	    } else if (evnum1 == eventnum + 1) {
	      EUDAQ_INFO("Detected 'zero' event");
	    } else {
	      EUDAQ_THROW("Unable to synchronize 'zero' event - please report this run to the EUDAQ developers.");
	    }
	  }
	}
	ev = queue.popevent();
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
