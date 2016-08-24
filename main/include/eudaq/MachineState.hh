#ifndef EUDAQ_INCLUDED_MachineState
#define EUDAQ_INCLUDED_MachineState

#include <string>
#include <vector>
#include <map>
#include "eudaq/Platform.hh"
#include "eudaq/ConnectionState.hh"
#include "eudaq/TransportBase.hh"

namespace eudaq
{
  class DLLEXPORT MachineState{
  public:
    MachineState();
    int GetState(); //Returns the state of the whole machine 
    int GetState(ConnectionInfo id); // Returns the state of a single connection
    void SetState(ConnectionInfo* id, ConnectionState* state); // Sets the connection associated with id to state.
    bool HasRunning(); //Returns true if there are running connections
    void RemoveState(ConnectionInfo id); //Removes a connection when it is disconnected....    
    void Print();
  private:
    std::map<ConnectionInfo, ConnectionState> connection_status_info;
  };

}

#endif // EUDAQ_INCLUDED_MachineState
