#include "miniTLUController.hh"

#include <iomanip>
#include <thread>
#include <chrono>

#include "uhal/uhal.hpp"

namespace tlu {
  miniTLUController::miniTLUController(const std::string & connectionFilename, const std::string & deviceName) : m_hw(0), m_DACaddr(0), m_IDaddr(0) {

    std::cout << "Configuring from " << connectionFilename << " the device " << deviceName << std::endl;
    if(!m_hw) {
      ConnectionManager manager ( connectionFilename );
      m_hw = new uhal::HwInterface(manager.getDevice( deviceName ));

    }
  }


  void miniTLUController::SetWRegister(const std::string & name, int value) {
    try {
    m_hw->getNode(name).write(static_cast< uint32_t >(value));
    m_hw->dispatch();
    } catch (...) {
       return;
    }
  }

  uint32_t miniTLUController::ReadRRegister(const std::string & name) {
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


  void miniTLUController::ReceiveEvents(){
    // std::cout<<"miniTLUController::ReceiveEvents"<<std::endl;
    uint32_t nevent = GetEventFifoFillLevel()/4;
    // std::cout<< "fifo "<<GetEventFifoFillLevel()<<std::endl;
    if (nevent*4 == 0x7D00) std::cout << "WARNING! miniTLU hardware FIFO is full" << std::endl;
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
	  m_data.push_back(new minitludata(*i, *(i+1), *(i+2), *(i+3)));
	  std::cout<< *(m_data.back());
	}
      }	
    }
  }
  
  void miniTLUController::ResetEventsBuffer(){
    for(auto &&i: m_data){
      delete i;
    }
    m_data.clear();
  }

  minitludata* miniTLUController::PopFrontEvent(){
    minitludata *e = m_data.front();
    m_data.pop_front();
    return e;
  }

  
  void miniTLUController::InitializeI2C(char DACaddr, char IDaddr) {
    SetI2CClockPrescale(0x30);
    SetI2CClockPrescale(0x30);
    SetI2CControl(0x80);
    SetI2CControl(0x80);
    
    std::cout << "Scan I2C bus:" << std::endl;
    for(int myaddr = 0; myaddr < 128; myaddr++) {
      SetI2CTX((myaddr<<1)|0x0);
      SetI2CCommand(0x90); // 10010000
      while(I2CCommandIsDone()) { // xxxxxx1x   TODO:: isDone or notDone
	std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      bool isConnected = (((GetI2CStatus()>>7)&0x1) == 0);  // 0xxxxxxx connected
      if(myaddr == DACaddr) {
	if (isConnected) { 
	  std::cout << "DAC at addr 0x" << std::hex << myaddr << std::dec<< " is connected" << std::endl;
	  m_DACaddr = myaddr;
	} else {
	  std::cout << "DAC at addr 0x" << std::hex << DACaddr << std::dec<<" is NOT connected" << std::endl;
	}
      } else if (myaddr == IDaddr) {
	if (isConnected) { 
	  std::cout << "ID at addr 0x" << std::hex << myaddr << std::dec<<" is connected" << std::endl;
	  m_IDaddr = myaddr;
	} else {
	  std::cout << "ID at addr 0x" << std::hex << DACaddr << std::dec<<" is NOT connected" << std::endl;
	}
      } else if (isConnected) 
	std::cout << "Device 0x" << std::hex << myaddr << std::dec<<" is connected" << std::endl;
      SetI2CTX(0x0);
      SetI2CCommand(0x50); // 01010000
      while(I2CCommandIsDone()) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
  
    if(m_IDaddr) {
      m_BoardID = 0;
      for(unsigned char myaddr = 0xfa; myaddr > 0x0; myaddr++) {
	char nibble = ReadI2CChar(m_IDaddr, myaddr);
	m_BoardID = ((((uint64_t)nibble)&0xff)<<((0xff-myaddr)*8))|m_BoardID;
      }
      std::cout << "Unique ID : " << std::setw(12) << std::setfill('0') << m_BoardID << std::endl;
    }
  }

  void miniTLUController::WriteI2CChar(char deviceAddr, char memAddr, char value) {
    SetI2CTX((deviceAddr<<1)|0x0);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CTX(value);
    SetI2CCommand(0x50);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  void miniTLUController::WriteI2CCharArray(char deviceAddr, char memAddr, unsigned char *values, unsigned int len) {
    unsigned int i;

    SetI2CTX((deviceAddr<<1)|0x0);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    for(i = 0; i < len-1; ++i) {
      SetI2CTX(values[i]);
      SetI2CCommand(0x10);
      while(I2CCommandIsDone()) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    SetI2CTX(values[len-1]);
    SetI2CCommand(0x50);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }

  char miniTLUController::ReadI2CChar(char deviceAddr, char memAddr) {
    SetI2CTX((deviceAddr<<1)|0x0);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CTX((deviceAddr<<1)|0x1);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    SetI2CCommand(0x28);
    while(I2CCommandIsDone()) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return GetI2CRX();
  }


  void miniTLUController::SetDACValue(unsigned char channel, uint32_t value) {
    unsigned char chrsToSend[2];

    std::cout << "Setting DAC channel " << (unsigned int)channel << " = " << value << std::endl;

    chrsToSend[0] = 0x0;
    chrsToSend[1] = 0x0;
    //chrsToSend[1] = 0x1;
    WriteI2CCharArray(m_DACaddr, 0x38, chrsToSend, 2);

    // set the value
    chrsToSend[1] = value&0xff;
    chrsToSend[0] = (value>>8)&0xff;
    WriteI2CCharArray(m_DACaddr, 0x18+(channel&0x7), chrsToSend, 2);
  }

  void miniTLUController::SetThresholdValue(unsigned char channel, float thresholdVoltage ) {

    std::cout << "Setting threshold for channel " << (unsigned int)channel << " to " << thresholdVoltage << " Volts" << std::endl;
    float vref = 1.300 ; // Reference voltage is 1.3V on newer TLU
    float vdac = ( thresholdVoltage + vref ) / 2;
    float dacCode =  0xFFFF * vdac / vref;

    if ( std::abs(thresholdVoltage) > vref )
      std::cout<<"Threshold voltage must be > -1.3V and < 1.3V"<<std::endl;

    SetDACValue(channel , int(dacCode) );
      
  }

  void miniTLUController::DumpEventsBuffer() {
    std::cout<<"miniTLUController::DumpEvents......"<<std::endl;
    for(auto&& i: m_data){
      std::cout<<i<<std::endl;
    }
    std::cout<<"miniTLUController::DumpEvents end"<<std::endl;
  }


  void miniTLUController::SetUhalLogLevel(uchar_t l){
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


  
  std::ostream &operator<<(std::ostream &s, minitludata &d) {
    s << "eventnumber: " << d.eventnumber << " type: " << int(d.eventtype) <<" timestamp: 0x" <<std::hex<< d.timestamp <<std::dec<<std::endl
      <<" input0: " << int(d.input0) << " input1: " << int(d.input1) << " input2: " << int(d.input2) << " input3: " << int(d.input3) <<std::endl
      <<" sc0: " << int(d.sc0) << " sc1: "  << int(d.sc1) << " sc2: "  << int(d.sc2) << " sc3: " << int(d.sc3) <<std::endl;
    return s;
  } 
  

}
