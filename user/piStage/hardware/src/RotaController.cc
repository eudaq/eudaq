/*
Library to access a PI GCS controller
tested for a PI C-863 Mercurz DC Motor Controller connected to a rotational stage
written by Peter Wimberger (summer student, online: @topsrek, always avilable for questions) 
and Stefano Mersi (supervisor) in Summer 2018
tested for the XDAQ Supervisor for CMS telescope CHROMIE at CERN

 Changes by Lennart Huth <lennart.huth@desy.de>
*/
#include "RotaController.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <cmath>
#include <iostream>

//Physik Instrumente General Command Set library:
#include "PI_GCS2_DLL.h"
//also defines the BOOL type (wich is just an int)
//manual for this library is named PIGCS_2_0_DLL_SM151E210.pdf

RotaController::RotaController()
{
    ID = -1; //initialize to an impossible value to detect errors in the future
}

RotaController::~RotaController()
{
    PI_CloseConnection(ID); //is actually not necessary, but feels right to do
}

int RotaController::getID() //get internal ID used to identify the controller
{
    return ID;
}

bool RotaController::connect(const char *devnam, int baudrat)
{
    devname = devnam;
    baudrate = baudrat;
    //we use the devname function to detect the ID
    //because we connected the stage over USB (mounted in /dev/ttyUSB*)
    //you could also use the following function for different setups:
    //ID = PI_TryConnectRS232(1, 0); //this call ensures that it works the first time after power up
    //std::cerr << "ID = PI_ConnectRS232ByDevName(devname, baudrate);" << std::endl;

      ID = PI_ConnectRS232ByDevName(devname, baudrate);
//    ID = PI_ConnectUSB(devname);
   // ID = PI_ConnectUSBWithBaudRate(devname, baudrate);
//    ID = PI_ConnectRS232(1, 38400); // port 1 is "/dev/ttyS0"
//    ID = PI_ConnectTCPIP("192.168.22.123",50000);
    if (ID < 0)
    {
        RotaController::ConnectionError("Could not connect to Mercury Controller!");
        return false;
    }
    
/*  The following 
BOOL isConnecting = TRUE;
    // std::cerr << "PI_IsConnecting" << std::endl;
    while (isConnecting)
        PI_IsConnecting(ID, &isConnecting);
    // std::cerr << "Connection is over" << std::endl; */ 
    BOOL isConnected = PI_IsConnected(ID);
    // std::cerr << "Should be done" << std::endl;
    // this call ensures that the Controller is not in an error state after power up
    // (It resets the error state on the device) and makes it receivable for new commands
    bool isReady = true;
    if (isConnected == TRUE)
    {
        LogInfo(std::string("Connected to device with (internal) ID ") + std::to_string(ID));
    }
    else
    {
        LogInfo(std::string("Device not connected on ID ") + std::to_string(ID));
        isReady = false;
    }    int error_code = PI_GetError(ID);
    if (error_code != 0)
    {
        LogInfo(std::string("Device reported error code ") + std::to_string(error_code));
        isReady = false;
    }
    return isReady;
}

bool RotaController::connectTCPIP(std::string IP, int port)
{
    std::cout << "Connecting to "<< IP <<", "<< port << std::endl;
    ID = PI_ConnectTCPIP(IP.c_str(),port);
    if (ID < 0)
    {
        RotaController::ConnectionError("Could not connect to Controller vias TCPIP!");
        return false;
    }
    BOOL isConnected = PI_IsConnected(ID);
    bool isReady = true;
    if (isConnected == TRUE)
    {
        LogInfo(std::string("Connected to device with (internal) ID ") + std::to_string(ID));
    }
    else
    {
        LogInfo(std::string("Device not connected on ID ") + std::to_string(ID));
        isReady = false;
    }
    int error_code = PI_GetError(ID);
    if (error_code != 0)
    {
        LogInfo(std::string("Device reported error code ") + std::to_string(error_code));
        isReady = false;
    }
    return isReady;
}

bool RotaController::checkIDN() //logs the identification string (using command *IDN?)
{
    char buffer[255];
    if (!PI_qIDN(ID, buffer, 255)) //*IDN? is a specially formatted command to get the Identification of the connected device (Manufacturer, modelnumber etc.)
    {
        HardError("Could not read *IDN?  : (Identification string)");
        return false;
    }
    std::string msg = "Connected to: ";
    std::string str_buffer(buffer);
    LogInfo(msg + str_buffer);
    return true;
}

std::string RotaController::getParameterAddresses()
{
    char buffer2[32768];              //large buffer, because long message
    if (!PI_qHPA(ID, buffer2, 32768)) //*IDN? is a specially formatted command to get the Identification of the connected device (Manufacturer, modelnumber etc.)
    {
        SoftError("Could not read HPA?  : (Parameter string)");
        return std::string("no data");
    }

    return std::string(buffer2);
}

bool RotaController::setParameter(const char *szAxis, const unsigned int *parameterAddress, const double *parameterValue, const char *szStrings)
//szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
{
    if (!PI_SPA(ID, szAxis, parameterAddress, parameterValue, szStrings))
    {
        SoftError(std::string("Cannot set parameter with address of ") + std::to_string(*parameterAddress) + " to value of " + std::to_string(*parameterValue));
        return false;
    }
    LogInfo(std::string("Set parameter with address ") + std::to_string(*parameterAddress) + " to value of " + std::to_string(*parameterValue));
    return true;
}

bool RotaController::getParameter(const char *szAxis, unsigned int *parameterAddress, double *parameterValue, char *szStrings, int maxNameSize)
//szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
{
    if (!PI_qSPA(ID, szAxis, parameterAddress, parameterValue, szStrings, maxNameSize))
    {
        SoftError("cannot get parameter");
        return false;
    }
    return true;
}

bool RotaController::setNewAxisName(const char *axisname, const char *newaxisname)
//new axis name stays saved to controller even after re-power
{
    if (!PI_SAI(ID, axisname, newaxisname)) //Set new axisname for axis with axisname
    {
        HardError("Could not set new Axis name");
        return false;
    }
    LogInfo(std::string("Changed name for axis: ") + axisname + " to: " + std::string(newaxisname));
    return true;
}

bool RotaController::checkListAllAxes() //logs all axis identifiers (using command SAI?)
{
    char buffer[255];
    if (!PI_qSAI_ALL(ID, buffer, 255))
    { //Get list of current Axis Identifies
        HardError("Could not read SAI? ALL  : (List of all current axis identifiers)");
        return false;
    }
    LogInfo(std::string("List of connected axes: ") + std::string(buffer));
    return true;
}

bool RotaController::initializeStage(const char *szAxis, const char *szStage)
{
    LogInfo(std::string("Initializing axis \"") + std::string(szAxis) + "\" with stage type: \"" + std::string(szStage) + "\"");
    char buffer[255];
    if (!PI_qCST(ID, szAxis, buffer, 255)) //Get Assignment of stages to axes
    {
        SoftError("qCST failed  : (Assignment of stages to axes)");
        return false;
    }
    char *pStage = strchr(buffer, '=');                     // get pointer of first occurence of = in buffer
    pStage++;                                               //content after first = in buffer
    if (strncasecmp(szStage, pStage, strlen(szStage)) == 0) //Compare Strings without Case Sensitivity
    {
        LogInfo("Stage type already defined");
        return true;
    }

    if (!PI_CST(ID, szAxis, szStage)) //Assign Stage to Axis
    {
        SoftError(std::string("CST failed  : (Assignment of Stage ") + std::string(szStage) + std::string(" to axis ") + std::string(szAxis) + std::string("failed.)"));
        return false;
    }

    return true;
}

bool RotaController::reference(const char *szAxis, bool *alreadyReferenced) //does a reference movement if needed (determine current position relative to reference switch)
                                                                            //A stage could also only have reference limits on the ends instead of a reference switch in the middle. refer to manual of stage and/or controller
//This function prefers to use the reference switch, because it is generally more desireable to move near the center of the stage than to the ends

{
    BOOL bServoState = TRUE;               //uppercase because it comes from hardware (gcs) library
    if (!PI_SVO(ID, szAxis, &bServoState)) //set servo state
    {
        HardError("SVO failed  : could not set servo state");
        return false;
    }

    BOOL bRefOK = FALSE;

    if (!PI_qFRF(ID, szAxis, &bRefOK)) //asks if a reference result is already available
    {
        HardError("qFRF failed  : could not determine if reference data is already available");
        return false;
    }
    if (bRefOK)
    {
        LogInfo(std::string("Axis ") + std::string(szAxis) + std::string(" already referenced"));
        *alreadyReferenced = true;
        return true;
    }
    else
    {
        *alreadyReferenced = false;
    }

    BOOL bHasRerenceOrLimitSwitch = FALSE;
    if (!PI_qTRS(ID, szAxis, &bHasRerenceOrLimitSwitch)) //Indicate reference switch
    {
        HardError("qTRS failed  : Could not indicate reference switch");
        return false;
    }
    if (bHasRerenceOrLimitSwitch) // stage has reference switch
    {
        LogInfo("Stage has reference switch");
        if (!PI_FRF(ID, szAxis)) //Fast refernce move to reference switch
        {
            HardError("FRF failed  : Fast reference move to reference switch");
            return false;
        }
        LogInfo(std::string("Starting Reference move for axis ") + std::string(szAxis) + std::string(" by reference switch."));
        //LogInfo("waiting for controller finishing reference move");
    }
    else
    {
        LogInfo("stage does not have reference switch");
        if (!PI_qLIM(ID, szAxis, &bHasRerenceOrLimitSwitch)) // Indicate Limit switches
        {
            HardError("qLIM failed  : Indicate Limit switches");
            return false;
        }
        if (bHasRerenceOrLimitSwitch)
        {
            if (!PI_FNL(ID, szAxis)) // Fast reference to negative limit
            {
                HardError("FNL failed  : Fast reference to negative limit");
                return false;
            }
            LogInfo(std::string("Starting Reference move for axis ") + std::string(szAxis) + std::string(" by negative limit switch."));
            //LogInfo("waiting for controller finishing reference move");
        }
        else
        {
            LogInfo(std::string("Stage for axis ") + std::string(szAxis) + std::string("has neither reference nor limit switch!"));
            return false;
        }
    }
    std::cout << "moving to reference: "<<bRefOK<<std::endl;

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "searching ref: "<<bRefOK<<std::endl;
        if (!PI_IsControllerReady(ID, &bRefOK))
        {
            SoftError("IsControllerReady failed");
            return false;
        }
    } while (!bRefOK);
    LogInfo("Reference move done. Controller ready again.");

    if (!PI_qFRF(ID, szAxis, &bRefOK)) //Get Referencing result
    {
        HardError("qFRF failed  : could not get referencing result");
        return false;
    }
    if (bRefOK)
    {
        LogInfo(std::string("Axis ") + std::string(szAxis) + std::string(" successfully referenced."));
        return true;
    }
    else
    {
        HardError(std::string("Reference failed") + std::string("Axis ") + std::string(szAxis) + std::string(" not referenced!"));
        return false;
    }
}

bool RotaController::reference(const char *szAxis)
{
    bool *alreadyReferencedPlaceHolder = new bool(false);
    return reference(szAxis, alreadyReferencedPlaceHolder);
}

bool RotaController::defineNewHome(const char *Axis, double newHome)
{
    move(Axis, newHome);
    if (!PI_DFH(ID, Axis))
    {
        SoftError(std::string("DFH failed: Could not define new Home/Zero Position for Axis ") + std::string(Axis));
        return false;
    }
    return true;
}

double RotaController::getMinLimit(const char *Axis)
{
    double initTravelMin = std::nan("1");
    double *travelMin = &initTravelMin;
    if (!PI_qTMN(ID, Axis, travelMin))
    {
        SoftError(std::string("qTMN failed  : Could not get Minimum Limit for Axis ") + std::string(Axis));
    }
    return *travelMin;
}

double RotaController::getMaxLimit(const char *Axis)
{
    double initTravelMax = std::nan("1");
    double *travelMax = &initTravelMax;
    if (!PI_qTMX(ID, Axis, travelMax))
    {
        SoftError(std::string("qTMX failed  : Could not get Maximum Limit for Axis ") + std::string(Axis));
    }
    return *travelMax;
}

bool RotaController::move(const char *szAxis, double target, bool keepServoOn)
{
    BOOL servoState = TRUE;               //uppercase because it comes from hardware (gcs) library
    if (!PI_SVO(ID, szAxis, &servoState)) //Set Servo state
    {
        HardError("SVO failed  : could not start servo (motor)");
        return false;
    }
    if (!PI_MOV(ID, szAxis, &target)) //Move Axis to target position
    {
        HardError("MOV failed  : could not initiate Movement");
        return false;
    }
    BOOL bFlag = true;
    double position;
    LogInfo(std::string("Moving axis ") + std::string(szAxis) + std::string(" to target position ") + std::to_string(target));
    do
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!PI_IsMoving(ID, szAxis, &bFlag))
        {
            SoftError(std::string("IsMoving failed  : Could not determine stage movement status of Axis ") + std::string(szAxis));
            return false;
        }
        //std::this_thread::sleep_for(std::chrono::milliseconds(0)); //this sleep time ensures that the controller actually came to a halt before we measure its actual position
        if (!PI_qPOS(ID, szAxis, &position))
        {
            SoftError(std::string("qPOS failed  : Could not get current position of Axis ") + std::string(szAxis));
            return false;
        }
        //LogInfo(std::string("Axis ") + std::string(szAxis) + std::string(" currently at ") + std::to_string(position));
    } while (bFlag);
    LogInfo(std::string("Axis ") + std::string(szAxis) + std::string(" finished moving to ") + std::to_string(position));

    if(!keepServoOn){
        servoState = FALSE;                   //uppercase because it comes from hardware (gcs) library
        if (!PI_SVO(ID, szAxis, &servoState)) //Set Servo state
        {
            HardError("SVO failed  : could not turn off servo (motor)");
            return false;
        }
    }
    return true;
}

bool RotaController::checkMovingState(const char *szAxis, bool *isMoving, double *currentPosition)
{
    BOOL bFlag = true;
    if (!PI_IsMoving(ID, szAxis, &bFlag))
    {
        SoftError(std::string("IsMoving failed  : Could not determine stage movement status of Axis ") + std::string(szAxis));
        return false;
    }
    if (bFlag)
    {
        *isMoving = true;
        return true;
    }
    else
    {
        if (!PI_qPOS(ID, szAxis, currentPosition))
        {
            SoftError(std::string("qPOS failed  : Could not get current position of Axis ") + std::string(szAxis));
            return false;
        }
        return true;
    }
}

bool RotaController::GoHome(const char *Axis)
{
    if (!PI_GOH(ID, Axis))
    {
        HardError(std::string("GOH failed: Could not go to home position for Axis ") + std::string(Axis));
        return false;
    }
    return true;
}

bool RotaController::HaltSmoothly(const char *Axis)
{
    if (!PI_HLT(ID, Axis))
    {
        HardError(std::string("HLT failed: Could not halt motion (smoothly) for Axis ") + std::string(Axis));
        return false;
    }
    return true;
}

bool RotaController::Reboot()
{
    if (!PI_RBT(ID))
    {
        HardError(std::string("RBT failed: Could not reboot"));
        return false;
    }
    return true;
}

bool RotaController::Version(char *szBuffer, int iBuffersize)
{
    if (!PI_qVER(ID, szBuffer, iBuffersize))
    {
        SoftError(std::string("qVer failed: Could not get Version of Firmware and Drivers"));
        return false;
    }
    return true;
}

void RotaController::HardError(const char *szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage, error, szError);
    *defaulterrorstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::HardError(std::string szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage.c_str(), error, szError);
    *defaulterrorstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::SoftError(const char *szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage, error, szError);
    *defaulterrorstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::SoftError(std::string szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage.c_str(), error, szError);
    *defaulterrorstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::ConnectionError(const char *errorMessage)
{
    *defaulterrorstream << errorMessage << std::endl;
}

void RotaController::LogInfo(const char *infoMessage)
{
    *defaultlogstream << infoMessage << std::endl;
}

void RotaController::LogInfo(std::string infoMessage)
{
    *defaultlogstream << infoMessage << std::endl;
}

void RotaController::HardError(std::ostream &outstream, const char *szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage, error, szError);
    outstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::HardError(std::ostream &outstream, std::string szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage.c_str(), error, szError);
    outstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::SoftError(std::ostream &outstream, const char *szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage, error, szError);
    outstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::SoftError(std::ostream &outstream, std::string szMessage)
{
    int error = PI_GetError(ID);
    char szError[1024];
    PI_TranslateError(error, szError, 1024);
    //printf("%s - error %d, %s\n", szMessage.c_str(), error, szError);
    outstream << szMessage << " - error " << error << ", " << szError << std::endl;
}

void RotaController::ConnectionError(std::ostream &outstream, const char *errorMessage)
{
    outstream << errorMessage << std::endl;
}

void RotaController::LogInfo(std::ostream &outstream, const char *infoMessage)
{
    outstream << infoMessage << std::endl;
}

void RotaController::LogInfo(std::ostream &outstream, std::string infoMessage)
{
    outstream << infoMessage << std::endl;
}
