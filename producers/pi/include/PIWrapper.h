#ifndef PIWRAPPER_H
#define PIWRAPPER_H

//PI includes
#include "PI_GCS2_DLL.h"

//system includes
#include <windows.h>
#include <vector>


	//PI controller class
	class PIWrapper {

	protected:
		int m_ID;
		int m_baudrate;
		int m_portnumber;
		char* m_hostname;
		char* m_axis;


	public:
		// constructor and destructor
		PIWrapper();
		PIWrapper(int port, int baud, char* axis);														//constructor for serial connection
		PIWrapper(int port, char* host);						 					   	//constructor for TCPIP connection
		~PIWrapper();

		//functions to establish and close connection
		bool connectRS232();
		bool connectTCPIP();
		bool closeConnection();
		bool isConnected();

		long ConnectFirstFoundC884ViaTCPIP();

		//get and set functions
		int getID();																															//return member variable m_ID
		void setID(const int id);																									//write id as member variable m_ID
		int getBaudRate();																												//return member variable m_baudrate
		void setBaudRate(const int baud);																					//write baud as member variable m_baudrate
		int getPortNumber();																											//return member variable m_portnumber
		void setPortNumber(const int port);																				//write port as member variable m_portnumber
		char* getHostName();																									    //return member variable m_hostname
		void setHostName(char* host);																						  //write host as member variable m_hostname
		char* getAxis();																											  	//return member variable m_szAxis
		void setAxis(char* axis);																								  //write axis as member variable m_szAxis

		//functions for interaction
		bool moveTo(char* axis, double position);
		bool moveToWithOutput(char* axis, double position);																			//move of stage to position
		bool axisIsReferenced(char* axis);

		bool printStageTypeOfAxis(char* axis);
		bool getStageTypeOfAxis(char* axis, char* name, int buffer);
		bool printStageTypeAll();
		bool getStageTypeAll(char* name, int buffer);
		bool setStageToAxis(char* axis, char* name);

		bool printStringID();
		bool getStringID(char* name, int buffer);

		bool referenceIfNeeded(char* axis);

		bool maxRangeOfStage(char* axis, double* maxValue);

		bool getPosition(char* axis, double position);
		bool getPosition2(char* axis, double* position);
		bool printPosition(char* axis);

		bool printAvailableStageTypes();

		bool setServoState(char* axis, BOOL state);
		bool setServoStateAll(BOOL state);
		bool getServoState(char* axis, BOOL state);
		bool printServoState(char* axis);
		bool printServoStateAll();

		// to be followed
		/*
		//functions for configuration
		bool referenceIfNeeded();																									//start referencing of PI stage

		int moveServeralSteps(const std::vector<double> positions, int pauseTime);
		bool isMoving();
		double getPosition();

		//general functions
		void pause(int miliseconds);
		void reportError();
		*/

		bool checkIDN();


		bool setVelocityStage(char* axis, double velo);
		bool getVelocityStage(char* axis, double velo);
		bool getVelocityStage2(char* axis, double* velo);
		bool printVelocityStage(char* axis);

	};


#endif
