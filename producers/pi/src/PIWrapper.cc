#include "PIWrapper.h"
#include <iostream>
#include <string>


	//*** constructor and destructor

	/*	PIWrapper::PIWrapper()
	 *		Default constructor with empty argument list.
	 */
	PIWrapper::PIWrapper() {}

	/*	PIWrapper::PIWrapper(int port, int baud, char* axis)
	 *		Constructor heading for serial connection with arguments 'port' as port number, 'baud' as baud rate and 'axis' as the chosen axis.
	 */
	PIWrapper::PIWrapper(int port, int baud, char* axis) {
		m_portnumber = port;
		m_baudrate = baud;
		m_axis = axis;
	}

	/*	PIWrapper::PIWrapper(int port, char* host, char* axis)
	 *		Constructor heading for TCP/IP connection with arguments 'port' as port number, 'host' as host name and 'axis' as the chosen axis.
	 */
	PIWrapper::PIWrapper(int port, char* host) {
		m_portnumber = port;
		m_hostname = host;
	}

	/*	PIWrapper::~PIWrapper()
	 *		Default destructor with empty argument list.
	 */
	PIWrapper::~PIWrapper() {}


	//*** functions to establish and close connection

	/*  bool PIWrapper::connectRS232()
	 *		Function for establishing a serial connection to the wrapper with given port number and baud rate.
	 *		Sets the ID and returns true for a sucessful connection.
	 */
	bool PIWrapper::connectRS232() {
		std::clog << "Try to connect via RS232 on port " << getPortNumber() << " and with a baud rate of " << getBaudRate() << "." << std::endl;
		int id = PI_ConnectRS232(getPortNumber(), getBaudRate());
		if (id != -1){
			std::clog << "Connection successful with ID " << id << "." << std::endl;
			setID(id);
			return true;
		}
		else {
			std::clog << "Connection failed." << std::endl;
			return false;
		}
	}

	/*  bool PIWrapper::connectTCPIP()
	 *		Function for establishing a TCP/IP connection to the wrapper with given hostname and port number.
	 *		Sets the ID and returns true for a sucessful connection.
	 */
	bool PIWrapper::connectTCPIP() {
		std::clog << "Try to connect via TCP/IP with hostname " << getHostName() << " and on port " << getPortNumber() << "." << std::endl;
		int id = PI_ConnectTCPIP(getHostName(), getPortNumber());
		if (id != -1) {
			std::clog << "Connection successful with ID " << id << "." << std::endl;
			setID(id);
			return true;
		}
		else {
			std::clog << "Connection failed." << std::endl;
			return false;
		}
	}

	/*  bool PIWrapper::closeConnection()
	 *		Function for closing the connection to the wrapper if a valid ID exists.
	 *		Returns true for a sucessful disconnection.
	 */
	bool PIWrapper::closeConnection() {
		if (getID() != -1) {
			std::clog << "Close connection with ID " << getID() << "." << std::endl;
			PI_CloseConnection(getID());
			return true;
		}
		else {
			std::clog << "No existing connection." << std::endl;
			return false;
		}
	}

	//*** get and set functions

	/*	int PIWrapper::getID()
	 *		Returns the ID (member variable m_ID).
	 */
	int PIWrapper::getID() {
		return m_ID;
	}

	/*	void PIWrapper::setID(const int id)
	 *		Sets the ID (member variable m_ID) to value 'id'.
	 */
	void PIWrapper::setID(const int id) {
		m_ID = id;
	}

	/*	int PIWrapper::getID()
	 *		Returns the ID value (member variable m_ID).
	 */
	int PIWrapper::getBaudRate() {
		return m_baudrate;
	}

	/*	void PIWrapper::setBaudRate(const int baud)
	 *		Sets the baud rate (member variable m_baudrate) to value 'baud'.
	 */
	void PIWrapper::setBaudRate(const int baud){
		m_baudrate = baud;
	}

	/*	int PIWrapper::getPortNumber()
	 *		Returns the port number (member variable m_portnumber).
	 */
	int PIWrapper::getPortNumber() {
		return m_portnumber;
	}

	/*	void PIWrapper::setPortNumber(const int port)
	 *		Sets the port number (member variable m_portnumber) to value 'port'.
	 */
	void PIWrapper::setPortNumber(const int port){
		m_portnumber = port;
	}

	/*	char* PIWrapper::getHostName()
	 *		Returns the host name (member variable m_hostname).
	 */
	char* PIWrapper::getHostName() {
		return m_hostname;
	}

	/*	void PIWrapper::setHostName(char* host)
	 *		Sets the host name (member variable m_hostname) to value 'host'.
	 */
	void PIWrapper::setHostName(char* host) {
		m_hostname = host;
	}

	/*	char* PIWrapper::getAxis()
	 *		Returns the axis (member variable m_axis).
	 */
	char* PIWrapper::getAxis() {
		return m_axis;
	}

	/*	void PIWrapper::setAxis(char* axis)
	 *		Sets the axis (member variable m_axis) to value 'axis'.
	 */
	void PIWrapper::setAxis(char* axis){
		m_axis = axis;
	}

	long PIWrapper::ConnectFirstFoundC884ViaTCPIP()
	{
		char szFoundDevices[1000];
		printf("searching TCPIP devices...\n");
		if (PI_EnumerateTCPIPDevices(szFoundDevices, 999, "") == 0)
			return -1;
		std::clog << szFoundDevices << std::endl;
		char* szAddressToConnect = NULL;
		int port = 0;
		char * pch = strtok(szFoundDevices, "\n");
		while (pch != NULL)
		{
			_strupr(pch);
			if ((strstr(pch, "C-884") != NULL) && (strstr(pch, "LISTENING") != NULL))
			{
				char* colon = strstr(pch, ":");
				*colon = '\0';
				szAddressToConnect = new char[strlen(strstr(pch, "(") + 1) + 1];
				strcpy(szAddressToConnect, strstr(pch, "(") + 1);

				*strstr(colon + 1, ")") = '\0';
				port = atoi(colon + 1);
				break;
			}
			pch = strtok(NULL, "\n");
		}

		if (szAddressToConnect != NULL)
		{
			printf("trying to connect with %s, port %d\n", szAddressToConnect, port);
			int iD = PI_ConnectTCPIP(szAddressToConnect, port);
			printf("connected with iD %d\n", iD);
			delete[]szAddressToConnect;
			setID(iD);
			return iD;
		}
		return -1;
	}

	bool PIWrapper::checkIDN()
	{
		char szIDN[200];
		if (PI_qIDN(getID(), szIDN, 199) == FALSE)
		{
			printf("qIDN failed. Exiting.\n");
			return false;
		}
		printf("qIDN returned: %s\n", szIDN);
		return true;
	}

	bool PIWrapper::moveTo(char* axis, double position) {
		printf("Moving axis %s to %f...\n", axis, position);
		if (!PI_MOV(getID(), axis, &position))
			return FALSE;
		// Wait until the closed loop move is done.
		BOOL bIsMoving = TRUE;
		double dPos;
		while (bIsMoving == TRUE)
		{
			if (!PI_qPOS(getID(), axis, &dPos))
				return FALSE;
			if (!PI_IsMoving(getID(), axis, &bIsMoving))
				return FALSE;
		}
		return true;
	}

	bool PIWrapper::setVelocityStage(char* axis, double velo) {
		bool status = PI_VEL(getID(), axis, &velo);
		if (status == false)
			std::cout << "Could not assign velocity." << std::endl;
		else
			std::cout << "Velocity of axis " << axis << " set to " << velo << std::endl;
		return status;
	}

	// can't get velo value 
	bool PIWrapper::getVelocityStage(char* axis, double velo) {
		return PI_qVEL(getID(), axis, &velo);
	}

	// modified to get velo value 
	bool PIWrapper::getVelocityStage2(char* axis, double* velo) {
		return PI_qVEL(getID(), axis, velo);
	}

	bool PIWrapper::printVelocityStage(char* axis) {
		double velo;
		bool status = PI_qVEL(getID(), axis, &velo);
		std::cout << velo << std::endl;
		return status;
	}

	bool PIWrapper::moveToWithOutput(char* axis, double position) {
		printf("Moving axis %s to %f...\n", axis, position);
		if (!PI_MOV(getID(), axis, &position))
			return FALSE;
		// Wait until the closed loop move is done.
		BOOL bIsMoving = TRUE;
		double dPos;
		while (bIsMoving == TRUE)
		{
			if (!PI_qPOS(getID(), axis, &dPos))
				return FALSE;
			if (!PI_IsMoving(getID(), axis, &bIsMoving))
				return FALSE;
			printf("qPOS(%s): %g\n", axis, dPos);
		}
		return true;
	}

	bool PIWrapper::getPosition(char* axis, double position) {
		return PI_qPOS(getID(), axis, &position);
	}

	// modified to get position value 
	bool PIWrapper::getPosition2(char* axis, double* position) {
		return PI_qPOS(getID(), axis, position);
	}

	bool PIWrapper::printPosition(char* axis) {
		double position;
		bool status = PI_qPOS(getID(), axis, &position);
		std::cout << position << std::endl;
		return status;
	}

	bool PIWrapper::axisIsReferenced(char* axis){
		BOOL isReferenced;
		bool flagCommand = PI_qFRF(getID(), axis, &isReferenced);
		return (isReferenced && flagCommand);
	}

	bool PIWrapper::isConnected() {
		return PI_IsConnected(getID());
	}

	bool PIWrapper::printStringID() {
		const int buffer = 100;
		char name[buffer];
		bool status = PI_qIDN(getID(), name, buffer);
		std::cout << name << std::endl;
		return status;
	}

	bool PIWrapper::getStringID(char* name, int buffer) {
		return PI_qIDN(getID(), name, buffer);
	}

	bool PIWrapper::printStageTypeOfAxis(char* axis) {
		const int buffer = 100;
		char name[buffer];
		bool status = PI_qCST(getID(), axis, name, buffer);
		std::cout << "Type of connected stages on axis " << axis << " :" << std::endl;
		std::cout << name << std::endl;
		return status;
	}

	bool PIWrapper::getStageTypeOfAxis(char* axis, char* name, int buffer) {
		return PI_qCST(getID(), axis, name, buffer);
	}

	bool PIWrapper::printStageTypeAll() {
		const int buffer = 100;
		char name[buffer];
		bool status = PI_qCST(getID(), NULL, name, buffer);
		std::cout << "Type of connected stages:" << std::endl;
		std::cout << name << std::endl;
		return status;
	}

	bool PIWrapper::getStageTypeAll(char* name, int buffer) {
		return PI_qCST(getID(), NULL, name, buffer);
	}

	bool PIWrapper::setStageToAxis(char* axis, char* name) {
		return PI_CST(getID(), axis, name);
	}

	bool PIWrapper::referenceIfNeeded(char* axis)
	{
		BOOL bReferenced;
		BOOL bFlag;
		if (!PI_qFRF(getID(), axis, &bReferenced))
			return false;
		if (!bReferenced)
		{// if needed,
			// reference the axis using the refence switch
			printf("Referencing axis %s...\n", axis);
			if (!PI_FRF(getID(), axis))
			{
				printf("hi");
				return false;
			}

			// Wait until the reference move is done.
			bFlag = false;
			while (bFlag != TRUE)
			{
				if (!PI_IsControllerReady(getID(), &bFlag))
					return false;
			}
		}

		printf("Axis %s is referenced.\n", axis);
		return true;
	}

	bool PIWrapper::printAvailableStageTypes() {
		const int buffer = 100;
		char name[buffer];
		bool status = PI_qVST(getID(), name, buffer);
		std::cout << "Type of available stages:" << std::endl;
		std::cout << name << std::endl;
		return status;
	}

	bool PIWrapper::setServoState(char* axis, BOOL state) {
		return PI_SVO(getID(), axis, &state);
	}

	bool PIWrapper::setServoStateAll(BOOL state) {
		return PI_SVO(getID(), "", &state);
	}

	bool PIWrapper::getServoState(char* axis, BOOL state) {
		bool status = PI_qSVO(getID(), axis, &state);
		return status;
	}

	bool PIWrapper::printServoState(char* axis) {
		BOOL servoState;
		bool status = PI_qSVO(getID(), axis, &servoState);
		std::cout << "Servo of axis " << axis << " is ";
		std::cout << (servoState) ? "ON" : "OFF";
		std::cout << std::endl;
		return status;
	}

	bool PIWrapper::printServoStateAll() {
		BOOL servoState;
		return PI_qSVO(getID(), "", &servoState);
	}

	bool PIWrapper::maxRangeOfStage(char* axis, double* maxValue) {
		return PI_qTMX(getID(), axis, maxValue);
	}




	//////work to do here!!!

	//*** functions for configuration

	/*	PIWrapper::ReferenceIfNeeded()
	 *		...
	 */
	/*
	bool PIWrapper::ReferenceIfNeeded(){
	BOOL flag = true;
	if (PI_SVO(getID(), getSzAxis(), &flag) == FALSE)
	{
	return false;
	}
	BOOL bReferenced;
	BOOL bFlag;

	//PI_qFRF:  Gets whether the given axis is referenced or not.

	if (!PI_qFRF(getID(), getSzAxis(), &bReferenced)){ return false; }
	if (!bReferenced){
	// if needed,
	// reference the axis using the refence switch
	if (!PI_FNL(getID(), getSzAxis())){ return FALSE; }
	// Wait until the reference move is done.
	bFlag = FALSE;
	while (bFlag != TRUE){
	if (!PI_IsWrapperReady(getID(), &bFlag))
	return false;
	}
	}
	return true;
	}



	void PIWrapper::reportError()
	{
	int err = PI_GetError(getID());
	char szErrMsg[300];
	if (PI_TranslateError(err, szErrMsg, 299))
	{
	printf("Error %d occured: %s\n", err, szErrMsg);
	}
	}

	//not that the values this function returns so far don't realy make any sense...
	double PIWrapper::getPosition(){
	double position[1];
	bool returnValue = PI_qPOS(getID(), getSzAxis(), position);
	if (!returnValue){
	std::cout << "could not find the wrappers position." << endl;
	return -1;
	}
	else{ return position[0]; }
	}



	int PIWrapper::MoveServeralSteps(const std::vector<double> positions, int pauseTime){
	for (unsigned int i = 0; i < positions.size(); i++){
	if (positions[i] < 0 ){
	return i;
	}
	else{
	Move(positions[i]);
	while (isMoving()){}
	pause(pauseTime);
	}
	}
	return -1;

	}

	bool PIWrapper::isMoving(){
	BOOL pbValue = TRUE;
	if (PI_IsMoving(getID(), getSzAxis(), &pbValue)){
	return pbValue;
	}
	else{
	std::cout << "Error in isMoving function in PIWrapper class! function PI_IsMoving didn't return expected value(true)" << std::endl;
	return false;
	}
	}

	void PIWrapper::pause(int miliseconds){
	Sleep(miliseconds);
	}
	*/


