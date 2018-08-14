#ifndef H_AIDATLUHARDWARE_HH
#define H_AIDATLUHARDWARE_HH

#include <string>
#include <iostream>
#include <vector>
#include "FmctluI2c.hh"

/*
  This file contains classes for chips used on the new TLU design.
  Paolo.Baesso@bristol.ac.uk - 2017
*/
//namespace tluHw{
//////////////////////////////////////////////////////////////////////
class AD5665R{
private:
  char m_i2cAddr;
  int m_DACn;
  i2cCore * m_DACcore;
  //i2cCore myi2c;
public:
  //AD5665R::AD5665R()
  AD5665R();
  void testme();
  void SetI2CPar( i2cCore  *mycore , char addr);
  void SetIntRef(bool intRef, bool verbose);
  void SetDACValue(unsigned char channel, uint32_t value);
};
//////////////////////////////////////////////////////////////////////
class PCA9539PW{
private:
  char m_i2cAddr;
  i2cCore * m_IOXcore;

  void writeReg(unsigned int memAddr, unsigned char regContent, bool verbose);
  char readReg(unsigned int memAddr, bool verbose);
public:
  PCA9539PW();
  void SetI2CPar( i2cCore  *mycore , char addr);
  void setInvertReg(unsigned int memAddr, unsigned char polarity, bool verbose);
  char getInvertReg(unsigned int memAddr, bool verbose);
  void setIOReg(unsigned int memAddr, unsigned char direction, bool verbose);
  char getIOReg(unsigned int memAddr, bool verbose);
  char getInputs(unsigned int memAddr, bool verbose);
  void setOutputs(unsigned int memAddr, unsigned char direction, bool verbose);
  char getOutputs(unsigned int memAddr, bool verbose);
};
//////////////////////////////////////////////////////////////////////
class Si5345{
private:
  char m_i2cAddr;
  i2cCore * m_Clkcore;

  void setPage(unsigned int page, bool verbose);
  char getPage(bool verbose);

public:
  Si5345();
  void SetI2CPar( i2cCore  *mycore , char addr);
  unsigned int getDeviceVersion();
  char readRegister(unsigned int myaddr, bool verbose);
  void writeRegister(unsigned int myaddr, unsigned char data, bool verbose);
  void checkDesignID();
  std::vector< std::vector< unsigned int> > parseClkFile(const std::string & filename, bool verbose);
  void writeConfiguration(std::vector< std::vector< unsigned int> > regSetting, bool verbose);
};

//}
#endif
