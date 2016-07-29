#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>

class RCConnections {
public:
  RCConnections(const eudaq::ConnectionInfo & id): m_id(id.Clone()){};
  eudaq::ConnectionState GetConnectionState() const { return m_connectionstate; }
  int GetState() const { return m_connectionstate.GetState(); }
  bool IsConnected() const { return m_id->IsEnabled(); }
  void SetConnected(bool con) { m_id->SetState(2*con - 1); }
  const eudaq::ConnectionInfo & GetId() const { return *m_id; }
  void SetConnectionState(const eudaq::ConnectionState & connectionstate) { m_connectionstate = connectionstate; }
private:
  std::shared_ptr<eudaq::ConnectionInfo> m_id;
  eudaq::ConnectionState m_connectionstate;
};

class PyRunControl : public eudaq::RunControl {
public:
  PyRunControl(const std::string & listenaddress)
    : eudaq::RunControl(listenaddress)
  {}
  void OnReceive(const eudaq::ConnectionInfo & id, std::shared_ptr<eudaq::ConnectionState> connectionstate) {
    std::cout << "[Run Control] Received:    " << id << std::endl;
    for (size_t i = 0; i < m_connections.size(); ++i) {
      if (id.Matches(m_connections[i].GetId())) {
	m_connections[i].SetConnectionState(*connectionstate);
	break;
      }
    }
  }
  void OnConnect(const eudaq::ConnectionInfo & id) {
    std::cout << "[Run Control] Connection established:    " << id << std::endl;
    for (size_t i = 0; i < m_connections.size(); ++i) {
      if (!m_connections.at(i).IsConnected() && id.Matches(m_connections.at(i).GetId())) {
	m_connections[i] = RCConnections(id);
	return;
      }
    }
    m_connections.push_back(RCConnections(id));
  }
  void OnDisconnect(const eudaq::ConnectionInfo & id) {
    std::cout << "[Run Control] Disconnected:    " << id << std::endl;
    for (size_t i = 0; i < m_connections.size(); ++i) {
      if (id.Matches(m_connections[i].GetId())) {
	m_connections[i].SetConnected(false);
	break;
      }
    }
  }

  void PrintConnections(){
    std::cout << "Connections (" << m_connections.size() << ")" << std::endl;
    for (uint16_t i = 0; i < m_connections.size(); ++i) {
      std::cout << "  " << m_connections.at(i).GetId()<< ": " << m_connections.at(i).GetConnectionState() << std::endl;
    }
  }

  uint16_t GetRunNumber(){return RunControl::m_runnumber;}

  bool AllOk(){
    bool ok = true;
    for (size_t i = 0; i < m_connections.size(); ++i) {
      if (m_connections.at(i).GetState() != eudaq::ConnectionState::LVL_OK) {
	ok = false;
	break;
      }
    }
    return ok;
  }

  size_t NumConnections(){return m_connections.size();}

private:
  std::vector<RCConnections> m_connections;
};

// ctypes can only talk to C functions -- need to provide them through 'extern "C"'
extern "C" {
  DLLEXPORT PyRunControl* PyRunControl_new(char *listenaddress){return new PyRunControl(std::string(listenaddress));}
  DLLEXPORT void PyRunControl_GetConnectionState(PyRunControl *prc){prc->GetConnectionState();}
  DLLEXPORT void PyRunControl_StartRun(PyRunControl *prc){prc->StartRun();}
  DLLEXPORT void PyRunControl_StopRun(PyRunControl *prc){prc->StopRun();}
  DLLEXPORT void PyRunControl_Configure(PyRunControl *prc, char *cfg){prc->Configure(std::string(cfg));}
  DLLEXPORT void PyRunControl_PrintConnections(PyRunControl *prc){prc->PrintConnections();}
  DLLEXPORT size_t PyRunControl_NumConnections(PyRunControl *prc){return prc->NumConnections();}
  DLLEXPORT bool PyRunControl_AllOk(PyRunControl *prc){return prc->AllOk();}
  DLLEXPORT uint16_t PyRunControl_GetRunNumber(PyRunControl *prc){return prc->GetRunNumber();}

}
