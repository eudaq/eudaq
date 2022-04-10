#include "eudaq/Producer.hh"
#include "RotaController.h"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#endif
class PIStageProducer : public eudaq::Producer {
  public:
  PIStageProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("PIStageProducer");
private:
  // Setting up the stage relies on he fact, that the PI stage copmmand set GCS holds for the device, tested with C-863 and c-884
  void setupStage(std::string axisname, double refPos, double rangeNegative, double rangePositive, double Speed = 1);
  bool m_flag_ts;
  bool m_flag_tg;
  bool m_connected_X, m_connected_Y, m_connected_Rot;
  std::string m_axis_X, m_axis_Y, m_axis_Rot;
  uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  bool m_allow_changes, m_connected;
  RotaController *m_controller;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<PIStageProducer, const std::string&, const std::string&>(PIStageProducer::m_id_factory);
}

PIStageProducer::PIStageProducer(const std::string & name, const std::string & runcontrol)
  : eudaq::Producer(name, runcontrol), m_file_lock(0), m_allow_changes(false), m_connected(false)
    , m_connected_X(false), m_connected_Y(false), m_connected_Rot(false){
    m_controller = new RotaController();
}



void PIStageProducer::DoInitialise(){



  m_allow_changes = true;


  // We can skip everything if it is already connected:
  if(m_connected && m_controller->checkIDN()>=0)
      return;

  auto ini = GetInitConfiguration();
  std::string controllerIP = ini->Get("ControllerIP", "192.168.22.123");
  int port = ini->Get("port", 50000);
  if(!m_controller->connectTCPIP(controllerIP, port))
      EUDAQ_THROW("Cannot connect to PI-Stage-Controller");
  // to be used with the C-863 controller
  //if(!m_controller->connect("/dev/ttyUSB0",38400))
  //    EUDAQ_THROW("Cannot connect to PI-Stage-Controller");

  m_axis_X           = ini->Get("axisX","Disconnected");
  m_axis_Y           = ini->Get("axisY","Disconnected");
  m_axis_Rot         = ini->Get("axisRot","Disconnected");
  m_connected_X     = (m_axis_X!= "Disconnected");
  m_connected_Y     = (m_axis_Y!= "Disconnected");
  m_connected_Rot   = (m_axis_Rot!= "Disconnected");
  double initX      = ini->Get("initX",0.0);
  double initY      = ini->Get("initY",0.0);
  double initRot    = ini->Get("initRot",0.0);
  double velocityX  = ini->Get("velocityX",5.0);
  double velocityY  = ini->Get("velocityY",5.0);
  double velocityRot= ini->Get("velocityRot",2.5);
  double negLimitX  = ini->Get("negativeLimitX",-100.0);
  double posLimitX  = ini->Get("positiveLimitX",100.0);
  double negLimitY  = ini->Get("negativeLimitY",-100.0);
  double posLimitY  = ini->Get("positiveLimitY",100.0);
  double negLimitRot  = ini->Get("negativeLimitRot",-360.0);
  double posLimitRot  = ini->Get("positiveLimitRot",360.0);

  std::string linStageX  = ini->Get("linStageTypeX","no_type");
  std::string linStageY  = ini->Get("linStageTypeY","no_type");
  std::string rotStage  = ini->Get("rotStageType","no_type");

  bool forceInit        = ini->Get("forceInit",true);
  // Supports only x,y and rot movements
  if ( m_connected_X && !(linStageX=="no_type")&& !m_controller->initializeStage(m_axis_X.c_str(), linStageX.c_str()))
      EUDAQ_THROW("Failed to initialize linear stage along x-axis check stage model and limit switches");
  if (m_connected_Y && !(linStageY=="no_type") && !m_controller->initializeStage(m_axis_Y.c_str(), linStageY.c_str()))
      EUDAQ_THROW("Failed to initialize linear stage along y-axis check stage model and limit switches");
  if ( m_connected_Rot && !(rotStage=="no_type") && !m_controller->initializeStage(m_axis_Rot.c_str(), rotStage.c_str()))
      EUDAQ_THROW("Failed to initialize rotational stage M-060.DG");
  // setup stages - movements are limited to avoid damage of hardware
  if(m_connected_X)  setupStage(m_axis_X,initX,negLimitX,posLimitX,velocityX);
  if(m_connected_Y)  setupStage(m_axis_Y,initY,negLimitY,posLimitY,velocityY);
  if(m_connected_Rot)setupStage(m_axis_Rot,initRot,negLimitRot,posLimitRot,velocityRot);

  // init stages
  if (m_connected_X && !m_controller->reference(m_axis_X.c_str(), &forceInit))
      EUDAQ_THROW("Failed to reference linear stage M-521.DD1 along x-axis");
  if (m_connected_Y && !m_controller->reference(m_axis_Y.c_str(), &forceInit))
      EUDAQ_THROW("Failed to reference linear stage M-521.DD1 along y-axis");
  if (m_connected_Rot && !m_controller->reference(m_axis_Rot.c_str(), &forceInit))
      EUDAQ_THROW("Failed to reference rotational stage M-060.DG");

  m_connected = true;
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void PIStageProducer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  if(!m_allow_changes)
      EUDAQ_THROW("Refusing to change position as the run is still ongoing");

  double pX     = conf->Get("positionX",-10000);
  double pY     = conf->Get("positionY",-10000);
  double pRot   = conf->Get("positionRot",-10000);
  bool servoX   = conf->Get("keepServoOnX",1);
  bool servoY   = conf->Get("keepServoOnY",1);
  bool servoRot = conf->Get("keepServoOnRot",1);

  if(pX == -10000 && m_connected_X)
         EUDAQ_THROW("No movement position given for X");
  if(pY == -10000 && m_connected_Y)
         EUDAQ_THROW("No movement position given for Y");
  if(pRot == -10000 && m_connected_Rot)
         EUDAQ_THROW("No movement position given for Rot");

  if(m_connected_X)     if(!m_controller->move(m_axis_X.c_str(),pX,servoX)) EUDAQ_THROW("X Movement failed");
  if(m_connected_Y)     if(!m_controller->move(m_axis_Y.c_str(),pY,servoY)) EUDAQ_THROW("Y Movement failed");
  if(m_connected_Rot)   if(!m_controller->move(m_axis_Rot.c_str(),pRot,servoRot)) EUDAQ_THROW("Rotation Movement failed");

}

void PIStageProducer::DoStartRun(){
  m_allow_changes = false;
}

void PIStageProducer::DoStopRun(){
  m_allow_changes = true;
}

void PIStageProducer::DoReset(){
 m_allow_changes = true;
}

void PIStageProducer::DoTerminate(){
}
void PIStageProducer::RunLoop(){
    // nothing to be done here - stage is not moving
}


void PIStageProducer::setupStage(std::string axisname, double refPos, double rangeNegative, double rangePositive, double Speed)
{
    const unsigned int *ref_val_parameteraddress = new unsigned int(22);
    const double *ref_val_parametervalue = new double(refPos);
    const char ref_val_szStrings[512] = ""; //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->setParameter(axisname.c_str(), ref_val_parameteraddress, ref_val_parametervalue, ref_val_szStrings);
    std::cout << "----Set address: " << *ref_val_parameteraddress << "\tvalue: " << *ref_val_parametervalue << std::endl;

    //Parameter 21 (0x15): Maximum Travel In Positive Direction (Phys. Unit)
    const unsigned int *max_pos_parameteraddress = new unsigned int(21);
    const double *max_pos_parametervalue = new double(rangePositive);
    const char max_pos_szStrings[512] = ""; //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->setParameter(axisname.c_str(), max_pos_parameteraddress, max_pos_parametervalue, max_pos_szStrings);
    std::cout << "----Set address: " << *max_pos_parameteraddress << "\tvalue: " << *max_pos_parametervalue << std::endl;

    //Parameter 48 (0x30): Maximum Travel In Negative Direction (Phys. Unit)
    const unsigned int *min_pos_parameteraddress = new unsigned int(48);
    const double *min_pos_parametervalue = new double(rangeNegative);
    const char min_pos_szStrings[512] = ""; //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->setParameter(axisname.c_str(), min_pos_parameteraddress, min_pos_parametervalue, min_pos_szStrings);
    std::cout << "----Set address: " << *min_pos_parameteraddress << "\tvalue: " << *min_pos_parametervalue << std::endl;

    //Parameter 63 (0x3F): On-Target-Status Settling Time
    const unsigned int *parameteraddress = new unsigned int(63);
    const double *parametervalue = new double(0.01); //a non-zero value is needed to measure the achieved position accurately after moving
    const char szStrings[512] = "";                    //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->setParameter(axisname.c_str(), parameteraddress, parametervalue, szStrings);
    std::cout << "----Set address: " << *parameteraddress << "\tvalue: " << *parametervalue << std::endl;

    unsigned int *get_parameteraddress = new unsigned int(63);
    double *get_parametervalue = new double(0);
    char get_szStrings[2048] = "";                                                               //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->getParameter(axisname.c_str(), get_parameteraddress, get_parametervalue, get_szStrings, 2048); //2048 is the maxNameSize for the string
    std::cout << "----Got address: " << *get_parameteraddress << "\tvalue: " << *get_parametervalue << "\tstring: " << get_szStrings << std::endl;

    //Parameter 73 (0x49): Closed-Loop Velocity (means normal velocity)
    const unsigned int *parameteraddress2 = new unsigned int(73);
    const double *parametervalue2 = new double(Speed); //phys. unit/s
    const char *szStrings2 = new char[512];        //szStrings is needed for parameters, which require chars (for example stage name: 0x3C)
    m_controller->setParameter(axisname.c_str(), parameteraddress2, parametervalue2, szStrings2);
    std::cout << "----Set address: " << *parameteraddress2 << "\tvalue: " << *parametervalue2 << std::endl;

    //Parameter 12: Closed-Loop Deceleration
    unsigned int *get_parameteraddress2 = new unsigned int(12);
    double *get_parametervalue2 = new double(0.2);
    char get_szStrings2[2048] = "";
    m_controller->getParameter(axisname.c_str(), get_parameteraddress2, get_parametervalue2, get_szStrings2, 2048);
    std::cout << "----Got address: " << *get_parameteraddress2 << "\tvalue: " << *get_parametervalue2 << "\tstring: " << get_szStrings2 << std::endl;
    std::cout << "MinLimit: " << m_controller->getMinLimit(axisname.c_str()) << std::endl;
    std::cout << "MaxLimit: " << m_controller->getMaxLimit(axisname.c_str()) << std::endl;
    std::cout << "Set to reference ...";

}
