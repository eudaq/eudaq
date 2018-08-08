#include "AidaTluDisplay.hh"
#include <iostream>
#include <ostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include "uhal/uhal.hpp"
#include <thread>
#include <chrono>
#include <math.h>
#include <cmath> // std:: abs


LCD09052::LCD09052(){
  std::cout << "  AIDA_TLU: Instantiated display" << std::endl;
}

LCD09052::LCD09052(i2cCore * thisCore, char thisAddr, unsigned int theseRows, unsigned int theseCols){
  std::cout << "  AIDA_TLU: Instantiated display" << std::endl;
  disp_i2c_core= thisCore;
  disp_i2c_addr= thisAddr;
  nRows= theseRows;
  nCols= theseCols;
}

void LCD09052::setParameters( i2cCore  *mycore , char thisAddr, unsigned int theseRows, unsigned int theseCols){
  std::cout << "  AIDA_TLU DISPLAY: Setting parameters" << std::endl;
  disp_i2c_core = mycore;
  disp_i2c_addr= thisAddr;
  nRows= theseRows;
  nCols= theseCols;
}

void LCD09052::test(){
  std::cout << "TEST TEST TEST" << std::endl;
}

void LCD09052::setBrightness(unsigned int value){
  // Sets the brightness level of the backlight.
  // Value is an integer in range [0, 250]. 0= no light, 250= maximum light.
  if (value < 0){
    std::cout << "\tLCD09052 setBrightness: minimum value= 0. Coherced to 0" << std::endl;
    value = 0;
  }
  if (value > 250){
    std::cout << "\tLCD09052 setBrightness: maximum value= 250. Coherced to 250" << std::endl;
    value = 250;
  }
  int myaddr= 7;
  disp_i2c_core->WriteI2CChar(disp_i2c_addr, myaddr, (unsigned char)value);
}

void LCD09052::clear(){
  // Clears the display and locates the curson on position (1,1), i.e. top left
  int myaddr= 4;
  disp_i2c_core->WriteI2CChar(disp_i2c_addr, myaddr, 0x0);
}

void LCD09052::pulseLCD(unsigned int nCycles){
  // Pulse the LCD backlight
  float startP= 0;
  float endP= M_PI*nCycles;
  int stepsInCycle= 15;
  float nSteps= stepsInCycle*nCycles;
  float stepSize= M_PI/(stepsInCycle);
  float currVal=0;
  float iBright;

  for (float iCy= 0; iCy < nSteps; iCy++){
    currVal= currVal + stepSize;
    //iBright= int(250*abs(cos(currVal)));
    iBright= abs( ( 250*cos(currVal) ) );
    //std::cout << "\tGNE GNE GNE " << iCy << " " << currVal << " " << iBright << std::endl;
    setBrightness(iBright);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
