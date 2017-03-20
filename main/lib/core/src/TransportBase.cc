#include "eudaq/TransportBase.hh"
#include <ostream>
#include <iostream>

namespace eudaq {

  const ConnectionInfo ConnectionInfo::ALL("ALL");
  static const int DEFAULT_TIMEOUT = 1000;

  void ConnectionInfo::Print(std::ostream &os) const {
    if (m_state == -1) {
      os << "(disconnected)";
    } else if (m_state == 0) {
      os << "(waiting)";
    } else {
      os << m_type;
      if (m_name != "")
        os << "." << m_name;
    }
  }

  bool ConnectionInfo::Matches(const ConnectionInfo & /*other*/) const {
    // std::cout << " [Match: all] " << std::flush;
    return true;
  }

  TransportBase::TransportBase() : m_callback(0) {}

  void TransportBase::SetCallback(const TransportCallback &callback) {
    m_callback = callback;
  }

  void TransportBase::Process(int timeout) {
    if (timeout == -1)
      timeout = DEFAULT_TIMEOUT;
    std::unique_lock<std::recursive_mutex> lk(m_mutex);
    ProcessEvents(timeout);
    lk.unlock();
    for (;;) {
      std::unique_lock<std::recursive_mutex> lk(m_mutex);
      if (m_events.empty())
        break;
      TransportEvent evt(m_events.front());
      m_events.pop();
      lk.unlock();
      m_callback(evt);
    }
  }

  bool TransportBase::ReceivePacket(std::string *packet, int timeout,
                                    const ConnectionInfo &conn) {
    if (timeout == -1)
      timeout = DEFAULT_TIMEOUT;
    while (!m_events.empty() &&
           m_events.front().etype != TransportEvent::RECEIVE) {
      m_events.pop();
    }
    if (m_events.empty()) {
      ProcessEvents(timeout);
      while (!m_events.empty() &&
             m_events.front().etype != TransportEvent::RECEIVE) {
        m_events.pop();
      }
    }
    bool ret = false;
    if (!m_events.empty() && conn.Matches(*(m_events.front().id))) {
      ret = true;
      *packet = m_events.front().packet;
      m_events.pop();
    }
    return ret;
  }

  TransportBase::~TransportBase() {}
}
