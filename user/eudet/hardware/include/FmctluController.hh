#ifndef H_FMCTLUCONTROLLER_HH
#define H_FMCTLUCONTROLLER_HH

#include <deque>
#include <string>
#include <vector>
#include <iostream>
#include "FmctluI2c.hh"
#include "FmctluHardware.hh"

typedef unsigned char uchar_t;

namespace uhal{
  class HwInterface;
}
using namespace uhal;

namespace tlu {

  class fmctludata;

  class FmctluController {
  public:
    FmctluController(const std::string & connectionFilename, const std::string & deviceName);
    ~FmctluController(){ResetEventsBuffer();};

    void enableHDMI(unsigned int dutN, bool enable, bool verbose);
    unsigned int PackBits(std::vector< unsigned int>  rawValues);
    void SetSerdesRst(int value) { SetWRegister("triggerInputs.SerdesRstW",value); };
    void SetInternalTriggerInterval(int value) { SetWRegister("triggerLogic.InternalTriggerIntervalW",value); };
    //void SetTriggerMask(int value) { SetWRegister("triggerLogic.TriggerMaskW",value); };
    void SetTriggerMask(uint64_t value);
    void SetTriggerMask(uint32_t maskHi, uint32_t maskLo);
    void SetTriggerVeto(int value) { SetWRegister("triggerLogic.TriggerVetoW",value); };
    void SetPulseStretch(int value) { SetWRegister("triggerLogic.PulseStretchW",value); };
    void SetPulseDelay(int value) { SetWRegister("triggerLogic.PulseDelayW",value); };
    void SetPulseStretchPack(std::vector< unsigned int>  valuesVec);
    void SetPulseDelayPack(std::vector< unsigned int>  valuesVec);
    void SetDUTMask(int value) { SetWRegister("DUTInterfaces.DUTMaskW",value); };
    void SetDUTMaskMode(int value) { SetWRegister("DUTInterfaces.DUTInterfaceModeW",value); };
    void SetDUTMaskModeModifier(int value) { SetWRegister("DUTInterfaces.DUTInterfaceModeModifierW",value); };
    void SetDUTIgnoreBusy(int value){ SetWRegister("DUTInterfaces.IgnoreDUTBusyW",value); };
    void SetDUTIgnoreShutterVeto(int value){ SetWRegister("DUTInterfaces.IgnoreShutterVetoW",value); };

    uint32_t GetDUTMask() { return ReadRRegister("DUTInterfaces.DUTMaskR"); };

    void SetEventFifoCSR(int value) { SetWRegister("eventBuffer.EventFifoCSR",value); };
    void SetLogicClocksCSR(int value) { SetWRegister("logic_clocks.LogicClocksCSR",value); };


    void SetEnableRecordData(int value) { SetWRegister("Event_Formatter.Enable_Record_Data",value); };

    uint32_t GetLogicClocksCSR() { return ReadRRegister("logic_clocks.LogicClocksCSR"); };
    uint32_t GetInternalTriggerInterval() { return ReadRRegister("triggerLogic.InternalTriggerIntervalR"); };
    uint32_t GetPulseStretch(){ return ReadRRegister("triggerLogic.PulseStretchR"); };
    uint32_t GetPulseDelay() { return ReadRRegister("triggerLogic.PulseDelayR"); };
    //uint32_t GetTriggerMask() { return ReadRRegister("triggerLogic.TriggerMaskR"); };
    uint64_t GetTriggerMask();
    uint32_t GetTriggerVeto() { return ReadRRegister("triggerLogic.TriggerVetoR"); };
    uint32_t GetPreVetoTriggers() { return ReadRRegister("triggerLogic.PreVetoTriggersR"); };
    uint32_t GetPostVetoTriggers() { return ReadRRegister("triggerLogic.PostVetoTriggersR"); };
    uint64_t GetCurrentTimestamp(){
      uint64_t time = ReadRRegister("Event_Formatter.CurrentTimestampHR");
      time = time << 32;
      time = time + ReadRRegister("Event_Formatter.CurrentTimestampLR");
      return time;
    }

    uint32_t GetFW();
    uint32_t GetEventFifoCSR();
    uint32_t GetEventFifoFillLevel();
    uint32_t GetI2CStatus() { return ReadRRegister("i2c_master.i2c_cmdstatus"); };
    uint32_t GetI2CRX() { return ReadRRegister("i2c_master.i2c_rxtx"); };
    uint32_t GetFirmwareVersion() { return ReadRRegister("version"); };
    void GetScaler(uint32_t &s0, uint32_t &s1, uint32_t &s2, uint32_t &s3, uint32_t &s4, uint32_t &s5 ){
      s0 = ReadRRegister("triggerInputs.ThrCount0R");
      s1 = ReadRRegister("triggerInputs.ThrCount1R");
      s2 = ReadRRegister("triggerInputs.ThrCount2R");
      s3 = ReadRRegister("triggerInputs.ThrCount3R");
      s4 = ReadRRegister("triggerInputs.ThrCount4R");
      s5 = ReadRRegister("triggerInputs.ThrCount5R");
    }


    void SetI2CClockPrescale(int value) {
      SetWRegister("i2c_master.i2c_pre_lo", value&0xff);
      SetWRegister("i2c_master.i2c_pre_hi", (value>>8)&0xff);
    };

    uint32_t I2C_enable(char EnclustraExpAddr);
    uint64_t getSN();

    void SetI2CControl(int value) { SetWRegister("i2c_master.i2c_ctrl", value&0xff); };
    void SetI2CCommand(int value) { SetWRegister("i2c_master.i2c_cmdstatus", value&0xff); };
    void SetI2CTX(int value) { SetWRegister("i2c_master.i2c_rxtx", value&0xff); };

    void ResetBoard() {SetWRegister("logic_clocks.LogicRst", 1);};
    void ResetTimestamp() {SetWRegister("Event_Formatter..ResetTimestampW", 1);};


    bool I2CCommandIsDone() { return ((GetI2CStatus())>>1)&0x1; };
    uint32_t GetBoardID() { return m_BoardID; }
    void ResetFIFO() { SetEventFifoCSR(0x0); };


    void ResetCounters() {
      SetSerdesRst(0x2);
      SetSerdesRst(0x0);
    };

    void ResetSerdes() {
      SetSerdesRst(0x3);
      SetSerdesRst(0x0);
      SetSerdesRst(0x4);
      SetSerdesRst(0x0);
    };

    fmctludata* PopFrontEvent();
    bool IsBufferEmpty(){return m_data.empty();};
    void ReceiveEvents();
    void ResetEventsBuffer();
    void DumpEventsBuffer();

    void enableClkLEMO(bool enable, bool verbose);
    //void InitializeI2C(char DACaddr, char IDaddr);
    void InitializeClkChip(const std::string & filename);
    void InitializeDAC();
    void InitializeIOexp();
    void InitializeI2C();
    void SetDACValue(unsigned char channel, uint32_t value);
    void SetThresholdValue(unsigned char channel, float thresholdVoltage);
    void SetDutClkSrc(unsigned int hdmiN, unsigned int source, bool verbose);

    void SetWRegister(const std::string & name, int value);
    uint32_t ReadRRegister(const std::string & name);

    void SetUhalLogLevel(uchar_t l);


    void SetI2C_core_addr(char addressa) { m_I2C_address.core = addressa; };
    void SetI2C_clockChip_addr(char addressa) { m_I2C_address.clockChip = addressa; };
    void SetI2C_DAC1_addr(char addressa) { m_I2C_address.DAC1 = addressa; };
    void SetI2C_DAC2_addr(char addressa) { m_I2C_address.DAC2 = addressa; };
    void SetI2C_EEPROM_addr(char addressa) { m_I2C_address.EEPROM = addressa; };
    void SetI2C_expander1_addr(char addressa) { m_I2C_address.expander1 = addressa; };
    void SetI2C_expander2_addr(char addressa) { m_I2C_address.expander2 = addressa; };
  private:


    struct I2C_addresses{
      char core;
      char clockChip;
      char DAC1;
      char DAC2;
      char EEPROM;
      char expander1;
      char expander2;
    } m_I2C_address;


    HwInterface * m_hw; //Instance of IPBus
    i2cCore *m_i2c; //Instance of I2C

    char m_DACaddr;
    char m_IDaddr;
    uint64_t m_BoardID;

    // Instantiate on-board hardware (I2C slaves)
    AD5665R m_zeDAC1, m_zeDAC2;
    PCA9539PW m_IOexpander1, m_IOexpander2;
    Si5345 m_zeClock;

    std::deque<fmctludata*> m_data;

  };

  class fmctludata{
  public:
    fmctludata(uint64_t wl, uint64_t wh):  // wl -> wh
      eventtype((wl>>60)&0xf),
      input0((wl>>59)&0x1),
      input1((wl>>58)&0x1),
      input2((wl>>57)&0x1),
      input3((wl>>56)&0x1),
      timestamp(wl&0xffffffffffff),
      sc0((wh>>56)&0xff),
      sc1((wh>>48)&0xff),
      sc2((wh>>40)&0xff),
      sc3((wh>>32)&0xff),
      eventnumber(wh&0xffffffff){
    }

    fmctludata(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) // w0 w1 w2 w3  wl= w0 w1; wh= w2 w3
      :eventtype((w0>>28)&0xf),
       input0((w0>>27)&0x1),
       input1((w0>>26)&0x1),
       input2((w0>>25)&0x1),
       input3((w0>>24)&0x1),
       timestamp(((uint64_t(w0&0xffff))<<32) + w1),
       // timestamp(w1),
       sc0((w2>>24)&0xff),
       sc1((w2>>16)&0xff),
       sc2((w2>>8)&0xff),
       sc3(w2&0xff),
       eventnumber(w3),
       timestamp1(w0&0xffff){
    }

    uchar_t eventtype;
    uchar_t input0;
    uchar_t input1;
    uchar_t input2;
    uchar_t input3;
    uint64_t timestamp;
    uchar_t sc0;
    uchar_t sc1;
    uchar_t sc2;
    uchar_t sc3;
    uint32_t eventnumber;
    uint64_t timestamp1;

  };

  std::ostream &operator<<(std::ostream &s, fmctludata &d);

}

#endif
