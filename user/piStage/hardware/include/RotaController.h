/*
Library to access a PI GCS controller
tested for a PI C-863 Mercurz DC Motor Controller connected to a rotational stage
written by Peter Wimberger (summer student, online: @topsrek, always avilable for questions) 
and Stefano Mersi (supervisor) in Summer 2018
tested for the XDAQ Supervisor for CMS telescope CHROMIE at CERN

Changes by Lennart Huth <lennart.huth@desy.de>
*/

#ifndef ROTACONTROLLER_H //header guard
#define ROTACONTROLLER_H

#include <string>
#include <iostream>

class RotaController
{
public:
  RotaController();
  virtual ~RotaController();
  int getID();
  bool connect(const char *devnam, int baudrat); //checks devname and gets the correct ID
  bool connectTCPIP(std::string IP, int port);
  bool checkIDN();                     //check identification string coming from the device
  std::string getParameterAddresses(); //run HPA?  get all parameters available to inspect (with SPA? and SEP?) and its corresponding hex addresses from the controller
  bool setParameter(const char *szAxis, const unsigned int *parameterAddress, const double *parameterValue, const char *szStrings);
  bool getParameter(const char *szAxis, unsigned int *parameterAddress, double *parameterValue, char *szStrings, int maxNameSize);
  bool checkListAllAxes();
  bool setNewAxisName(const char *axisname, const char *newaxisname); //set a new name (newaxisname) for axis with axisname
  //new axis name stays saved to controller even after re-power
  bool initializeStage(const char *szAxis, const char *szStage); //tell the library which stage (and its parameters) to use for given axis
  bool reference(const char *szAxis, bool *alreadyReferenced);
  bool reference(const char *szAxis); //does a reference movement if needed (determine current position relative to reference switch)
  //A stage could also only have reference limits on the ends instead of a reference switch in the middle. refer to manual of stage and/or controller
  //also sets the servo state to active/true.
  bool defineNewHome(const char *Axis, double newHome);
  double getMinLimit(const char *szAxis);
  double getMaxLimit(const char *szAxis);
  bool move(const char *szAxis, double target, bool keepServoOn= true);
  bool GoHome(const char *Axis);
  bool HaltSmoothly(const char *Axis);
  bool Reboot();
  bool Version(char *szBuffer, int iBuffersize);
  bool checkMovingState(const char *szAxis, bool *isMoving, double *currentPosition);

  void HardError(std::ostream&, const char *szMessage);
  void HardError(std::ostream&, std::string szMessage);
  void SoftError(std::ostream&, const char *szMessage);
  void SoftError(std::ostream&, std::string);
  void ConnectionError(std::ostream&, const char *errorMessage);
  void LogInfo(std::ostream&, std::string infoMessage);
  void LogInfo(std::ostream&, const char *infoMessage);

  std::ostream* defaulterrorstream = &std::cout;
  std::ostream* defaultlogstream = &std::cout;



private:
  const char *devname;
  int baudrate;
  int ID;
  //std::unordered_set<const char* > axes;
  //these virtual functions can be adapted by you with minimal effot
  virtual void HardError(const char *szMessage);
  virtual void HardError(std::string szMessage);
  virtual void SoftError(const char *szMessage);
  virtual void SoftError(std::string);
  virtual void ConnectionError(const char *errorMessage);
  virtual void LogInfo(std::string infoMessage);
  virtual void LogInfo(const char *infoMessage);

};

#endif

//this libraray took advantage of the sample source files PI supplied ('PI_GCS2_Sample_two_axes.cpp')
//Please be aware that the sample for single_axis has some logical errors built in (bools in referenceIfNeeded(), but it still compiles.
