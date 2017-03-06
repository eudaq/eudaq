#include "eudaq/Controller.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Configuration.hh"

#include "PIWrapper.h"

#include <iostream>
#include <ostream>
#include <vector>
//#include <sched.h>

//PIController::PIController(const std::string &name,
//			     const std::string &runcontrol)
//  : eudaq::Controller(name, runcontrol), m_terminated(false), m_name(name) { }


class PIController : public eudaq::Controller {
public:
	PIController(const std::string &name, const std::string &runcontrol)
		: eudaq::Controller(name, runcontrol), m_terminated(false), m_name(name) {
		std::cout << "PIController was started successfully." << std::endl;
	}

	//~PIController();

	virtual void PIController::OnInitialise(const eudaq::Configuration & init) {
		try {
			std::cout << "Initalising: " << init.Name() << std::endl;

			std::string m_hostname2 = init.Get("HostName", "192.168.21.5");
			m_hostname = &m_hostname2[0];
			m_portnumber = init.Get("PortNumber", 50001);
			std::cout << m_hostname2 << m_portnumber << m_hostname << std::endl;
			if (!wrapper) {
				wrapper = std::make_shared<PIWrapper>(m_portnumber, m_hostname);
				// connect
				if (!wrapper->connectTCPIP()) {
					EUDAQ_ERROR("No TCP/IP connection to PI Controller!");
				}
			}
			// check
			if (!wrapper->isConnected()) {
				EUDAQ_ERROR("No TCP/IP connection to PI Controller!");
			}
			else {
				wrapper->printStringID();
				wrapper->printStageTypeAll();
			}

			if (!wrapper->axisIsReferenced(m_axis1)) {
				std::cout << "Axis1 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis2)) {
				std::cout << "Axis2 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis3)) {
				std::cout << "Axis3 not referenced! Please use PIControl!" << std::endl;
			}
			if (!wrapper->axisIsReferenced(m_axis4)) {
				std::cout << "Axis4 not referenced! Please use PIControl!" << std::endl;
			}

			std::cout << "Set to max. velocity:" << std::endl;
			wrapper->setVelocityStage(m_axis1, m_velocitymax);
			wrapper->setVelocityStage(m_axis2, m_velocitymax);
			wrapper->setVelocityStage(m_axis3, m_velocitymax);
			wrapper->setVelocityStage(m_axis4, m_velocitymax);
			
			std::cout << "\nGet velocity:" << std::endl;
			wrapper->printVelocityStage(m_axis1);
			wrapper->printVelocityStage(m_axis2);
			wrapper->printVelocityStage(m_axis3);
			wrapper->printVelocityStage(m_axis4);

			// get maximum travel range
			wrapper->maxRangeOfStage(m_axis1, &m_axis1max);
			wrapper->maxRangeOfStage(m_axis2, &m_axis2max);
			wrapper->maxRangeOfStage(m_axis3, &m_axis3max);
			wrapper->maxRangeOfStage(m_axis4, &m_axis4max);
			std::cout << "\nGet maximum range:\n"
				<< m_axis1max << "\n"
				<< m_axis2max << "\n"
				<< m_axis3max << "\n"
				<< m_axis4max << std::endl;
			// get position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);


			SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Initialised (" + init.Name() + ")");
		}
		catch (...) {
			// Message as cout in the terminal of your producer
			std::cout << "Unknown exception" << std::endl;
			// Message to the LogCollector
			EUDAQ_ERROR("Unknown error during initialization from PI controller");
			// Otherwise, the State is set to ERROR
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Initialisation Error");
		}
	}

	virtual void PIController::OnConfigure(const eudaq::Configuration &config) {
		try {
			std::cout << "Configuring: " << config.Name() << std::endl;

			m_velocity1 = config.Get("Velocity1", -1.0);
			m_velocity2 = config.Get("Velocity2", -1.0);
			m_velocity3 = config.Get("Velocity3", -1.0);
			m_velocity4 = config.Get("Velocity4", -1.0);
			m_position1 = config.Get("Position1", -1.0);
			m_position2 = config.Get("Position2", -1.0);
			m_position3 = config.Get("Position3", -1.0);
			m_position4 = config.Get("Position4", -1.0);

			// Set velocity
			printf("\nSet individual velocity:\n");
			if (m_velocity1 >= m_velocitymax) { m_velocity1 = m_velocitymax; }
			if (m_velocity1 < 0) { printf("No velocity change for axis1!\n"); }
			else { wrapper->setVelocityStage(m_axis1, m_velocity1); }
			if (m_velocity2 >= m_velocitymax) { m_velocity2 = m_velocitymax; }
			if (m_velocity2 < 0) { printf("No velocity change for axis2!\n"); }
			else { wrapper->setVelocityStage(m_axis2, m_velocity2); }
			if (m_velocity3 >= m_velocitymax) { m_velocity3 = m_velocitymax; }
			if (m_velocity3 < 0) { printf("No velocity change for axis3!\n"); }
			else { wrapper->setVelocityStage(m_axis3, m_velocity3); }
			if (m_velocity4 >= m_velocitymax) { m_velocity4 = m_velocitymax; }
			if (m_velocity4 < 0) { printf("No velocity change for axis4!\n"); }
			else { wrapper->setVelocityStage(m_axis4, m_velocity4); }
			// get velocity // alternative: wrapper->getVelocityStage2(m_axis1, &m_velocity1);
			std::cout << "\nGet velocity:" << std::endl;
			wrapper->printVelocityStage(m_axis1);
			wrapper->printVelocityStage(m_axis2);
			wrapper->printVelocityStage(m_axis3);
			wrapper->printVelocityStage(m_axis4);

			// Move
			printf("\nMoving...\n");
			if (m_position1 >= m_axis1max) { m_position1 = m_axis1max; }
			if (m_position1 < 0) { printf("No movement of axis1!\n"); }
			else { wrapper->moveTo(m_axis1, m_position1); }
			if (m_position2 >= m_axis2max) { m_position2 = m_axis2max; }
			if (m_position2 < 0) { printf("No movement of axis2!\n"); }
			else { wrapper->moveTo(m_axis2, m_position2); }
			if (m_position3 >= m_axis3max) { m_position3 = m_axis3max; }
			if (m_position3 < 0) { printf("No movement of axis3!\n"); }
			else { wrapper->moveTo(m_axis3, m_position3); }
			if (m_position4 >= m_axis4max) { m_position4 = m_axis4max; }
			if (m_position4 < 0) { printf("No movement of axis4!\n"); }
			else { wrapper->moveTo(m_axis4, m_position4); }

			// New position	
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);

			SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + config.Name() + ")");
		}
		catch (...) {
			EUDAQ_ERROR("Unknown exception.");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
		}
	}

	virtual void PIController::OnStartRun(unsigned runnumber) {

		try {
			// No Action

			// get position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);

			SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Running");
		}
		catch (...) {
			EUDAQ_ERROR("Unknown exception.");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Unknown exception.");
		}
	}

	virtual void PIController::OnStopRun() {

		try {
			// No Action

			// get position
			printf("\nCurrent position:\n");
			wrapper->printPosition(m_axis1);
			wrapper->printPosition(m_axis2);
			wrapper->printPosition(m_axis3);
			wrapper->printPosition(m_axis4);

			SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
		}
		catch (const std::exception &e) {
			printf("While Stopping: Caught exception: %s\n", e.what());
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
		}
		catch (...) {
			printf("While Stopping: Unknown exception\n");
			SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Stop Error");
		}
	}

	virtual void PIController::OnTerminate() {

		std::cout << "PIController terminating..." << std::endl;

		m_terminated = true;
		std::cout << "PIController " << m_name << " terminated." << std::endl;
	}

	void PIController::Loop() {

		// Loop until Run Control tells us to terminate
		while (!m_terminated) {
			// Move this thread to the end of the scheduler queue:
			//sched_yield();
			eudaq::mSleep(50);
			continue;
		}

	}

private:
	std::shared_ptr<PIWrapper> wrapper;
	//PIWrapper wrapper(); 
	bool m_terminated;
	std::string m_name;
	char *m_hostname;
	int m_portnumber;
	char *m_axis1 = "1";
	char *m_axis2 = "2";
	char *m_axis3 = "3";
	char *m_axis4 = "4";
	double m_axis1max = 0.0;
	double m_axis2max = 0.0;
	double m_axis3max = 0.0;
	double m_axis4max = 0.0;
	double m_position1 = 0.0;
	double m_position2 = 0.0;
	double m_position3 = 0.0;
	double m_position4 = 0.0;
	double m_velocity1 = 0.0;
	double m_velocity2 = 0.0;
	double m_velocity3 = 0.0;
	double m_velocity4 = 0.0;
	double m_velocitymax = 10.0;
};

// The main function that will create a Producer instance and run it
int main(int /*argc*/, const char **argv) {
  // You can use the OptionParser to get command-line arguments
  // then they will automatically be described in the help (-h) option
  eudaq::OptionParser op("PI Stages Controller", "0.0", "Run options");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol",
                                   "tcp://localhost:44000", "address",
                                   "The address of the RunControl.");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name(op, "n", "name", "PIController", "string",
                                  "The name of this Producer/Controller");

  try {
    // This will look through the command-line arguments and set the options
    op.Parse(argv);
    // Set the Log level for displaying messages based on command-line
    EUDAQ_LOG_LEVEL(level.Value());

    // Create the instance
    PIController controller(name.Value(), rctrl.Value());
	
    // And set it running...
    controller.Loop();

    // When the keep-alive loop terminates, it is time to go
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    // This does some basic error handling of common exceptions
    return op.HandleMainException();
  }
  return 0;
}
