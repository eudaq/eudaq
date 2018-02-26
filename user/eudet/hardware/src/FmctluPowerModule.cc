#include "FmctluPowerModule.hh"
#include <iostream>
#include <ostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include "uhal/uhal.hpp"
#include "FmctluHardware.hh"


void PWRLED::testme(){
  std::cout << "TEST TEST TEST" << std::endl;
}

PWRLED::PWRLED(){
  std::cout << "  TLU_POWERMODULE: Instantiated" << std::endl;
}

PWRLED::PWRLED( i2cCore  *mycore , char DACaddr, char Exp1Add, char Exp2Add){
  pwr_i2c_core = mycore;
  pwr_i2c_DACaddr= DACaddr;
  pwr_i2c_exp1Add= Exp1Add;
  pwr_i2c_exp2Add= Exp2Add;
  std::cout << "  TLU_POWERMODULE: Instantiated" << std::endl;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)DACaddr << std::dec << "(DAC)" << std::endl;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)Exp1Add << std::dec << "(LED EXPANDER 1)" << std::endl;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)Exp2Add << std::dec << "(LED EXPANDER 1)" << std::endl;
}

void PWRLED::initI2Cslaves(bool intRef, bool verbose){
  pwr_zeDAC.SetI2CPar(pwr_i2c_core, pwr_i2c_DACaddr);
  pwr_zeDAC.SetIntRef(intRef, verbose);
}

void PWRLED::setI2CPar( i2cCore  *mycore , char DACaddr, char Exp1Add, char Exp2Add){
  pwr_i2c_core = mycore;
  pwr_i2c_DACaddr= DACaddr;
  pwr_i2c_exp1Add= Exp1Add;
  pwr_i2c_exp2Add= Exp2Add;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)DACaddr << std::dec << "(DAC)" << std::endl;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)Exp1Add << std::dec << "(LED EXPANDER 1)" << std::endl;
  std::cout << "\tI2C addr: 0x" << std::hex<< (int)Exp2Add << std::dec << "(LED EXPANDER 1)" << std::endl;
}

void PWRLED::setVch(int channel, float voltage, bool verbose){
  // Note: the channel here is the DAC channel.
  // The mapping with the power module is not one-to-one
  float dacValue;
  if ((channel < 0) | (3 < channel )){
    std::cout<< "  PWRModule: channel should be comprised between 0 and 3" << std::endl;
  }
  else{
    if (voltage < 0){
      std::cout<< "PWRModule: voltage must be comprised between 0 and 1 V. Coherced to 0 V." << std::endl;
      voltage = 0;
    }
    if (voltage > 1){
      std::cout<< "PWRModule: voltage must be comprised between 0 and 1 V. Coherced to 1 V." << std::endl;
      voltage = 1;
    }
    dacValue= voltage*65535;
    pwr_zeDAC.SetDACValue(int(dacValue), channel);
  }
}
