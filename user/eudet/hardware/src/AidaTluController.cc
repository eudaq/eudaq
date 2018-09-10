#include "AidaTluController.hh"
#include "AidaTluHardware.hh"
#include "AidaTluPowerModule.hh"
#include "AidaTluDisplay.hh"
#include "AidaTluI2c.hh"
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <bitset>
#include <iomanip>
#include "eudaq/Logger.hh"

#include "uhal/uhal.hpp"

namespace tlu {
  AidaTluController::AidaTluController(const std::string & connectionFilename, const std::string & deviceName) : m_hw(0), m_DACaddr(0), m_IDaddr(0) {

    std::string myMsg= "CONFIGURING FROM " + connectionFilename + " THE DEVICE " + deviceName + "\t";
    EUDAQ_INFO(myMsg);
    if(!m_hw) {
      ConnectionManager manager ( connectionFilename );
      SetUhalLogLevel(2); //  Get rid of initial flood of messages for address map
      m_hw = new uhal::HwInterface(manager.getDevice( deviceName )); // <<
      m_i2c = new i2cCore(m_hw);
      GetFW();
      m_IPaddress= parseURI();
    }
  }

  void AidaTluController::configureHDMI(unsigned int hdmiN, unsigned int enable, bool verbose){
    int nDUTs;
    unsigned char oldStatus;
    unsigned char newStatus;
    unsigned char mask;
    unsigned char newnibble;
    nDUTs= m_nDUTs;

    if ((0 < hdmiN )&&( hdmiN < nDUTs+1 )){
      std::cout << std::boolalpha << "  Configuring HDMI " << hdmiN << ":" << std::endl;

      hdmiN = hdmiN-1;  // <<<<< CAREFUL HERE. All the rest is meant to work with [0:3] rather than [1:4}]
      unsigned int bank = (unsigned int)hdmiN / 2; // DUT0 and DUT1 are on bank 0. DUT2 and DUT3 on bank 1
      unsigned int nibble = hdmiN % 2;    // DUT0 and DUT2 are on nibble 0. DUT1 and DUT3 are on nibble 1

      if (verbose){
        std::cout << "\tBank " << bank << " Nibble " << nibble << std::endl;
      }
      // Modify the expander responsible for CONT, TRIG, SPARE and BUSY
      oldStatus= m_IOexpander1.getOutputs(bank, false);
      newnibble= (enable & 0xF) << 4*nibble;
      mask = 0xF << 4*nibble; // bits we want to change are marked with 1
      newStatus= (oldStatus & (~mask)) | (newnibble & mask);

      if (verbose){
        std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMask " << (int)mask << "\tNEW " << (int)newStatus << std::dec << std::endl;
      }
      m_IOexpander1.setOutputs(bank, newStatus, verbose);
    }
    else{
      std::cout << "enableHDMI: connector out of range [1, " << nDUTs << "]" << std::endl;
    }
  }

  void AidaTluController::DefineConst(int nDUTs, int nTrigInputs){
    m_nTrgIn= nTrigInputs;
    m_nDUTs= nDUTs;
    std::cout << "  TLU (" << m_nDUTs << " DUTs; " << m_nTrgIn << " Trigger inputs)" << std::endl;
  }

  void AidaTluController::DumpEventsBuffer() {
    std::cout<<"AidaTluController::DumpEvents......"<<std::endl;
    for(auto&& i: m_data){
      std::cout<<i<<std::endl;
    }
    std::cout<<"AidaTluController::DumpEvents end"<<std::endl;
  }

  void AidaTluController::enableClkLEMO(bool enable, bool verbose= false){
    int bank=1;
    unsigned char mask= 0x10;
    unsigned char oldStatus;
    unsigned char newStatus;

    oldStatus= m_IOexpander2.getOutputs(bank, false);
    newStatus= oldStatus & ~mask;
    std::string outstat= "disabled";
    if (enable){ //1 activates the output. 0 disables it.
      newStatus= newStatus | mask;
      outstat= "enabled";
    }
    std::cout << "  Clk LEMO " << outstat << std::endl;
    if (verbose){
      std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMask " << (int)mask << "\tNEW " << (int)newStatus << std::dec << std::endl;
    }
    m_IOexpander2.setOutputs(bank, newStatus, verbose);
  }

  void AidaTluController::enableHDMI(unsigned int hdmiN, bool enable, bool verbose= false){
    int nDUTs;
    unsigned char oldStatus;
    unsigned char newStatus;
    unsigned char mask;
    nDUTs= m_nDUTs;

    std::cout << "enableHDMI: This function is obsolete. Please use configureHDMI instead." << std::endl;
    if ((0 < hdmiN )&&( hdmiN < nDUTs+1 )){
      std::cout << std::boolalpha << "  Setting HDMI " << hdmiN << " to " << enable << std::endl;

      hdmiN = hdmiN-1;  // <<<<< CAREFUL HERE. All the rest is meant to work with [0:3] rather than [1:4}]
      unsigned int bank = (unsigned int)hdmiN / 2; // DUT0 and DUT1 are on bank 0. DUT2 and DUT3 on bank 1
      unsigned int nibble = hdmiN % 2;    // DUT0 and DUT2 are on nibble 0. DUT1 and DUT3 are on nibble 1

      if (verbose){
        std::cout << "\tBank " << bank << " Nibble " << nibble << std::endl;
      }
      oldStatus= m_IOexpander1.getOutputs(bank, false);
      mask= 0xF << 4*nibble;
      newStatus= oldStatus & (~mask);
      if (!enable){ // we want to write 0 to activate the outputs so check opposite of "enable"
        newStatus |= mask;
      }
      if (verbose){
        std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMask " << (int)mask << "\tNEW " << (int)newStatus << std::dec << std::endl;
      }
      m_IOexpander1.setOutputs(bank, newStatus, verbose);
    }
    else{
      std::cout << "enableHDMI: connector out of range [1, " << nDUTs << "]" << std::endl;
    }
  }

  uint32_t AidaTluController::GetBoardID() {
    // Return the board unique ID. The ID is generally comprised of
    // 6 characters (48 bits) but the first 3 are manufacturer specific so, in order to have just
    // 32-bit, we can just skip the first two characters.
    // D8 80 39 XX YY ZZ -->  39 XX YY ZZ

    int nwords= 6;
    int shift= 0;
    uint32_t shortID= 0;
    for(int iaddr =2; iaddr < nwords; iaddr++) {
      shift= (nwords-1) -iaddr;
      shortID= (m_BoardID[iaddr] << (8*shift) ) | shortID;
    }
    std::cout << "  BoardID (short) " << std::hex << shortID << std::endl;
    return shortID;
  }

  uint32_t AidaTluController::GetEventFifoCSR(int verbose) {
    uint32_t res;
    bool empty, alm_empty, alm_full, full, prog_full;
    res= ReadRRegister("eventBuffer.EventFifoCSR");
    empty = 0x1 & res;
    alm_empty= 0x2 & res;
    alm_full= 0x4 & res;
    full= 0x8 & res;
    prog_full= 0x4 & res;
    if (empty && (verbose > 2)){std::cout << "  TLU FIFO status:\n\tEMPTY" << std::endl;}
    if (alm_empty && (verbose > 2)){std::cout << "  TLU FIFO status:\nALMOST EMPTY (1 word in FIFO)" << std::endl;}
    if (alm_full){std::cout << "  TLU FIFO status:\n\tALMOST FULL (1 word left)" << std::endl;}
    if (full){std::cout <<   "  TLU FIFO status:\n\tFULL (8192 word)" << std::endl;}
    if (prog_full){std::cout << "  TLU FIFO status:\n\tABOVE THRESHOLD (8181/8192)" << std::endl;}
    return res;
  }

  uint32_t AidaTluController::GetEventFifoFillLevel() {
    uint32_t res;
    uint32_t fifomax= 8192;

    res= ReadRRegister("eventBuffer.EventFifoFillLevel");
    //std::cout << std::fixed << std::setw( 3 ) << std::setprecision( 2 ) << std::setfill( '0' ) << "  FIFO level " << (float)res/fifomax << "% (" << res << "/" << fifomax << ")" << std::endl;
    return res;
  };

  uint32_t AidaTluController::GetFW(){
    uint32_t res;
    res= ReadRRegister("version");
    std::stringstream ss; //Use a string stream to do all the manipulation necessary
    ss << "AIDA TLU FIRMWARE VERSION " << std::hex << std::showbase  << res << "\t";
    std::string myMsg = ss.str();
    EUDAQ_INFO(myMsg);
    return res;
  }

  uint32_t AidaTluController::GetDUTMask(){
    uint32_t res;
    res= ReadRRegister("DUTInterfaces.DUTMaskR");
    std::cout << "\tDUTMask read back as= 0x" << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetDUTMaskMode(){
    uint32_t res;
    res= ReadRRegister("DUTInterfaces.DUTInterfaceModeR");
    std::cout << "\tDUTMaskMode read back as= 0x" << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetDUTMaskModeModifier(){
    uint32_t res;
    res= ReadRRegister("DUTInterfaces.DUTInterfaceModeModifierR");
    std::cout << "\tDUTMaskModifier read back as= 0x" << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetDUTIgnoreBusy(){
    uint32_t res;
    res= ReadRRegister("DUTInterfaces.IgnoreDUTBusyR");
    std::cout << "\tDUTIgnore busy read back as= 0x" << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetDUTIgnoreShutterVeto(){
    uint32_t res;
    res= ReadRRegister("DUTInterfaces.IgnoreShutterVetoR");
    std::cout << "\tDUTIgnoreShutterVeto read back as= 0x" << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetShutterControl(){
    uint32_t res;
    res= ReadRRegister("Shutter.ControlRW");
    std::cout << "\tShutter Control read back as (dec)= " << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetShutterInternalInterval(){
    uint32_t res;
    res= ReadRRegister("Shutter.InternalShutterPeriodRW");
    std::cout << "\tInternalShutterPeriod read back as (dec)= " << res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetShutterSource(){
    uint32_t res;
    res= ReadRRegister("Shutter.ShutterSelectRW");
    std::cout << "\tShutterSelect read back as (dec)= " << res <<std::dec<< std::endl;
    return res;
  }


  uint32_t AidaTluController::GetShutterOnTime(){
    uint32_t res;
    res= ReadRRegister("Shutter.ShutterOnTimeRW");
    std::cout << "\tShutterOnTime read back as (dec)= " << res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetShutterOffTime(){
    uint32_t res;
    res= ReadRRegister("Shutter.ShutterOffTimeRW");
    std::cout << "\tShutterOffTime read back as (dec)= " << res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetShutterVetoOffTime(){
    uint32_t res;
    res= ReadRRegister("Shutter.ShutterVetoOffTimeRW");
    std::cout << "\tVetoOffTime read back as (dec)= " << res <<std::dec<< std::endl;
    return res;
  }

  uint32_t AidaTluController::GetInternalTriggerInterval(int verbose){
    uint32_t interval;
    uint32_t true_freq;

    interval= ReadRRegister("triggerLogic.InternalTriggerIntervalR");
    if (verbose > 0){
      if (interval==0){
        true_freq = 0;
      }
      else{
        true_freq= (int) floor( (float)160000000 / interval );
      }
      std::cout << "\tFrequency read back as: " << true_freq << " Hz"<< std::endl;
    }
  }

  std::string AidaTluController::parseURI(){
    std::string myURI= m_hw->uri();
    std::string delimiter;
    std::stringstream ss;
    if (myURI.find("ipbusudp") != std::string::npos) {
      //std::cout << myURI << std::endl;
      delimiter = "://";
      myURI = myURI.substr(myURI.find(delimiter)+3);
      delimiter = ":";
      myURI = myURI.substr(0, myURI.find(delimiter));
      ss << "USING IPBus WITH IP= " << myURI << "\t";
    }
    if (myURI.find("chtcp") != std::string::npos) {
      //std::cout << myURI << std::endl;
      delimiter = "et="; //end of "target="
      myURI = myURI.substr(myURI.find(delimiter)+3);
      delimiter = ":";
      myURI = myURI.substr(0, myURI.find(delimiter));
      ss << "USING ControlHub WITH IP= " << myURI << "\t";
    }
    std::string myMsg = ss.str();
    EUDAQ_INFO(myMsg);
    return myURI;
  }

  unsigned int* AidaTluController::SetBoardID(){
    m_IDaddr= m_I2C_address.EEPROM;
    unsigned char myaddr= 0xfa;
    int nwords= 6;

    std::ios::fmtflags coutflags( std::cout.flags() );
    for(int iaddr =0; iaddr < nwords; iaddr++) {
      char nibble = m_i2c->ReadI2CChar(m_IDaddr, myaddr + iaddr);
      m_BoardID[iaddr]= ((uint)nibble)&0xff;
    }

    std::stringstream ss; //Use a string stream to do all the manipulation necessary
    ss << "AIDA TLU UNIQUE ID:" ;
    for(int iaddr =0; iaddr < nwords/2; iaddr++) {
      ss << " " << std::setw(2) << std::setfill('0') << std::hex <<  m_BoardID[2*iaddr] << m_BoardID[2*iaddr+1];
    }
    ss << "\t";
    std::string myMsg = ss.str();
    EUDAQ_INFO(myMsg);

    //std::cout << " " << std::endl;
    //std::cout.flags( coutflags );
    return m_BoardID;
  }

  uint64_t AidaTluController::GetTriggerMask(){
    uint32_t maskHi, maskLo;
    maskLo= ReadRRegister("triggerLogic.TriggerPattern_lowR");
    maskHi= ReadRRegister("triggerLogic.TriggerPattern_highR");
    uint64_t triggerPattern = ((uint64_t)maskHi) << 32 | maskLo;
    std::cout << std::hex << "\tTRIGGER PATTERN (for external triggers) READ 0x" << maskHi << " --- 0x"<< maskLo << std::dec << std::endl;
    return triggerPattern;
  }

  uint32_t AidaTluController::I2C_enable(char EnclustraExpAddr)
  // This must be executed at least once after powering up the TLU or the I2C bus will not work.
  {
    std::cout << "  Enabling I2C bus" << std::endl;
    m_i2c->WriteI2CChar(EnclustraExpAddr, 0x01, 0x7F);//here
    char res= m_i2c->ReadI2CChar(EnclustraExpAddr, 0x01);//here
    std::bitset<8> resbit(res);
    if (resbit.test(7))
      {
	       std::cout << "\tWarning: enabling Enclustra I2C bus might have failed. This could prevent from talking to the I2C slaves on the TLU." << int(res) << std::endl;
         EUDAQ_WARN("The Enclustra I2C bus is not correctly enabled.");
      }else{
      std::cout << "\tSuccess." << std::endl;
    }
  }

  int AidaTluController::InitializeClkChip(const std::string & filename){
    std::vector< std::vector< unsigned int> > tmpConf;
    std::stringstream ss;
    m_zeClock.SetI2CPar(m_i2c, m_I2C_address.clockChip);
    m_zeClock.getDeviceVersion();
    //std::string filename = "/users/phpgb/workspace/myFirmware/AIDA/bitFiles/TLU_CLK_Config.txt";
    tmpConf= m_zeClock.parseClkFile(filename, false);
    if (tmpConf.size() == 0){
      return -1;
    }
    m_zeClock.writeConfiguration(tmpConf, false);

    ss << "Si5345 design Id: " << m_zeClock.checkDesignID() << "\t";
    std::string myMsg = ss.str();
    EUDAQ_INFO(myMsg);
    return 0;
  }

  void AidaTluController::InitializeDAC(bool intRef, float Vref) {
    m_zeDAC1.SetI2CPar(m_i2c, m_I2C_address.DAC1);
    m_zeDAC1.SetIntRef(intRef, true);
    m_zeDAC2.SetI2CPar(m_i2c, m_I2C_address.DAC2);
    m_zeDAC2.SetIntRef(intRef, true);
    SetDACref(Vref);
  }

  void AidaTluController::InitializeIOexp(){
    m_IOexpander1.SetI2CPar(m_i2c, m_I2C_address.expander1);
    m_IOexpander2.SetI2CPar(m_i2c, m_I2C_address.expander2);

    //EPX1 bank 0
    m_IOexpander1.setInvertReg(0, 0x00, false); //0= normal, 1= inverted
    m_IOexpander1.setIOReg(0, 0x00, false); // 0= output, 1= input
    m_IOexpander1.setOutputs(0, 0xFF, false); // If setIOReg is output, set to pin to xx
    //EPX1 bank 1
    m_IOexpander1.setInvertReg(1, 0x00, false); // 0= normal, 1= inverted
    m_IOexpander1.setIOReg(1, 0x00, false);// 0= output, 1= input
    m_IOexpander1.setOutputs(1, 0xFF, false); // If setIOReg is output, set to pin to xx

    //EPX2 bank 0
    m_IOexpander2.setInvertReg(0, 0x00, false);// 0= normal, 1= inverted
    m_IOexpander2.setIOReg(0, 0x00, false);// 0= output, 1= input
    m_IOexpander2.setOutputs(0, 0x00, false);// If setIOReg is output, set to pin to xx
    //EPX2 bank 1
    m_IOexpander2.setInvertReg(1, 0x00, false);// 0= normal, 1= inverted
    m_IOexpander2.setIOReg(1, 0x00, false);// 0= output, 1= input
    m_IOexpander2.setOutputs(1, 0xB0, false);// If setIOReg is output, set to pin to xx
    std::cout << "  I/O expanders: initialized" << std::endl;
  }

  void AidaTluController::InitializeI2C() {
    std::ios::fmtflags coutflags( std::cout.flags() );// Store cout flags to be able to restore them

    SetI2CClockPrescale(0x30);
    SetI2CControl(0x80);

    //First we need to enable the enclustra I2C expander or we will not see any I2C slave past on the TLU
    I2C_enable(m_I2C_address.core);


    std::cout << "  Scan I2C bus:" << std::endl;
    for(int myaddr = 0; myaddr < 128; myaddr++) {
      SetI2CTX((myaddr<<1)|0x0);
      SetI2CCommand(0x90); // 10010000
      while(I2CCommandIsDone()) { // xxxxxx1x   TODO:: isDone or notDone
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      bool isConnected = (((GetI2CStatus()>>7)&0x1) == 0);  // 0xxxxxxx connected
      if (isConnected){
        if (myaddr== m_I2C_address.core){
          //std::cout << "\tFOUND I2C slave CORE" << std::endl;
        }
        else if (myaddr== m_I2C_address.clockChip){
          std::cout << "\t0x" << std::setw(2) << std::setfill('0') << std::hex << myaddr << " : CLOCK CHIP found."<< std::endl;
        }
        else if (myaddr== m_I2C_address.DAC1){
          std::cout << "\t0x" << std::hex << myaddr << " : DAC1 found."<< std::endl;
        }
        else if (myaddr== m_I2C_address.DAC2){
          std::cout << "\t0x" << std::hex << myaddr << " : DAC2 found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.EEPROM){
          m_IDaddr= myaddr;
          std::cout << "\t0x" << std::hex << myaddr << " : EEPROM found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.expander1){
          std::cout << "\t0x" << std::hex << myaddr << " : EXPANDER1 found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.expander2){
          std::cout << "\t0x" << std::hex << myaddr << " : EXPANDER2 found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.ledxp1addr){
          std::cout << "\t0x" << std::hex << myaddr << " : POWER MODULE EXPANDER1 found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.ledxp2addr){
          std::cout << "\t0x" << std::hex << myaddr << " : POWER MODULE EXPANDER2 found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.pwraddr){
          std::cout << "\t0x" << std::hex << myaddr << " : POWER MODULE DAC found."<< std::endl;
        }
        else if (myaddr==m_I2C_address.pwrId){
          std::cout << "\t0x" << std::hex << myaddr << " : POWER MODULE EEPROM found."<< std::endl;
          m_powerModuleType= myaddr;
        }
        else if (myaddr==m_I2C_address.lcdDisp){
          std::cout << "\t0x" << std::hex << myaddr << " : LCD DISPLAY found."<< std::endl;
          m_hasDisplay= true;
        }
        else{
          std::cout << "\t0x" << std::setw(2) << std::setfill('0') << std::hex << myaddr << " : UNKNOWN DEVICE. Not on AIDA TLU address list." << std::endl;
        }
      }
      SetI2CTX(0x0);
      SetI2CCommand(0x50); // 01010000
      while(I2CCommandIsDone()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

    if(m_IDaddr){
      SetBoardID();
    }

    pwrled_Initialize(0, m_powerModuleType);
    std::cout.flags( coutflags ); // Restore cout flags

    if (m_hasDisplay){
      EUDAQ_INFO("AIDA TLU: LCD display detected. This is a 19-inch rack unit.\t");
      m_lcddisp.setParameters(m_i2c, m_I2C_address.lcdDisp, 2, 16);
      std::cout << "  AIDA_TLU LCD: Initialising" << std::endl;

      m_lcddisp.clear();
      m_lcddisp.writeString("Please wait...");
      m_lcddisp.pulseLCD(1);
      m_lcddisp.writeAll("AIDA TLU", m_IPaddress);
    }
  }

  void AidaTluController::pwrled_Initialize(int verbose, unsigned int type) {
    std::cout << "  TLU_POWERMODULE: Initialising" << std::endl;
    m_pwrled.setI2CPar( m_i2c , m_I2C_address.pwraddr, m_I2C_address.ledxp1addr, m_I2C_address.ledxp2addr, type);
    m_pwrled.initI2Cslaves(false, verbose);
    //int indicator= 1;
    //std::array<int, 3>RGB{ {1, 0, 1} };
    //m_pwrled.setIndicatorRGB( indicator, RGB, verbose);
    m_pwrled.led_allOff();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_pwrled.led_allWhite();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_pwrled.led_allOff();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_pwrled.testLED();
  }

  void AidaTluController::pwrled_setVoltages(float v1, float v2, float v3, float v4, int verbose) {
    // Note that ordering is not 1:1. Mapping is done in here.
    std::cout << "  TLU_POWERMODULE: Setting voltages" << std::endl;
    if (verbose > 0){
      std::cout << "\tV PMT1=" << v3 << "V" << std::endl;
      std::cout << "\tV PMT2=" << v2 << "V" << std::endl;
      std::cout << "\tV PMT3=" << v4 << "V" << std::endl;
      std::cout << "\tV PMT4=" << v1 << "V" << std::endl;
    }
    m_pwrled.setVchannel(0, v3, verbose);
    m_pwrled.setVchannel(1, v2, verbose);
    m_pwrled.setVchannel(2, v4, verbose);
    m_pwrled.setVchannel(3, v1, verbose);
  }

  unsigned int AidaTluController::PackBits(std::vector< unsigned int>  rawValues){
    //Pack 6 number using only 5-bits for each.
    int nChannels= m_nTrgIn;
    unsigned int packedbits= 0;
    int tmpint= 0;
    if (nChannels== rawValues.size()){
      for (int iCh=0; iCh < nChannels; iCh++){
        tmpint= ((int)rawValues.at(iCh))<< iCh*5;
        packedbits = packedbits | tmpint;
      }
      //std::cout << "PACKED "<< packedbits << std::endl;
    }
    else{
      std::cout << "PackBits - ERROR: wrong number of elements in vector." << std::endl;
    }
    return packedbits;
  }

  void AidaTluController::PulseT0(){
    SetWRegister("Shutter.PulseT0", 0x1);
    std::cout << "  PULSE T0: done"  << std::endl;
  }

  fmctludata* AidaTluController::PopFrontEvent(){
    fmctludata *e = m_data.front();
    m_data.pop_front();
    return e;
  }

  uint32_t AidaTluController::ReadRRegister(const std::string & name) {
    try {
      ValWord< uint32_t > test = m_hw->getNode(name).read();
      m_hw->dispatch();
      if(test.valid()) {
	return test.value();
      } else {
	std::cout << "Error reading " << name << std::endl;
	return 0;
      }
    } catch (...) {
      return 0;
    }
  }

  void AidaTluController::ReceiveEvents(int verbose){
    //bool verbose= 0;
    uint32_t nevent = GetEventFifoFillLevel()/6;
    uint32_t fifoStatus= GetEventFifoCSR(verbose);
    if ((fifoStatus & 0x18)){
      std::cout << "WARNING! fmctlu hardware FIFO is full (CSR)" << std::endl;
    }
    if (nevent*6 == 0x3FEA) std::cout << "WARNING! fmctlu hardware FIFO is full" << std::endl; //0x7D00 ?
    // if(0){ // no read
    if(nevent){
      ValVector< uint32_t > fifoContent = m_hw->getNode("eventBuffer.EventFifoData").readBlock(nevent*6);
      m_hw->dispatch();
      if(fifoContent.valid()) {
        if (verbose > 0){
          std::cout<< "TLU events required: "<<nevent<<" events received: " << fifoContent.size()/6<<std::endl;
        }
        if(fifoContent.size()%6 !=0){
          std::cout<<"receive error"<<std::endl;
        }
        for ( std::vector<uint32_t>::const_iterator i ( fifoContent.begin() ); i!=fifoContent.end(); i+=6 ) { //0123
          m_data.push_back(new fmctludata(*i, *(i+1), *(i+2), *(i+3), *(i+4), *(i+5)));
          if (verbose > 1){
            std::cout<< *(m_data.back());
          }
        }
      }
    }
  }

  void AidaTluController::ResetEventsBuffer(){
    for(auto &&i: m_data){
      delete i;
    }
    m_data.clear();
  }

  void AidaTluController::SetDutClkSrc(unsigned int hdmiN, unsigned int source, bool verbose= false){
    int nDUTs;
    unsigned char oldStatus;
    unsigned char newStatus;
    unsigned char newnibble;
    unsigned char mask, maskLow, maskHigh;
    int bank= 0;

    nDUTs= m_nDUTs;
    if ((hdmiN < 1 ) || ( hdmiN > nDUTs )){
      std::cout << "\tSetDutClkSrc - ERROR: HDMI must be in range [1, 4]" << std::endl;
      return;
    }
    if ((source < 0 ) || ( source > 2 )){
      std::cout << "\tSetDutClkSrc - ERROR: Clock source can only be 0 (disabled), 1 (Si5345) or 2 (FPGA)" << std::endl;
      return;
    }
    std::cout  << "  Setting HDMI " << hdmiN << " clock source:" << std::endl;
    hdmiN= hdmiN-1;
    maskLow= 1 << (1* hdmiN); //CLK FROM FPGA
    maskHigh= 1<< (1* hdmiN +4); //CLK FROM Si5345
    mask= maskLow | maskHigh;
    oldStatus= m_IOexpander2.getOutputs(bank, false);
    //newStatus= oldStatus & ~mask;
    switch(source){
      case 0 : {
        //newStatus = newStatus | mask;
        newStatus= (oldStatus & ~mask)  ;
        std::cout << "\tdisabled" << std::endl;
        break;
      }
      case 1 : {
        newStatus = (oldStatus | maskHigh) & ~maskLow;
        //newStatus= (oldStatus & ~mask) | (0xF0 & mask);
        std::cout << "\tSi5435" << std::endl;
        break;
      }
      case 2 : {
        //newStatus= newStatus | maskLow;
        newStatus = (oldStatus | maskLow) & ~maskHigh;
        std::cout << "\tFPGA" << std::endl;
        break;
      }
      default: {
        newStatus= oldStatus;
        std::cout << "\tNo valid clock source selected" << std::endl;
        break;
      }
    }
    if(verbose){
      std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMASK " <<  (int)mask << "\tMASK_L " << (int)maskLow << "\tMASK_H " << (int)maskHigh << "\tNEW " << (int)newStatus << std::dec << std::endl;
    }
    m_IOexpander2.setOutputs(bank, newStatus, verbose);
  }

  void AidaTluController::SetDUTMask(uint32_t value, bool verbose){
    SetWRegister("DUTInterfaces.DUTMaskW",value);
    if (verbose){
      std::cout << "  Writing DUTMask: 0x" << std::hex << (uint32_t)value  << std::endl;
      GetDUTMask();
    }
  };

  void AidaTluController::SetDUTMaskMode(uint32_t value, bool verbose){
    SetWRegister("DUTInterfaces.DUTInterfaceModeW",value);
    if (verbose){
      std::cout << "  Writing DUTInterfaceMode: 0x" << std::hex << value  <<std::dec << std::endl;
      GetDUTMaskMode();
    }
  };

  void AidaTluController::SetDUTMaskModeModifier(uint32_t value, bool verbose){
    SetWRegister("DUTInterfaces.DUTInterfaceModeModifierW",value);
    if (verbose){
      std::cout << "  Writing DUTModeModifier: 0x" << std::hex << value << std::endl;
      GetDUTMaskModeModifier();
    }
  };

  void AidaTluController::SetDUTIgnoreBusy(uint32_t value, bool verbose){
    SetWRegister("DUTInterfaces.IgnoreDUTBusyW",value);
    if (verbose){
      std::cout << "  Writing DUT Ignore Busy: 0x" << std::hex << value << std::endl;
      GetDUTIgnoreBusy();
    }
  };

  void AidaTluController::SetShutterControl(uint32_t value, bool verbose){
    SetWRegister("Shutter.ControlRW",value);
    if (verbose){
      std::cout << "  Writing ShutterControl (dec): " << value << std::endl;
      GetShutterControl();
    }
  };

  void AidaTluController::SetShutterInternalInterval(uint32_t value, bool verbose){
    SetWRegister("Shutter.InternalShutterPeriodRW",value);
    if (verbose){
      std::cout << "  Writing InternalShutterPeriod (dec): " << value << std::endl;
      GetShutterInternalInterval();
    }
  };

  void AidaTluController::SetShutterSource(uint32_t value, bool verbose){
    SetWRegister("Shutter.ShutterSelectRW",value);
    if (verbose){
      std::cout << "  Writing ShutterSource (dec): " << value << std::endl;
      GetShutterControl();
    }
  };

   void AidaTluController::SetShutterOnTime(uint32_t value, bool verbose){
    SetWRegister("Shutter.ShutterOnTimeRW",value);
    if (verbose){
      std::cout << "  Writing ShutterOnTime (dec): " << value << std::endl;
      GetShutterControl();
    }
  };

   void AidaTluController::SetShutterOffTime(uint32_t value, bool verbose){
    SetWRegister("Shutter.ShutterOffTimeRW",value);
    if (verbose){
      std::cout << "  Writing ShutterOffTime (dec): " << value << std::endl;
      GetShutterOffTime();
    }
  };

   void AidaTluController::SetShutterVetoOffTime(uint32_t value, bool verbose){
    SetWRegister("Shutter.ShutterVetoOffTimeRW",value);
    if (verbose){
      std::cout << "  Writing ShutterVetoOffTime (dec): " << value << std::endl;
      GetShutterVetoOffTime();
    }
  };

  void AidaTluController::SetDUTIgnoreShutterVeto(uint32_t value, bool verbose){
    SetWRegister("DUTInterfaces.IgnoreShutterVetoW",value);
    if (verbose){
      std::cout << "  Writing DUT Ignore Veto: 0x" << std::hex << value << std::endl;
      GetDUTIgnoreShutterVeto();
    }
  };

  void AidaTluController::SetInternalTriggerFrequency(uint32_t user_freq, int verbose){
    uint32_t max_freq= 160000000;
    uint32_t interval;
    uint32_t actual_interval;
    if (user_freq > max_freq){
      std::cout << "  SetInternalTriggerFrequency: Max frequency allowed is "<< max_freq << " Hz. Coerced to this value." << std::endl;
      user_freq= max_freq;
    }
    if (user_freq==0){
      interval = user_freq;
    }
    else{
      interval = (int) floor( (float)160000000 / user_freq );
    }
    SetInternalTriggerInterval(interval);
    std::cout << "  Required internal trigger frequency: " << user_freq << " Hz" << std::endl;
    std::cout << "\tSetting internal interval to:" << interval << std::endl;
    actual_interval= GetInternalTriggerInterval(1);
  }

  void AidaTluController::SetPulseStretchPack(std::vector< unsigned int>  valuesVec){
    SetPulseStretch( (int)PackBits(valuesVec) );
  }

  void AidaTluController::SetPulseDelayPack(std::vector< unsigned int>  valuesVec){
    SetPulseDelay( (int)PackBits(valuesVec) );
  }

  void AidaTluController::SetDACref(float vref){
    m_vref= vref;
    std::cout << "  DAC will use Vref= " << m_vref << " V" << std::endl;
  }

  void AidaTluController::SetThresholdValue(unsigned char channel, float thresholdVoltage ) {
    //Channel can either be [0, 5] or 7 (all channels).
    int nChannels= m_nTrgIn; //We should read this from conf file, ideally.
    bool intRef= false; //We should read this from conf file, ideally.
    float vref;

    if ( std::abs(thresholdVoltage) > m_vref ){
      thresholdVoltage= m_vref*thresholdVoltage/std::abs(thresholdVoltage);
      std::cout<<"\tWarning: Threshold voltage is outside range [-1.3 , +1.3] V. Coerced to "<< thresholdVoltage << " V"<<std::endl;
    }

    float vdac = ( thresholdVoltage + m_vref ) / 2;
    float dacCode =  0xFFFF * vdac / m_vref;

    if( (channel != 7) && ((channel < 0) || (channel > (nChannels-1)))  ){
      std::cout<<"\tError: DAC illegal DAC channel. Use 7 for all channels or 0 <= channel <= "<< nChannels-1 << std::endl;
      return;
    }

    if (channel==7){
      std::cout << "  Setting threshold for all channels to " << thresholdVoltage << " Volts" << std::endl;
      m_zeDAC1.SetDACValue(channel , int(dacCode) );
      m_zeDAC2.SetDACValue(channel , int(dacCode) );
      return;
    }
    if (channel <2){
      std::cout << "  Setting threshold for channel " << (unsigned int)channel << " to " << thresholdVoltage << " Volts" << std::endl;
      m_zeDAC1.SetDACValue( 1-channel , int(dacCode) ); //The ADC channels are connected in reverse order
    }
    else{
      std::cout << "  Setting threshold for channel " << (unsigned int)channel << " to " << thresholdVoltage << " Volts" << std::endl;
      m_zeDAC2.SetDACValue( 3-(channel-2) , int(dacCode) );
    }

  }

  void AidaTluController::SetTriggerMask(uint64_t value){
    uint32_t maskHi, maskLo;
    maskHi = (uint32_t)(value>>32);
    maskLo = (uint32_t)value;
    std::cout << std::hex << "  TRIGGER PATTERN (for external triggers) SET TO 0x" << maskHi << " --- 0x"<< maskLo << " (Two 32-bit words)" << std::dec << std::endl;
    SetWRegister("triggerLogic.TriggerPattern_lowW",  maskLo);
    SetWRegister("triggerLogic.TriggerPattern_highW", maskHi);
  }

  void AidaTluController::SetTriggerMask(uint32_t maskHi, uint32_t maskLo){
    std::cout << std::hex << "  TRIGGER PATTERN (for external triggers) SET TO 0x" << maskHi << " --- 0x"<< maskLo << " (Two 32-bit words)" << std::dec << std::endl;
    SetWRegister("triggerLogic.TriggerPattern_lowW",  maskLo);
    SetWRegister("triggerLogic.TriggerPattern_highW", maskHi);
  }

  void AidaTluController::SetTriggerVeto(int value){
    uint32_t vetoStatus;
    SetWRegister("triggerLogic.TriggerVetoW",value);
    vetoStatus= GetTriggerVeto();
    std::cout << "  TRIGGER VETO SET TO: " << vetoStatus << std::endl;
  }

  void AidaTluController::SetWRegister(const std::string & name, int value){
    try {
      m_hw->getNode(name).write(static_cast< uint32_t >(value));
      m_hw->dispatch();
    } catch (...) {
      return;
    }
  }

  void AidaTluController::SetUhalLogLevel(uchar_t l){
    switch(l){
    case 0:
      uhal::disableLogging();
      break;
    case 1:
      uhal::setLogLevelTo(uhal::Fatal());
      break;
    case 2:
      uhal::setLogLevelTo(uhal::Error());
      break;
    case 3:
      uhal::setLogLevelTo(uhal::Warning());
      break;
    case 4:
      uhal::setLogLevelTo(uhal::Notice());
      break;
    case 5:
      uhal::setLogLevelTo(uhal::Info());
      break;
    case 6:
      uhal::setLogLevelTo(uhal::Debug());
      break;
    default:
      uhal::setLogLevelTo(uhal::Debug());
    }
  }



  std::ostream &operator<<(std::ostream &s, fmctludata &d) {
    s << "__________________________________________________________________________" << std::endl
      << "EVENT NUMBER: " << d.eventnumber << " \t TYPE: " << int(d.eventtype) <<" \t TIMESTAMP COARSE: 0x" <<std::hex<< d.timestamp << std::dec<<std::endl
      << " TRIG. INPUT 0: " << int(d.input0) << " \t TRIG. INPUT 1: " << int(d.input1) << " \t TRIG. INPUT 2: " << int(d.input2) << std::endl
      << " TRIG. INPUT 3: " << int(d.input3) << " \t TRIG. INPUT 4: " << int(d.input4) << " \t TRIG. INPUT 5: " << int(d.input5) <<std::endl
      << " TS FINE 0: " << int(d.sc0) << " \t TS FINE 1: "  << int(d.sc1) << " \t TS FINE 2: "  << int(d.sc2)  << std::endl
      << " TS FINE 3: " << int(d.sc3) << " \t TS FINE 4: "  << int(d.sc4) << " \t TS FINE 5: "  << int(d.sc5)  <<std::endl;
    return s;
  }


}
