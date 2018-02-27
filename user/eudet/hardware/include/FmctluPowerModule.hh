#ifndef H_FMCTLUPOWERMODULE_HH
#define H_FMCTLUPOWERMODULE_HH

#include <string>
#include <iostream>
#include <vector>
#include "FmctluI2c.hh"
#include "FmctluHardware.hh"

class PWRLED{
private:
  char pwr_i2c_DACaddr;
  char pwr_i2c_exp1Add;
  char pwr_i2c_exp2Add;
  i2cCore * pwr_i2c_core;
  AD5665R pwr_zeDAC;
  PCA9539PW pwd_ledExp1, pwr_ledExp2;
  //i2cCore myi2c;
public:
  //AD5665R::AD5665R()
  PWRLED();
  PWRLED( i2cCore  *mycore , char DACaddr, char Exp1Add, char Exp2Add);
  void testme();
  void setI2CPar( i2cCore  *mycore , char DACaddr, char Exp1Add, char Exp2Add);
  void setVchannel(int channel, float voltage, bool verbose);
  void initI2Cslaves(bool intRef, bool verbose);
  //void SetIntRef(bool intRef, bool verbose);
  //void SetDACValue(unsigned char channel, uint32_t value);
};
#endif
