#include "eudaq/EventSynchronisationBase.hh"

#include "eudaq/Event.hh"

#include <iostream>

#include <memory>
#include "eudaq/PluginManager.hh"
#include "eudaq/Configuration.hh"

using std::cout;
using std::endl;
using std::shared_ptr;
using namespace std;
namespace eudaq {
  SyncBase::SyncBase(bool sync)
      : m_registertProducer(0), m_ProducerEventQueue(0),
        NumberOfEventsToSync_(1), longTimeDiff_(0), isAsync_(false),
        m_TLUs_found(0), m_sync(sync) {}

  void SyncBase::addBOREEvent(int fileIndex,
                              const eudaq::DetectorEvent &BOREvent) {
    m_registertProducer += BOREvent.NumEvents();

    m_EventsProFileReader.push_back(BOREvent.NumEvents());

    eudaq::Configuration conf(BOREvent.GetTag("CONFIG"));
    conf.SetSection("EventStruct");

    longTimeDiff_ = conf.Get("LongBusyTime", longTimeDiff_); // from config file
    longTimeDiff_ =
        BOREvent.GetTag("longTimeDelay", longTimeDiff_); // from command line

    NumberOfEventsToSync_ =
        conf.Get("NumberOfEvents", NumberOfEventsToSync_); // from config file
    NumberOfEventsToSync_ = BOREvent.GetTag(
        "NumberOfEvents", NumberOfEventsToSync_); // from command line

    const unsigned int TLU_ID = Event::str2id("_TLU");
    static size_t id = 1;

    for (unsigned i = 0; i < BOREvent.NumEvents(); ++i) {
      if (TLU_ID == BOREvent.GetEvent(i)->get_id()) {
        if (m_TLUs_found == 0) {
          m_ProducerId2Eventqueue[getUniqueID(fileIndex, i)] = 0;
        } else {
          m_ProducerId2Eventqueue[getUniqueID(fileIndex, i)] =
              id++; // only the first TLU gets threated differently all others
                    // are just producers
        }
        ++m_TLUs_found;
      } else {

        m_ProducerId2Eventqueue[getUniqueID(fileIndex, i)] = id++;
      }
    }
  }

  bool SyncBase::Event_Queue_Is_Empty() {

    for (auto q = m_ProducerEventQueue.begin(); q != m_ProducerEventQueue.end();
         ++q) {
      if (q->empty()) {
        return true;
      }
    }
    return false;
  }

  bool SyncBase::SubEventQueueIsEmpty(int FileID) {
    for (size_t eventNR = 0; eventNR < m_EventsProFileReader[FileID];
         ++eventNR) {
      if (getQueuefromId(FileID, eventNR).empty()) {
        return true;
      }
    }
    return false;
  }

  bool SyncBase::getNextEvent(std::shared_ptr<eudaq::DetectorEvent> &ev) {

    if (!m_DetectorEventQueue.empty()) {

      ev = m_DetectorEventQueue.front();
      m_DetectorEventQueue.pop();
      return true;
    }
    return false;
  }

  SyncBase::eventqueue_t &SyncBase::getQueuefromId(unsigned producerID) {
    if (m_ProducerId2Eventqueue.find(producerID) ==
        m_ProducerId2Eventqueue.end()) {

      EUDAQ_THROW("unknown Producer ID " + std::to_string(producerID));
    }

    return m_ProducerEventQueue[m_ProducerId2Eventqueue[producerID]];
  }

  SyncBase::eventqueue_t &SyncBase::getQueuefromId(unsigned fileIndex,
                                                   unsigned eventIndex) {
    return getQueuefromId(getUniqueID(fileIndex, eventIndex));
  }

  SyncBase::eventqueue_t &SyncBase::getFirstTLUQueue() {
    return m_ProducerEventQueue[0];
  }

  bool SyncBase::SyncFirstEvent() {

    if (m_ProducerEventQueue.size() == 0) {

      EUDAQ_THROW("Producer Event queue is empty");
    }
    auto &TLU_queue = getFirstTLUQueue();

    if (!m_sync) {
      makeDetectorEvent();

      return true;
    }

    while (!Event_Queue_Is_Empty()) {

      if (compareTLUwithEventQueues(TLU_queue.front())) {
        makeDetectorEvent();
        return true;
      } else if (!Event_Queue_Is_Empty()) {
        TLU_queue.pop();
      }
    }

    return false;
  }

  bool SyncBase::SyncNEvents(size_t N) {

    while (m_DetectorEventQueue.size() <= N) {
      if (!SyncFirstEvent()) {
        return false;
      }
    }
    return true;
  }

  void SyncBase::makeDetectorEvent() {
    auto &TLU = m_ProducerEventQueue[0].front();
    shared_ptr<DetectorEvent> det = make_shared<DetectorEvent>(
        TLU->GetRunNumber(), TLU->GetEventNumber(), TLU->GetTimestamp());
    det->AddEvent(TLU);
    for (size_t i = 1; i < m_ProducerEventQueue.size(); ++i) {

      det->AddEvent(m_ProducerEventQueue[i].front());
    }

    m_DetectorEventQueue.push(det);
    // event_queue_pop_TLU_event();
    event_queue_pop();
  }

  void SyncBase::event_queue_pop() {
    for (auto q = m_ProducerEventQueue.begin(); q != m_ProducerEventQueue.end();
         ++q) {

      q->pop();
    }
  }

  void SyncBase::event_queue_pop_TLU_event() {
    m_ProducerEventQueue.begin()->pop();
  }

  bool SyncBase::compareTLUwithEventQueue(
      std::shared_ptr<eudaq::Event> &tlu_event,
      eudaq::SyncBase::eventqueue_t &event_queue) {
    int ReturnValue = Event_IS_Sync;

    std::shared_ptr<TLUEvent> tlu =
        std::dynamic_pointer_cast<TLUEvent>(tlu_event);

    while (!event_queue.empty()) {
      auto &currentEvent = event_queue.front();

      ReturnValue = PluginManager::IsSyncWithTLU(*currentEvent, *tlu);
      if (ReturnValue == Event_IS_Sync) {

        return true;

      } else if (ReturnValue == Event_IS_LATE) {

        isAsync_ = true;
        event_queue.pop();

      } else if (ReturnValue == Event_IS_EARLY) {
        isAsync_ = true;
        return false;
      }
    }
    return false;
  }

  bool SyncBase::compareTLUwithEventQueues(
      std::shared_ptr<eudaq::Event> &tlu_event) {
    for (size_t i = 1; i < m_ProducerEventQueue.size(); ++i) {
      if (!compareTLUwithEventQueue(tlu_event, m_ProducerEventQueue[i])) {
        // could not sync event.
        // TLU event is to early or event queue is empty;
        return false;
      }
    }
    return true;
  }

  void SyncBase::clearDetectorQueue() {

    std::queue<std::shared_ptr<eudaq::DetectorEvent>> empty;
    std::swap(m_DetectorEventQueue, empty);
  }

  void SyncBase::PrepareForEvents() {

    if (!m_sync) {
      std::cout << "events not synchronized" << std::endl;
      if (m_TLUs_found == 0) {
        for (auto &e : m_ProducerId2Eventqueue) {
          e.second--;
        }
      }

    } else {
      if (m_TLUs_found == 0) {
        EUDAQ_THROW("no TLU events found in the data\n for the "
                    "resynchronisation it is nessasary to have a TLU in the "
                    "data stream \n for now the synchrounsation only works "
                    "with the old TLU (date 12.2013)");
      } else if (m_TLUs_found > 1) {
        std::cout << "more than one TLU detected only the first TLU is used "
                     "for synchronisation " << std::endl;
      }
    }
    m_ProducerEventQueue.resize(m_registertProducer);
  }

  unsigned SyncBase::getUniqueID(unsigned fileIndex, unsigned eventIndex) {
    return (fileIndex + 1) * 10000 + eventIndex;
  }

  unsigned SyncBase::getTLU_UniqueID(unsigned fileIndex) {
    return getUniqueID(0, fileIndex);
  }

  void SyncBase::storeCurrentOrder() {
    shared_ptr<TLUEvent> tlu =
        std::dynamic_pointer_cast<TLUEvent>(getFirstTLUQueue().back());
    for (size_t i = 1; i < m_ProducerEventQueue.size(); ++i) {
      shared_ptr<Event> currentEvent = m_ProducerEventQueue[i].back();
      eudaq::PluginManager::setCurrentTLUEvent(*currentEvent, *tlu);
    }
  }

  int SyncBase::AddDetectorElementToProducerQueue(
      int fileIndex, std::shared_ptr<eudaq::DetectorEvent> detEvent) {
    if (detEvent) {

      for (size_t i = 0; i < detEvent->NumEvents(); ++i) {

        auto &q = getQueuefromId(fileIndex, i);
        q.push(detEvent->GetEventPtr(i));
      }
    }
    return true;
  }
}
