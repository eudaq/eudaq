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
    // std::cout << "ReceivePacket()" << std::flush;
    while (!m_events.empty() &&
           m_events.front().etype != TransportEvent::RECEIVE) {
      m_events.pop();
    }
    // std::cout << " current level=" << m_events.size() << std::endl;
    if (m_events.empty()) {
      // std::cout << "Getting more" << std::endl;
      ProcessEvents(timeout);
      // std::cout << "Temp level=" << m_events.size() << std::endl;
      while (!m_events.empty() &&
             m_events.front().etype != TransportEvent::RECEIVE) {
        m_events.pop();
      }
      // std::cout << "New level=" << m_events.size() << std::endl;
    }
    bool ret = false;
    if (!m_events.empty() && conn.Matches(m_events.front().id)) {
      ret = true;
      *packet = m_events.front().packet;
      m_events.pop();
    }
    // std::cout << "ReceivePacket() return " << (ret ? "true" : "false") <<
    // std::endl;
    return ret;
  }

  bool TransportBase::SendReceivePacket(const std::string &sendpacket,
                                        std::string *recpacket,
                                        const ConnectionInfo &connection,
                                        int timeout) {
    std::lock_guard<std::recursive_mutex> lk(m_mutex);
    SendPacket(sendpacket, connection);
    if (timeout == -1)
      timeout = DEFAULT_TIMEOUT;
    bool ret = ReceivePacket(recpacket, timeout, connection);
    return ret;
  }

  TransportBase::~TransportBase() {}
}
