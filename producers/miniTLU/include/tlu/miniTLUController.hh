#ifndef H_MINITLUCONTROLLER_HH
#define H_MINITLUCONTROLLER_HH

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <ostream>
#include <memory>

#include "eudaq/Utils.hh"
#include "eudaq/Time.hh"
#include "uhal/uhal.hpp"

#include "tlu/RawDataQueue.hh"
 
#include <boost/filesystem.hpp>

using namespace uhal;

namespace tlu {

  static const int TLU_TRIGGER_INPUTS = 4;
  static const int TLU_PMTS = TLU_TRIGGER_INPUTS ;


  class miniTLUController {
  public:
    miniTLUController(const std::string & connectionFilename, const std::string & deviceName);
    ~miniTLUController();

    void SetSerdesRst(int value) { miniTLUController::SetWRegister("triggerInputs.SerdesRst",value); };
    void ResetCounters() {
      miniTLUController::SetSerdesRst(0x2);
      miniTLUController::SetSerdesRst(0x0);
    };

    void ResetSerdes() {
      miniTLUController::SetSerdesRst(0x3);
      miniTLUController::SetSerdesRst(0x0);
      miniTLUController::SetSerdesRst(0x4);
      miniTLUController::SetSerdesRst(0x0);
    };

    void SetDUTInterfaces(int value) { miniTLUController::SetRWRegister("DUTInterfaces",value); };
    void SetInternalTriggerInterval(int value) { miniTLUController::SetWRegister("triggerLogic.InternalTriggerIntervalW",value); };
    uint32_t GetInternalTriggerInterval() { return miniTLUController::ReadRRegister("triggerLogic.InternalTriggerIntervalR"); };
    void SetTriggerMask(int value) { miniTLUController::SetWRegister("triggerLogic.TriggerMaskW",value); };
    uint32_t GetTriggerMask() { return miniTLUController::ReadRRegister("triggerLogic.TriggerMaskR"); };

    void SetDUTMask(int value) { miniTLUController::SetWRegister("DUTInterfaces.DutMaskW",value); };
    uint32_t GetDUTMask() { return miniTLUController::ReadRRegister("DUTInterfaces.DutMaskR"); };

    void SetTriggerVeto(int value) { miniTLUController::SetWRegister("triggerLogic.TriggerVetoW",value); };
    uint32_t GetTriggerVeto() { return miniTLUController::ReadRRegister("triggerLogic.TriggerVetoR"); };
    void AllTriggerVeto() { miniTLUController::SetTriggerVeto(1); };
    void NoneTriggerVeto() { miniTLUController::SetTriggerVeto(0); };

    void SetEventFifoCSR(int value) { miniTLUController::SetRWRegister("eventBuffer.EventFifoCSR",value); };
    uint32_t GetEventFifoCSR() { return miniTLUController::ReadRRegister("eventBuffer.EventFifoCSR"); };
    void ResetEventFIFO() { miniTLUController::SetEventFifoCSR(0x0); };

    void SetLogicClocksCSR(int value) { miniTLUController::SetRWRegister("logic_clocks.LogicClocksCSR",value); };
    uint32_t GetLogicClocksCSR() { return miniTLUController::ReadRRegister("logic_clocks.LogicClocksCSR"); };

    void SetTriggerLength(int value) { miniTLUController::SetRWRegister("Trigger_Generator.TriggerLength",value); };
    void SetTrigStartupDeadTime(int value) { miniTLUController::SetRWRegister("Trigger_Generator.TrigStartupDeadTime",value); };
    void SetTrigInterpulseDeadTime(int value) { miniTLUController::SetRWRegister("Trigger_Generator.TrigInterpulseDeadTime",value); };
    void SetTriggerDelay(int value) { miniTLUController::SetRWRegister("Trigger_Generator.TriggerDelay",value); };
    void SetNMaxTriggers(int value) { miniTLUController::SetRWRegister("Trigger_Generator.NMaxTriggers",value); };
    void SetTrigRearmDeadTime(int value) { miniTLUController::SetRWRegister("Trigger_Generator.TrigRearmDeadTime",value); };

    void SetShutterLength(int value) { miniTLUController::SetRWRegister("Shutter_Generator.ShutterLength",value); };
    void SetShutStartupDeadTime(int value) { miniTLUController::SetRWRegister("Shutter_Generator.ShutStartupDeadTime",value); };
    void SetShutInterpulseDeadTime(int value) { miniTLUController::SetRWRegister("Shutter_Generator.ShutInterpulseDeadTime",value); };
    void SetShutterDelay(int value) { miniTLUController::SetRWRegister("Shutter_Generator.ShutterDelay",value); };
    void SetNMaxShutters(int value) { miniTLUController::SetRWRegister("Shutter_Generator.NMaxShutters",value); };
    void SetShutRearmDeadTime(int value) { miniTLUController::SetRWRegister("Shutter_Generator.ShutRearmDeadTime",value); };

    void SetSpillLength(int value) { miniTLUController::SetRWRegister("Spill_Generator.SpillLength",value); };
    void SetSpillStartupDeadTime(int value) { miniTLUController::SetRWRegister("Spill_Generator.SpillStartupDeadTime",value); };
    void SetSpillInterpulseDeadTime(int value) { miniTLUController::SetRWRegister("Spill_Generator.SpillInterpulseDeadTime",value); };
    void SetSpillDelay(int value) { miniTLUController::SetRWRegister("Spill_Generator.SpillDelay",value); };
    void SetNMaxSpillgers(int value) { miniTLUController::SetRWRegister("Spill_Generator.NMaxSpills",value); };
    void SetSpillRearmDeadTime(int value) { miniTLUController::SetRWRegister("Spill_Generator.SpillRearmDeadTime",value); };

    void SetEnableRecordData(int value) { miniTLUController::SetRWRegister("Event_Formatter.Enable_Record_Data",value); };

    uint32_t GetEventFifoFillLevel() { return miniTLUController::ReadRRegister("eventBuffer.EventFifoFillLevel"); };

    void SetCheckConfig(bool value) { m_checkConfig = value; };

    void SetI2CClockPrescale(int value) {
      miniTLUController::SetRWRegister("i2c_master.i2c_pre_lo", value&0xff);
      miniTLUController::SetRWRegister("i2c_master.i2c_pre_hi", (value>>8)&0xff);
    };

    void SetI2CControl(int value) { miniTLUController::SetRWRegister("i2c_master.i2c_ctrl", value&0xff); };

    void SetI2CCommand(int value) { miniTLUController::SetWRegister("i2c_master.i2c_cmdstatus", value&0xff); };

    uint32_t GetI2CStatus() { return miniTLUController::ReadRRegister("i2c_master.i2c_cmdstatus"); };

    bool I2CCommandIsDone() { return ((GetI2CStatus())>>1)&0x1; };

    void SetI2CTX(int value) { miniTLUController::SetWRegister("i2c_master.i2c_rxtx", value&0xff); };

    uint32_t GetI2CRX() { return miniTLUController::ReadRRegister("i2c_master.i2c_rxtx"); };

    uint32_t GetFirmwareVersion() { return miniTLUController::ReadRRegister("version"); };

    uint32_t GetBoardID() { return m_BoardID; }

    void ResetBoard() { miniTLUController::SetWRegister("logic_clocks.LogicRst", 1); };

    void CheckEventFIFO();
    void ReadEventFIFO();
    void ReadEventFIFO(RawDataQueue &queue);

    uint32_t GetNEvent() { return m_nEvtInFIFO/2; }
    uint64_t GetEvent(int i) { return m_dataFromTLU[i]; }
    std::vector<uint64_t>* GetEventData() { return &m_dataFromTLU; }
    std::vector<uint64_t> GetEventDataV() { return m_dataFromTLU; }
    void ClearEventFIFO() { m_dataFromTLU.resize(0); }

    unsigned GetScaler(unsigned) const;

    void InitializeI2C(char DACaddr, char IDaddr);

    void SetDACValue(unsigned char channel, uint32_t value);

    void SetThresholdValue(unsigned char channel, float thresholdVoltage);

    void ConfigureInternalTriggerInterval(unsigned int value);

    void DumpEvents();

    void SetCatchedError() { m_catchedError = true; }
    void ClearCatchedError() { m_catchedError = false; }

    void SetMaxReadSize(unsigned int value) { m_maxRead = value; }

    std::string & GetConnectionFile() { return m_filename; }
    std::string & GetDeviceName() { return m_devicename; }

  private:
    HwInterface * m_hw;
    bool m_checkConfig;
    bool m_catchedError;
    void SetRWRegister(const std::string & name, int value);
    void SetWRegister(const std::string & name, int value);
    uint32_t ReadRRegister(const std::string & name);
    char ReadI2CChar(char deviceAddr, char memAddr);
    void WriteI2CChar(char deviceAddr, char memAddr, char value);
    void WriteI2CCharArray(char deviceAddr, char memAddr, unsigned char *values, unsigned int len);
    uint32_t m_nEvtInFIFO;
    unsigned int m_maxRead;

    bool m_ipbus_verbose;

    char m_DACaddr;
    char m_IDaddr;

    uint64_t m_BoardID;

    std::vector<uint64_t> m_dataFromTLU;

    unsigned m_scalers[TLU_TRIGGER_INPUTS];
    unsigned m_vetostatus, m_fsmstatus, m_dutbusy, m_clockstat;

    std::string m_filename, m_devicename;
  };
}

#endif
