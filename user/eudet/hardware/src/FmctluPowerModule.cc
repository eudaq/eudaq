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
#include <thread>
#include <chrono>

void PWRLED::testme(){
  std::cout << "TEST TEST TEST" << std::endl;
}

PWRLED::PWRLED(){
  std::cout << "  TLU_POWERMODULE: Instantiated" << std::endl;
  //std::vector<RGB_array> indicatorXYZ;
  //std::array<RGB_array, 11> indicatorXYZ{{30, 29, 31}, {27, 26, 28}, {24, 23, 25}, {21, 20, 22}, {18, 17, 19}, {15, 14, 16}, {12, 11, 13}, {9, 8, 10}, {6, 5, 7}, {3, 2, 4}, {1, 0, -1}};
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
  pwr_ledExp1.SetI2CPar( pwr_i2c_core, pwr_i2c_exp1Add );
  pwr_ledExp2.SetI2CPar( pwr_i2c_core, pwr_i2c_exp2Add );

  pwr_ledExp1.setInvertReg(0, 0x00, false);// 0= normal, 1= inverted
  pwr_ledExp1.setIOReg(0, 0x00, false);// 0= output, 1= input
  pwr_ledExp1.setOutputs(0, 0xDA, false);// If output, set to XX

  pwr_ledExp1.setInvertReg(1, 0x00, false);// 0= normal, 1= inverted
  pwr_ledExp1.setIOReg(1, 0x00, false);// 0= output, 1= input
  pwr_ledExp1.setOutputs(1, 0xB6, false);// If output, set to XX

  pwr_ledExp2.setInvertReg(0, 0x00, false);// 0= normal, 1= inverted
  pwr_ledExp2.setIOReg(0, 0x00, false);// 0= output, 1= input
  pwr_ledExp2.setOutputs(0, 0x6D, false);// If output, set to XX

  pwr_ledExp2.setInvertReg(1, 0x00, false);// 0= normal, 1= inverted
  pwr_ledExp2.setIOReg(1, 0x00, false);// 0= output, 1= input
  pwr_ledExp2.setOutputs(1, 0xDB, false);// If output, set to XX
}

uint32_t PWRLED::_set_bit(uint32_t v, int index, bool x){
  ///Set the index:th bit of v to 1 if x is truthy, else to 0, and return the new value
  std::cout << "set bit IN " << v << std::endl;
  uint32_t mask;
  if (index == -1){
    std::cout << "  SETBIT: Index= -1 will be ignored" << std::endl;
  }
  else{
    mask = 1 << index;  // Compute mask, an integer with just bit 'index' set.
    v &= ~mask;         // Clear the bit indicated by the mask (if x is False)
    if ( x ) {
      v |= mask;        // If x was True, set the bit indicated by the mask.
    }
  }
  std::cout << " INDEX " << index << " bool " << x << " OUT " << v << std::endl;
  return v;
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

void PWRLED::setIndicatorRGB(int indicator, const std::array<int, 3>& RGB, bool verbose){
  // Indicator is one of the 11 LEDs on the front panels, labeled from 0 to 10
  // RGB allows to switch on (True) or off (False) the corresponding component for that Led
  // Note that one LED only has 2 components connected
  // print self.indicatorXYZ[indicator-1][2]
  //std::cout << "<><><><><><><>" << indicatorXYZ.at(ind).at(rgb) << std::endl;
  std::cout << ",.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.,.," << std::endl;
  if ((1 <= indicator) && (indicator <= 11)){
      unsigned char nowStatus[4];
      unsigned char nextStatus[4];
      nowStatus[0]= 0xFF & (pwr_ledExp1.getOutputs(0, false));
      nowStatus[1]= 0xFF & (pwr_ledExp1.getOutputs(1, false));
      nowStatus[2]= 0xFF & (pwr_ledExp2.getOutputs(0, false));
      nowStatus[3]= 0xFF & (pwr_ledExp2.getOutputs(1, false));
      std::cout << "NOW STATUS "<< (unsigned int)nowStatus[0] << " " << (unsigned int)nowStatus[1] << " " << (unsigned int)nowStatus[2] << " " << (unsigned int)nowStatus[3] << " " << std::endl;
      uint32_t nowWrd= 0x00000000;
      nowWrd= nowWrd | (unsigned int)nowStatus[0];
      nowWrd= nowWrd | ((unsigned int)nowStatus[1] << 8);
      nowWrd= nowWrd | ((unsigned int)nowStatus[2] << 16);
      nowWrd= nowWrd | ((unsigned int)nowStatus[3] << 24);
      uint32_t nextWrd;
      nextWrd= nowWrd;
      int indexComp;
      int valueComp;
      for (int iComp=0; iComp <3; iComp++){
        indexComp= indicatorXYZ.at(indicator-1).at(iComp);
        valueComp= ! bool( RGB.at(iComp) );
        nextWrd= _set_bit(nextWrd, indexComp, int(valueComp));
        if (verbose==2){
          std::cout << "n= " << iComp << " INDEX= " << indexComp << " VALUE= " << (unsigned int) valueComp << " NEXTWORD= " << std::hex << (nextWrd) << std::endl;
        }
      }
      if (verbose){
        std::cout << "NOW  " << std::hex << (nowWrd) << std::dec << std::endl;
        std::cout << "NEXT " << std::hex << (nextWrd) << std::dec << std::endl;
      }
      //nextStatus= [0xFF & nextWrd, 0xFF & (nextWrd >> 8), 0xFF & (nextWrd >> 16), 0xFF & (nextWrd >> 24) ];
      nextStatus[0]= 0xFF & nextWrd;
      nextStatus[1]= 0xFF & (nextWrd >> 8);
      nextStatus[2]= 0xFF & (nextWrd >> 16);
      nextStatus[3]= 0xFF & (nextWrd >> 24);
      if (nowStatus[0] != nextStatus[0]){
        pwr_ledExp1.setOutputs(0, nextStatus[0], false);
      }
      if (nowStatus[1] != nextStatus[1]){
        pwr_ledExp1.setOutputs(1, nextStatus[1], false);
      }
      if (nowStatus[2] != nextStatus[2]){
        pwr_ledExp2.setOutputs(0, nextStatus[2], false);
      }
      if (nowStatus[3] != nextStatus[3]){
        pwr_ledExp2.setOutputs(1, nextStatus[3], false);
      }

  }
}

void PWRLED::setVchannel(int channel, float voltage, bool verbose){
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
    pwr_zeDAC.SetDACValue(channel, int(dacValue));
  }
}

void PWRLED::led_allBlue(){
  pwr_ledExp1.setOutputs(0, 111, false);
  pwr_ledExp1.setOutputs(1, 219, false);
  pwr_ledExp2.setOutputs(0, 182, false);
  pwr_ledExp2.setOutputs(1, 109, false);
}

void PWRLED::led_allGreen(){
  pwr_ledExp1.setOutputs(0, 218, false);
  pwr_ledExp1.setOutputs(1, 182, false);
  pwr_ledExp2.setOutputs(0, 109, false);
  pwr_ledExp2.setOutputs(1, 219, false);
}

void PWRLED::led_allRed(){
  pwr_ledExp1.setOutputs(0, 181, false);
  pwr_ledExp1.setOutputs(1, 109, false);
  pwr_ledExp2.setOutputs(0, 219, false);
  pwr_ledExp2.setOutputs(1, 182, false);
}

void PWRLED::led_allOff(){
  pwr_ledExp1.setOutputs(0, 255, false);
  pwr_ledExp1.setOutputs(1, 255, false);
  pwr_ledExp2.setOutputs(0, 255, false);
  pwr_ledExp2.setOutputs(1, 255, false);
}

void PWRLED::led_allWhite(){
  pwr_ledExp1.setOutputs(0, 0, false);
  pwr_ledExp1.setOutputs(1, 0, false);
  pwr_ledExp2.setOutputs(0, 0, false);
  pwr_ledExp2.setOutputs(1, 0, false);
}

void PWRLED::testLED(){
  std::array<int, 3> RGB{{0, 1, 0}};
  led_allOff();
  for (int iInd=1; iInd < 12; iInd++){
    setIndicatorRGB(iInd, RGB, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  setIndicatorRGB(11, {{0,0,0}}, false);
}
