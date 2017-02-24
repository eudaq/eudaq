
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <conio.h>
#include <time.h>
#include <string>
#include "eudaq/OptionParser.hh"
#include "PIWrapper.h"
#include "eudaq/Utils.hh"




int main(int argc, char* argv[])
{
	char *m_hostname;
	int m_portnumber;

	char *m_stagename1;
	char *m_stagename2;
	char *m_stagename3;
	char *m_stagename4;

	char *m_axis1 = "1";
	char *m_axis2 = "2";
	char *m_axis3 = "3";
	char *m_axis4 = "4";
	double m_axis1max = 0;
	double m_axis2max = 0;
	double m_axis3max = 0;
	double m_axis4max = 0;
	double m_position1 = 0.0;
	double m_position2 = 0.0;
	double m_position3 = 0.0;
	double m_position4 = 0.0;

	eudaq::OptionParser op("PI stage Control Utility", "1.1", "A comand-line tool for controlling the PI stage");
	eudaq::Option<std::string> hostname(op, "ho", "HostName", "192.168.22.5", "string", "set hostname for TCP/IP connection");
	eudaq::Option<int> portnumber(op, "p", "PortNumber", 50000, "number", "set portnumber for TCP/IP connection");
	eudaq::Option<std::string> stagename1(op, "s1", "StageName", "NOSTAGE", "string", "set stage for axis 1");
	eudaq::Option<std::string> stagename2(op, "s2", "StageName", "NOSTAGE", "string", "set stage for axis 2");
	eudaq::Option<std::string> stagename3(op, "s3", "StageName", "NOSTAGE", "string", "set stage for axis 3");
	eudaq::Option<std::string> stagename4(op, "s4", "StageName", "NOSTAGE", "string", "set stage for axis 4");
	eudaq::Option<double> position1(op, "p1", "Position1", -1.0, "number", "move stage on axis1 to position, -1 no movement");
	eudaq::Option<double> position2(op, "p2", "Position2", -1.0, "number", "move stage on axis2 to position, -1 no movement");
	eudaq::Option<double> position3(op, "p3", "Position3", -1.0, "number", "move stage on axis3 to position, -1 no movement");
	eudaq::Option<double> position4(op, "p4", "Position4", -1.0, "number", "move stage on axis4 to position, -1 no movement");

	try {
		op.Parse(argv);}
	catch (...) {
		return op.HandleMainException();}

	m_hostname = const_cast<char*>(hostname.Value().c_str());
	m_portnumber = portnumber.Value();
	m_stagename1 = const_cast<char*>(stagename1.Value().c_str());
	m_stagename2 = const_cast<char*>(stagename2.Value().c_str());
	m_stagename3 = const_cast<char*>(stagename3.Value().c_str());
	m_stagename4 = const_cast<char*>(stagename4.Value().c_str());
	m_position1 = position1.Value();
	m_position2 = position2.Value();
	m_position3 = position3.Value();
	m_position4 = position4.Value();

	//////////////////////////////////
	std::cout << "Start PIControl" << std::endl;
	// create
	PIWrapper controller(m_portnumber, m_hostname);
	
	// connect
	controller.connectTCPIP();
	
	// check
	if (!controller.isConnected()) {
		return false;
	}
	else {
		controller.printStringID();
		controller.printStageTypeAll();
	}

	// get velocity
	std::cout << "Velocity" << std::endl;
	controller.printVelocityStage(m_axis1);
	controller.printVelocityStage(m_axis2);
	controller.printVelocityStage(m_axis3);
	controller.printVelocityStage(m_axis4);
	// get maximum travel range
	controller.maxRangeOfStage(m_axis1, &m_axis1max);
	controller.maxRangeOfStage(m_axis2, &m_axis2max);
	controller.maxRangeOfStage(m_axis3, &m_axis3max);
	controller.maxRangeOfStage(m_axis4, &m_axis4max);
	std::cout << "\nMaximum range\n" 
		<< m_axis1max << "\n" 
		<< m_axis2max << "\n" 
		<< m_axis3max << "\n"
		<< m_axis4max << std::endl;
	// get position
	printf("\nPosition...\n");
	controller.printPosition(m_axis1);
	controller.printPosition(m_axis2);
	controller.printPosition(m_axis3);
	controller.printPosition(m_axis4);

	//check if set, if not, assign, servo, reference
	if (!controller.axisIsReferenced(m_axis1)) {
		controller.setStageToAxis(m_axis1, m_stagename1);
		if (controller.getServoState(m_axis1, false)) {
			controller.setServoState(m_axis1, true);
		}
		controller.referenceIfNeeded(m_axis1);
	}
	if (!controller.axisIsReferenced(m_axis2)) {
		controller.setStageToAxis(m_axis2, m_stagename2);
		if (controller.getServoState(m_axis2, false)) {
			controller.setServoState(m_axis2, true);
		}
		controller.referenceIfNeeded(m_axis2);
	}
	if (!controller.axisIsReferenced(m_axis3)) {
		controller.setStageToAxis(m_axis3, m_stagename3);
		if (controller.getServoState(m_axis3, false)) {
			controller.setServoState(m_axis3, true);
		}
		controller.referenceIfNeeded(m_axis3);
	}
	if (!controller.axisIsReferenced(m_axis4)) {
		controller.setStageToAxis(m_axis4, m_stagename4);
		if (controller.getServoState(m_axis4, false)) {
			controller.setServoState(m_axis4, true);
		}
		controller.referenceIfNeeded(m_axis4);
	}

	// Move
	printf("\nMove...\n");
	if (m_position1 >= m_axis1max) { m_position1 = m_axis1max; }
	if (m_position1 <= 0) { printf("No movement of axis1!\n"); }
	else { controller.moveTo(m_axis1, m_position1); }
	if (m_position2 >= m_axis2max) { m_position2 = m_axis2max; }
	if (m_position2 <= 0) { printf("No movement of axis2!\n"); }
	else { controller.moveTo(m_axis2, m_position2); }
	if (m_position3 >= m_axis3max) { m_position3 = m_axis3max; }
	if (m_position3 <= 0) { printf("No movement of axis3!\n"); }
	else { controller.moveTo(m_axis3, m_position3); }
	if (m_position4 >= m_axis4max) { m_position4 = m_axis4max; }
	if (m_position4 <= 0) { printf("No movement of axis4!\n"); }
	else { controller.moveTo(m_axis4, m_position4); }
	
	printf("\nPosition...\n");
	controller.printPosition(m_axis1);
	controller.printPosition(m_axis2);
	controller.printPosition(m_axis3);
	controller.printPosition(m_axis4);

	return 0;
}

