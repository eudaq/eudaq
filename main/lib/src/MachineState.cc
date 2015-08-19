#include "eudaq/Utils.hh"
#include "eudaq/MachineState.hh"



namespace eudaq {

	MachineState::MachineState()
	{

	}

	int MachineState::GetState()
	{
		bool isUnconf = false, isRunning = false; 
		
		int state = -1;
		for(std::pair <const eudaq::ConnectionInfo, eudaq::Status> s: connection_status_info)
		{
			state = s.second.GetState();

			if(state == eudaq::Status::ST_ERROR)
				return (eudaq::Status::ST_ERROR);

			isUnconf = (state == eudaq::Status::ST_UNCONF) || isUnconf;
			isRunning = (state == eudaq::Status::ST_RUNNING) || isRunning;

		}

		if (isRunning)
			return eudaq::Status::ST_RUNNING;
		else if(isUnconf)
			return eudaq::Status::ST_UNCONF;
		
		return eudaq::Status::ST_CONF; 

	}

	int MachineState::GetState(ConnectionInfo id)
	{
		//return (0);
		
		try
		{
			Status s = connection_status_info.at(id);
			return s.GetState();
		}
		catch(const std::out_of_range& err)
		{
			std::cout<<"Out of Range error: "<< err.what()<<" No such ConnectionInfo exists. \n";
			return -1;
		}

	}


	void MachineState::SetState(ConnectionInfo id, Status* state)
	{
		//std::cout<<" From GetRemoteInfo: "<< id.GetRemoteInfo()<< "\n";
		connection_status_info[id] = *state;
		return;
	}

	void MachineState::RemoveState(ConnectionInfo id)
	{
		connection_status_info.erase(id);
	}

	void MachineState::Print()
	{
		std::cout<<"----------------Current State---------------- \n";

		//std::cout<<"Size of array: "<< connection_status_info.size()<<"\n";
		for(std::pair <const eudaq::ConnectionInfo, eudaq::Status> s: connection_status_info)
		{
				std::cout<<" Connection From: "<< to_string(s.first)<< "   In State: "<< s.second.GetState()<< "\n";
		}

		std::cout<<"------------------------\n";
		std::cout<<" The State of the FSM: "<< GetState()<< "\n ";
		std::cout<<"------------------------------------------- \n \n";

	}

}