#include "FmctluController.hh"
#include "FmctluHardware.hh"
#include "FmctluI2c.hh"
#include <iomanip>
#include <thread>
#include <chrono>
#include <string>
#include <bitset>
#include <iomanip>

#include "uhal/uhal.hpp"

namespace tlu {
  FmctluController::FmctluController(const std::string & connectionFilename, const std::string & deviceName) : m_hw(0), m_DACaddr(0), m_IDaddr(0) {

    //m_i2c = new i2cCore(connectionFilename, deviceName);
    std::cout << "Configuring from " << connectionFilename << " the device " << deviceName << std::endl;
    if(!m_hw) {
      ConnectionManager manager ( connectionFilename );
      m_hw = new uhal::HwInterface(manager.getDevice( deviceName ));
      m_i2c = new i2cCore(m_hw);
      GetFW();

    }
  }

  void FmctluController::DumpEventsBuffer() {
    std::cout<<"FmctluController::DumpEvents......"<<std::endl;
    for(auto&& i: m_data){
      std::cout<<i<<std::endl;
    }
    std::cout<<"FmctluController::DumpEvents end"<<std::endl;
  }

  void FmctluController::enableClkLEMO(bool enable, bool verbose= false){
    int bank=1;
    unsigned char mask= 0x10;
    unsigned char oldStatus;
    unsigned char newStatus;

    oldStatus= m_IOexpander2.getIOReg(bank, false);
    newStatus= oldStatus & ~mask;
    std::string outstat= "enabled";
    if (!enable){ //0 activates the output. 1 disables it.
      newStatus= newStatus | mask;
      outstat= "disabled";
    }
    std::cout << "  Clk LEMO " << outstat << std::endl;
    if (verbose){
      std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMask " << (int)mask << "\tNEW " << (int)newStatus << std::dec << std::endl;
    }
    m_IOexpander2.setIOReg(bank, newStatus, verbose);
  }

  void FmctluController::enableHDMI(unsigned int hdmiN, bool enable, bool verbose= false){
    int nDUTs= 4;
    unsigned char oldStatus;
    unsigned char newStatus;
    unsigned char mask;

    if ((0 < hdmiN )&&( hdmiN < nDUTs+1 )){
      std::cout << std::boolalpha << "  Setting HDMI " << hdmiN << " to " << enable << std::endl;

      hdmiN = hdmiN-1;  // <<<<< CAREFUL HERE. All the rest is meant to work with [0:3] rather than [1:4}]
      unsigned int bank = (unsigned int)hdmiN / 2; // DUT0 and DUT1 are on bank 0. DUT2 and DUT3 on bank 1
      unsigned int nibble = hdmiN % 2;    // DUT0 and DUT2 are on nibble 0. DUT1 and DUT3 are on nibble 1

      if (verbose){
        std::cout << "\tBank " << bank << " Nibble " << nibble << std::endl;
      }
      oldStatus= m_IOexpander1.getIOReg(bank, false);
      mask= 0xF << 4*nibble;
      newStatus= oldStatus & (~mask);
      if (!enable){ // we want to write 0 to activate the outputs so check opposite of "enable"
        newStatus |= mask;
      }
      if (verbose){
        std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tMask " << (int)mask << "\tNEW " << (int)newStatus << std::dec << std::endl;
      }
      m_IOexpander1.setIOReg(bank, newStatus, verbose);
    }
    else{
      std::cout << "enableHDMI: connector out of range [1, " << nDUTs << "]" << std::endl;
    }
  }

  uint32_t FmctluController::GetEventFifoCSR() {
    uint32_t res;
    bool empty, alm_empty, alm_full, full, prog_full;
    res= ReadRRegister("eventBuffer.EventFifoCSR");
    empty = 0x1 & res;
    alm_empty= 0x2 & res;
    alm_full= 0x4 & res;
    full= 0x8 & res;
    prog_full= 0x4 & res;
    std::cout << "  FIFO status:" << std::endl;
    if (empty){std::cout << "\tEMPTY" << std::endl;}
    if (alm_empty){std::cout << "\tALMOST EMPTY (1 word in FIFO)" << std::endl;}
    if (alm_full){std::cout << "\tALMOST FULL (1 word left)" << std::endl;}
    if (full){std::cout << "\tFULL (8192 word)" << std::endl;}
    if (prog_full){std::cout << "\tABOVE THRESHOLD (8181/8192)" << std::endl;}
    return res;
  }

  uint32_t FmctluController::GetEventFifoFillLevel() {
    uint32_t res;
    uint32_t fifomax= 8192;

    res= ReadRRegister("eventBuffer.EventFifoFillLevel");
    //std::cout << std::fixed << std::setw( 3 ) << std::setprecision( 2 ) << std::setfill( '0' ) << "  FIFO level " << (float)res/fifomax << "% (" << res << "/" << fifomax << ")" << std::endl;
    return res;
  };

  uint32_t FmctluController::GetFW(){
    uint32_t res;
    res= ReadRRegister("version");
    std::cout << "TLU FIRMWARE VERSION= " << std::hex<< res <<std::dec<< std::endl;
    return res;
  }

  uint64_t FmctluController::getSN(){
    m_IDaddr= m_I2C_address.EEPROM;
    for(unsigned char myaddr = 0xfa; myaddr > 0x0; myaddr++) {
      char nibble = m_i2c->ReadI2CChar(m_IDaddr, myaddr);//here
      m_BoardID = ((((uint64_t)nibble)&0xff)<<((0xff-myaddr)*8))|m_BoardID;
    }
    std::cout << "  TLU Unique ID : " << std::setw(12) << std::setfill('0') << std::hex << m_BoardID << std::endl;
    return m_BoardID;
  }

  uint64_t FmctluController::GetTriggerMask(){
    uint32_t maskHi, maskLo;
    maskLo= ReadRRegister("triggerLogic.TriggerPattern_lowR");
    maskHi= ReadRRegister("triggerLogic.TriggerPattern_highR");
    uint64_t triggerPattern = ((uint64_t)maskHi) << 32 | maskLo;
    std::cout << std::hex << "\tTRIGGER PATTERN (for external triggers) READ 0x" << maskHi << " --- 0x"<< maskLo << std::dec << std::endl;
    return triggerPattern;
  }

  uint32_t FmctluController::I2C_enable(char EnclustraExpAddr)
  // This must be executed at least once after powering up the TLU or the I2C bus will not work.
  {
    std::cout << "  Enabling I2C bus" << std::endl;
    m_i2c->WriteI2CChar(EnclustraExpAddr, 0x01, 0x7F);//here
    char res= m_i2c->ReadI2CChar(EnclustraExpAddr, 0x01);//here
    std::bitset<8> resbit(res);
    if (resbit.test(7))
      {
	std::cout << "\tWarning: enabling Enclustra I2C bus might have failed. This could prevent from talking to the I2C slaves on the TLU." << int(res) << std::endl;
      }else{
      std::cout << "\tSuccess." << std::endl;
    }
  }

  void FmctluController::InitializeClkChip(const std::string & filename){
    std::vector< std::vector< unsigned int> > tmpConf;
    m_zeClock.SetI2CPar(m_i2c, m_I2C_address.clockChip);
    m_zeClock.getDeviceVersion();
    //std::string filename = "/users/phpgb/workspace/myFirmware/AIDA/bitFiles/TLU_CLK_Config.txt";
    tmpConf= m_zeClock.parseClkFile(filename, false);
    m_zeClock.writeConfiguration(tmpConf, false);
    m_zeClock.checkDesignID();
  }

  void FmctluController::InitializeDAC() {
    m_zeDAC1.SetI2CPar(m_i2c, m_I2C_address.DAC1);
    m_zeDAC1.SetIntRef(false, true);
    m_zeDAC2.SetI2CPar(m_i2c, m_I2C_address.DAC2);
    m_zeDAC2.SetIntRef(false, true);
    //std::cout << "  I/O expander: initialized" << std::endl;
  }

  void FmctluController::InitializeIOexp(){
    m_IOexpander1.SetI2CPar(m_i2c, m_I2C_address.expander1);
    m_IOexpander2.SetI2CPar(m_i2c, m_I2C_address.expander2);

    //EPX1 bank 0
    m_IOexpander1.setInvertReg(0, 0x00, false); //0= normal, 1= inverted
    m_IOexpander1.setIOReg(0, 0xFF, false); // 0= output, 1= input
    m_IOexpander1.setOutputs(0, 0xFF, false); // If output, set to 1
    //EPX1 bank 1
    m_IOexpander1.setInvertReg(1, 0x00, false); // 0= normal, 1= inverted
    m_IOexpander1.setIOReg(1, 0xFF, false);// 0= output, 1= input
    m_IOexpander1.setOutputs(1, 0xFF, false); // If output, set to 1

    //EPX2 bank 0
    m_IOexpander2.setInvertReg(0, 0x00, false);// 0= normal, 1= inverted
    m_IOexpander2.setIOReg(0, 0xFF, false);// 0= output, 1= input
    m_IOexpander2.setOutputs(0, 0xFF, false);// If output, set to 1
    //EPX2 bank 1
    m_IOexpander2.setInvertReg(1, 0x00, false);// 0= normal, 1= inverted
    m_IOexpander2.setIOReg(1, 0x5F, false);// 0= output, 1= input
    m_IOexpander2.setOutputs(1, 0xFF, false);// If output, set to 1
    std::cout << "  I/O expanders: initialized" << std::endl;
  }

  void FmctluController::InitializeI2C() {
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
          std::cout << "\tFOUND I2C slave CLOCK" << std::endl;
        }
        else if (myaddr== m_I2C_address.DAC1){
          std::cout << "\tFOUND I2C slave DAC1" << std::endl;
        }
        else if (myaddr== m_I2C_address.DAC2){
          std::cout << "\tFOUND I2C slave DAC2" << std::endl;
        }
        else if (myaddr==m_I2C_address.EEPROM){
          m_IDaddr= myaddr;
          std::cout << "\tFOUND I2C slave EEPROM" << std::endl;
        }
        else if (myaddr==m_I2C_address.expander1){
          std::cout << "\tFOUND I2C slave EXPANDER1" << std::endl;
        }
        else if (myaddr==m_I2C_address.expander2){
          std::cout << "\tFOUND I2C slave EXPANDER2" << std::endl;
        }
        else{
          std::cout << "\tI2C slave at address 0x" << std::hex << myaddr << " replied but is not on TLU address list. A mistery!" << std::endl;
        }
      }
      SetI2CTX(0x0);
      SetI2CCommand(0x50); // 01010000
      while(I2CCommandIsDone()) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }

    if(m_IDaddr){
      getSN();
    }
    std::cout.flags( coutflags ); // Restore cout flags
  }

  unsigned int FmctluController::PackBits(std::vector< unsigned int>  rawValues){
    //Pack 6 number using only 5-bits for each.
    int nChannels= 6;
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

  fmctludata* FmctluController::PopFrontEvent(){
    fmctludata *e = m_data.front();
    m_data.pop_front();
    return e;
  }

  uint32_t FmctluController::ReadRRegister(const std::string & name) {
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

  void FmctluController::ReceiveEvents(){
    // std::cout<<"FmctluController::ReceiveEvents"<<std::endl;
    uint32_t nevent = GetEventFifoFillLevel()/4;
    // std::cout<< "fifo "<<GetEventFifoFillLevel()<<std::endl;
    if (nevent*4 == 0x7D00) std::cout << "WARNING! fmctlu hardware FIFO is full" << std::endl;
    // if(0){ // no read
    if(nevent){
      ValVector< uint32_t > fifoContent = m_hw->getNode("eventBuffer.EventFifoData").readBlock(nevent*4);
      m_hw->dispatch();
      if(fifoContent.valid()) {
	std::cout<< "require events: "<<nevent<<" received events "<<fifoContent.size()/4<<std::endl;
	if(fifoContent.size()%4 !=0){
	  std::cout<<"receive error"<<std::endl;
	}
	for ( std::vector<uint32_t>::const_iterator i ( fifoContent.begin() ); i!=fifoContent.end(); i+=4 ) { //0123
	  m_data.push_back(new fmctludata(*i, *(i+1), *(i+2), *(i+3)));
	  std::cout<< *(m_data.back());
	}
      }
    }
  }

  void FmctluController::ResetEventsBuffer(){
    for(auto &&i: m_data){
      delete i;
    }
    m_data.clear();
  }

  void FmctluController::SetDutClkSrc(unsigned int hdmiN, unsigned int source, bool verbose= false){
    int nDUTs= 4;
    unsigned char oldStatus;
    unsigned char newStatus;
    unsigned char mask, maskLow, maskHigh;
    int bank= 0;

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
    oldStatus= m_IOexpander2.getIOReg(bank, false);
    newStatus= oldStatus & ~mask; //
    switch(source){
    case 0 : {
      newStatus = newStatus | mask;
      std::cout << "\tdisabled" << std::endl;
      break;
    }
    case 1 : {
      newStatus = newStatus | maskLow;
      std::cout << "\tSi5435" << std::endl;
      break;
    }
    case 2 : {
      newStatus= newStatus | maskHigh;
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
      std::cout << std::hex << "\tOLD " << (int)oldStatus << "\tNEW " << (int)newStatus << std::dec << std::endl;
    }
    m_IOexpander2.setIOReg(bank, newStatus, verbose);
  }

  void FmctluController::SetPulseStretchPack(std::vector< unsigned int>  valuesVec){
    SetPulseStretch( (int)PackBits(valuesVec) );
  }

  void FmctluController::SetPulseDelayPack(std::vector< unsigned int>  valuesVec){
    SetPulseDelay( (int)PackBits(valuesVec) );
  }


  void FmctluController::SetThresholdValue(unsigned char channel, float thresholdVoltage ) {
    //Channel can either be [0, 5] or 7 (all channels).
    int nChannels= 6; //We should read this from conf file, ideally.
    bool intRef= false; //We should read this from conf file, ideally.
    float vref;
    if (intRef){
      vref = 2.500; // Internal reference
    }
    else{
      vref = 1.300; // Reference voltage is 1.3V on newer TLU
    }
    if ( std::abs(thresholdVoltage) > vref ){
      thresholdVoltage= vref*thresholdVoltage/std::abs(thresholdVoltage);
      std::cout<<"\tWarning: Threshold voltage is outside range [-1.3 , +1.3] V. Coerced to "<< thresholdVoltage << " V"<<std::endl;
    }

    float vdac = ( thresholdVoltage + vref ) / 2;
    float dacCode =  0xFFFF * vdac / vref;

    if( (channel != 7) && ((channel < 0) || (channel > (nChannels-1)))  ){
      std::cout<<"\tError: DAC illegal DAC channel. Use 7 for all channels or 0 <= channel <= "<< nChannels-1 << std::endl;
      return;
    }

    if (channel==7){
      m_zeDAC1.SetDACValue(channel , int(dacCode) );
      m_zeDAC2.SetDACValue(channel , int(dacCode) );
      std::cout << "  Setting threshold for all channels to " << thresholdVoltage << " Volts" << std::endl;
      return;
    }
    if (channel <4){
      m_zeDAC2.SetDACValue(channel , int(dacCode) );
      std::cout << "  Setting threshold for channel " << (unsigned int)channel << " to " << thresholdVoltage << " Volts" << std::endl;
    }
    else{
      m_zeDAC1.SetDACValue(channel-4 , int(dacCode) );
      std::cout << "  Setting threshold for channel " << (unsigned int)channel << " to " << thresholdVoltage << " Volts" << std::endl;
    }

  }

  void FmctluController::SetTriggerMask(uint64_t value){
    uint32_t maskHi, maskLo;
    maskHi = (uint32_t)(value>>32);
    maskLo = (uint32_t)value;
    std::cout << std::hex << "  TRIGGER PATTERN (for external triggers) SET TO 0x" << maskHi << " --- 0x"<< maskLo << " (Two 32-bit words)" << std::dec << std::endl;
    SetWRegister("triggerLogic.TriggerPattern_lowW",  maskLo);
    SetWRegister("triggerLogic.TriggerPattern_highW", maskHi);
  }

  void FmctluController::SetTriggerMask(uint32_t maskHi, uint32_t maskLo){
    std::cout << std::hex << "  TRIGGER PATTERN (for external triggers) SET TO 0x" << maskHi << " --- 0x"<< maskLo << " (Two 32-bit words)" << std::dec << std::endl;
    SetWRegister("triggerLogic.TriggerPattern_lowW",  maskLo);
    SetWRegister("triggerLogic.TriggerPattern_highW", maskHi);
  }

  void FmctluController::SetWRegister(const std::string & name, int value){
    try {
      m_hw->getNode(name).write(static_cast< uint32_t >(value));
      m_hw->dispatch();
    } catch (...) {
      return;
    }
  }

  void FmctluController::SetUhalLogLevel(uchar_t l){
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
    s << "eventnumber: " << d.eventnumber << " type: " << int(d.eventtype) <<" timestamp: 0x" <<std::hex<< d.timestamp <<std::dec<<std::endl
      <<" input0: " << int(d.input0) << " input1: " << int(d.input1) << " input2: " << int(d.input2) << " input3: " << int(d.input3) <<std::endl
      <<" sc0: " << int(d.sc0) << " sc1: "  << int(d.sc1) << " sc2: "  << int(d.sc2) << " sc3: " << int(d.sc3) <<std::endl;
    return s;
  }


}
