#include "tlu/miniTLUController.hh"

#include "eudaq/Platform.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Timer.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"


#if EUDAQ_PLATFORM_IS(WIN32)
# include <cstdio>  // HK
#else
# include <unistd.h>
#endif

#include <iostream>
#include <ostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cmath>

using eudaq::mSleep;
using eudaq::hexdec;
using eudaq::to_string;
using eudaq::to_hex;
using eudaq::ucase;

using namespace uhal;

namespace tlu {
  miniTLUController::miniTLUController(const std::string & connectionFilename, const std::string & deviceName) : m_hw(0), m_dataFromTLU(0), m_DACaddr(0), m_IDaddr(0) {

    std::cout << "Configuring from " << connectionFilename << " the device " << deviceName << std::endl;
    if(!m_hw) {
      ConnectionManager manager ( connectionFilename );
      m_hw = new uhal::HwInterface(manager.getDevice( deviceName ));

      // HwInterface hw = manager.getDevice( deviceName );
      // m_hw = counted_ptr<HwInterface>( new HwInterface(hw) );
    }
    m_checkConfig = false;
    m_ipbus_verbose = false;
    m_nEvtInFIFO = 0;
  }


  miniTLUController::~miniTLUController() {
  }

  void miniTLUController::SetRWRegister(const std::string & name, int value) {
    m_hw->getNode(name).write(static_cast< uint32_t >(value));
    m_hw->dispatch();

    if (m_checkConfig) {
      ValWord< uint32_t > test = m_hw->getNode(name).read();
      m_hw->dispatch();
      if(test.valid()) {
	if (m_ipbus_verbose) 
	  std::cout << name << " = " << std::hex << test.value() << std::endl;
      } else std::cout << "Error writing " << name << std::endl;
    }
  }

  void miniTLUController::SetWRegister(const std::string & name, int value) {
    m_hw->getNode(name).write(static_cast< uint32_t >(value));
    m_hw->dispatch();
  }

  uint32_t miniTLUController::ReadRRegister(const std::string & name) {
    ValWord< uint32_t > test = m_hw->getNode(name).read();
    m_hw->dispatch();
    if(test.valid()) {
      if (m_ipbus_verbose) 
	std::cout << name << " = " << std::hex << test.value() << std::endl;
      return test.value();
    } else {
      std::cout << "Error reading " << name << std::endl;
      return 0;
    }
  }

  void miniTLUController::CheckEventFIFO() {
    m_nEvtInFIFO = miniTLUController::ReadRRegister("eventBuffer.EventFifoFillLevel");
    //   m_nEvtInFIFO = 2;
    if (m_nEvtInFIFO) std::cout << "nEvt : " << m_nEvtInFIFO << std::endl;
  }

  void miniTLUController::ReadEventFIFO() {
    if(m_nEvtInFIFO) {
      if (!(m_nEvtInFIFO)) std::cout << "Warning odd words in fifo!" << std::endl;
      ValVector< uint32_t > fifoContent = m_hw->getNode("eventBuffer.EventFifoData").readBlock(m_nEvtInFIFO);
      m_hw->dispatch();
      if(fifoContent.valid()) {
	bool highBits = false;
	uint64_t word = 0;
	//	std::cout << "Dump event FIFO" << std::endl;
	for ( ValVector< uint32_t >::const_iterator i ( fifoContent.begin() ); i!=fifoContent.end(); ++i ) {
	  //	  std::cout << "-- " << std::hex << *i << std::endl;
	  if(highBits) {
	    word = (((uint64_t)(*i))<<32) | word;
	    m_dataFromTLU.push_back(word);
	    highBits = false;
	  } else {
	    word = *i;
	    highBits = true;
	  }
	}
      } else {
	std::cout << "Error reading FIFO" << std::endl;
      }      
    }
  }

  void miniTLUController::InitializeI2C(char DACaddr, char IDaddr) {
    SetI2CClockPrescale(0x30);
    SetI2CClockPrescale(0x30);
    SetI2CControl(0x80);
    SetI2CControl(0x80);
    
    std::cout << "Scan I2C bus:" << std::endl;
    for(int myaddr = 0; myaddr < 128; myaddr++) {
      SetI2CTX((myaddr<<1)|0x0);
      SetI2CCommand(0x90);
      while(I2CCommandIsDone()) {
	eudaq::mSleep(1000);
      }
      bool isConnected = (((GetI2CStatus()>>7)&0x1) == 0);
      if(myaddr == DACaddr) {
	if (isConnected) { 
	  std::cout << "DAC at addr " << std::hex << myaddr << " is connected" << std::endl;
	  m_DACaddr = myaddr;
	} else {
	  std::cout << "DAC at addr " << std::hex << DACaddr << " is NOT connected" << std::endl;
	}
      } else if (myaddr == IDaddr) {
	if (isConnected) { 
	  std::cout << "ID at addr " << std::hex << myaddr << " is connected" << std::endl;
	  m_IDaddr = myaddr;
	} else {
	  std::cout << "ID at addr " << std::hex << DACaddr << " is NOT connected" << std::endl;
	}
      } else if (isConnected) 
	std::cout << "Device " << std::hex << myaddr << " is connected" << std::endl;
      SetI2CTX(0x0);
      SetI2CCommand(0x50);
      while(I2CCommandIsDone()) {
	eudaq::mSleep(1000);
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
      eudaq::mSleep(1000);
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    SetI2CTX(value);
    SetI2CCommand(0x50);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
  }

  void miniTLUController::WriteI2CCharArray(char deviceAddr, char memAddr, unsigned char *values, unsigned int len) {
    unsigned int i;

    SetI2CTX((deviceAddr<<1)|0x0);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    for(i = 0; i < len-1; ++i) {
      SetI2CTX(values[i]);
      SetI2CCommand(0x10);
      while(I2CCommandIsDone()) {
	eudaq::mSleep(1000);
      }
    }
    SetI2CTX(values[len-1]);
    SetI2CCommand(0x50);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
  }

  char miniTLUController::ReadI2CChar(char deviceAddr, char memAddr) {
    SetI2CTX((deviceAddr<<1)|0x0);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    SetI2CTX(memAddr);
    SetI2CCommand(0x10);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    SetI2CTX((deviceAddr<<1)|0x1);
    SetI2CCommand(0x90);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    SetI2CCommand(0x28);
    while(I2CCommandIsDone()) {
      eudaq::mSleep(1000);
    }
    return GetI2CRX();
  }

  unsigned TLUController::GetScaler(unsigned i) const {
    if (i >= (unsigned)TLU_TRIGGER_INPUTS) EUDAQ_THROW("Scaler number out of range");
    m_nEvtInFIFO = miniTLUController::ReadRRegister("eventBuffer.EventFifoFillLevel");

    //return m_scalers[i];
  }

  void miniTLUController::SetDACValue(unsigned char channel, uint32_t value) {
    unsigned char chrsToSend[2];

    std::cout << "Setting DAC channel " << channel << " = " << value << std::endl;

    // enter vref-off mode: ( very early TLU versions needed Vref mode on. )
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

    std::cout << "Setting threshold for channel " << channel << " to " << thresholdVoltage << " Volts" << std::endl;
    float vref = 1.300 ; // Reference voltage is 1.3V on newer TLU
    float vdac = ( thresholdVoltage + vref ) / 2;
    float dacCode =  0xFFFF * vdac / vref;

    if ( std::abs(thresholdVoltage) > vref )  EUDAQ_THROW("Threshold voltage must be > -1.3V and < 1.3V");

    SetDACValue(channel , int(dacCode) );
      
  }

}
