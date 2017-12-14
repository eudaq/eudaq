#ifndef H_CALICEAHCALBIFCONTROLLER_HH
#define H_CALICEAHCALBIFCONTROLLER_HH

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <ostream>
#include <memory>

#include "eudaq/Utils.hh"
#include "eudaq/Time.hh"
#include "uhal/uhal.hpp"

#include <boost/filesystem.hpp>

using namespace uhal;

namespace tlu {

   static const int TLU_TRIGGER_INPUTS = 4;
   static const int TLU_PMTS = TLU_TRIGGER_INPUTS;

   class caliceahcalbifController {
      public:
         caliceahcalbifController(const std::string & connectionFilename, const std::string & deviceName);
         ~caliceahcalbifController();

         void SetSerdesRst(int value) {
            caliceahcalbifController::SetWRegister("triggerInputs.SerdesRst", value);
         }
         ;
         void ResetCounters() {
            caliceahcalbifController::SetSerdesRst(0x2);
            caliceahcalbifController::SetSerdesRst(0x0);
         }
         ;

         void ResetSerdes() {
            caliceahcalbifController::SetSerdesRst(0x3);
            caliceahcalbifController::SetSerdesRst(0x0);
            caliceahcalbifController::SetSerdesRst(0x4);
            caliceahcalbifController::SetSerdesRst(0x0);
         }
         ;

         void SetDUTInterfaces(int value) {
            caliceahcalbifController::SetRWRegister("DUTInterfaces", value);
         }
         ;
         void SetInternalTriggerInterval(int value) {
            caliceahcalbifController::SetWRegister("triggerLogic.InternalTriggerIntervalW", value);
         }
         ;
         uint32_t GetInternalTriggerInterval() {
            return caliceahcalbifController::ReadRRegister("triggerLogic.InternalTriggerIntervalR");
         }
         ;
         void SetTriggerMask(int value) {
            caliceahcalbifController::SetWRegister("triggerLogic.TriggerMaskW", value);
         }
         ;
         uint32_t GetTriggerMask() {
            return caliceahcalbifController::ReadRRegister("triggerLogic.TriggerMaskR");
         }
         ;

         void SetDUTMask(int value) {
            caliceahcalbifController::SetWRegister("DUTInterfaces.DutMaskW", value);
         }
         ;
         uint32_t GetDUTMask() {
            return caliceahcalbifController::ReadRRegister("DUTInterfaces.DutMaskR");
         }
         ;

         void SetTriggerVeto(int value) {
            caliceahcalbifController::SetWRegister("triggerLogic.TriggerVetoW", value);
         }
         ;
         uint32_t GetTriggerVeto() {
            return caliceahcalbifController::ReadRRegister("triggerLogic.TriggerVetoR");
         }
         ;
         void AllTriggerVeto() {
            caliceahcalbifController::SetTriggerVeto(1);
         }
         ;
         void NoneTriggerVeto() {
            caliceahcalbifController::SetTriggerVeto(0);
         }
         ;

         void SetEventFifoCSR(int value) {
            caliceahcalbifController::SetRWRegister("eventBuffer.EventFifoCSR", value);
         }
         ;
         uint32_t GetEventFifoCSR() {
            return caliceahcalbifController::ReadRRegister("eventBuffer.EventFifoCSR");
         }
         ;
         void ResetEventFIFO() {
            caliceahcalbifController::SetEventFifoCSR(0x0);
         }
         ;

         void SetLogicClocksCSR(int value) {
            caliceahcalbifController::SetRWRegister("logic_clocks.LogicClocksCSR", value);
         }
         ;
         uint32_t GetLogicClocksCSR() {
            return caliceahcalbifController::ReadRRegister("logic_clocks.LogicClocksCSR");
         }
         ;

         void SetTriggerLength(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.TriggerLength", value);
         }
         ;
         void SetTrigStartupDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.TrigStartupDeadTime", value);
         }
         ;
         void SetTrigInterpulseDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.TrigInterpulseDeadTime", value);
         }
         ;
         void SetTriggerDelay(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.TriggerDelay", value);
         }
         ;
         void SetNMaxTriggers(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.NMaxTriggers", value);
         }
         ;
         void SetTrigRearmDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Trigger_Generator.TrigRearmDeadTime", value);
         }
         ;

         void SetShutterLength(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.ShutterLength", value);
         }
         ;
         void SetShutStartupDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.ShutStartupDeadTime", value);
         }
         ;
         void SetShutInterpulseDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.ShutInterpulseDeadTime", value);
         }
         ;
         void SetShutterDelay(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.ShutterDelay", value);
         }
         ;
         void SetNMaxShutters(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.NMaxShutters", value);
         }
         ;
         void SetShutRearmDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Shutter_Generator.ShutRearmDeadTime", value);
         }
         ;

         void SetSpillLength(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.SpillLength", value);
         }
         ;
         void SetSpillStartupDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.SpillStartupDeadTime", value);
         }
         ;
         void SetSpillInterpulseDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.SpillInterpulseDeadTime", value);
         }
         ;
         void SetSpillDelay(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.SpillDelay", value);
         }
         ;
         void SetNMaxSpillgers(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.NMaxSpills", value);
         }
         ;
         void SetSpillRearmDeadTime(int value) {
            caliceahcalbifController::SetRWRegister("Spill_Generator.SpillRearmDeadTime", value);
         }
         ;

         void SetEnableRecordData(int value) {
            caliceahcalbifController::SetRWRegister("Event_Formatter.Enable_Record_Data", value);
         }
         ;

         uint32_t GetEventFifoFillLevel() {
            return caliceahcalbifController::ReadRRegister("eventBuffer.EventFifoFillLevel");
         }
         ;

         void SetCheckConfig(bool value) {
            m_checkConfig = value;
         }
         ;

         void SetI2CClockPrescale(int value) {
            caliceahcalbifController::SetRWRegister("i2c_master.i2c_pre_lo", value & 0xff);
            caliceahcalbifController::SetRWRegister("i2c_master.i2c_pre_hi", (value >> 8) & 0xff);
         }
         ;

         void SetI2CControl(int value) {
            caliceahcalbifController::SetRWRegister("i2c_master.i2c_ctrl", value & 0xff);
         }
         ;

         void SetI2CCommand(int value) {
            caliceahcalbifController::SetWRegister("i2c_master.i2c_cmdstatus", value & 0xff);
         }
         ;

         uint32_t GetI2CStatus() {
            return caliceahcalbifController::ReadRRegister("i2c_master.i2c_cmdstatus");
         }
         ;

         bool I2CCommandIsDone() {
            return ((GetI2CStatus()) >> 1) & 0x1;
         }
         ;

         void SetI2CTX(int value) {
            caliceahcalbifController::SetWRegister("i2c_master.i2c_rxtx", value & 0xff);
         }

         uint32_t GetI2CRX() {
            return caliceahcalbifController::ReadRRegister("i2c_master.i2c_rxtx");
         }

         uint32_t GetFirmwareVersion() {
            return caliceahcalbifController::ReadRRegister("version");
         }

         uint32_t GetBoardID() {
            return m_BoardID;
         }

         void ResetBoard() {
            caliceahcalbifController::SetWRegister("logic_clocks.LogicRst", 1);
         }

         void CheckEventFIFO();
         void ReadEventFIFO();
         void ReadRawFifo(size_t N);

         uint32_t GetNEvent() {
            return m_nEvtInFIFO / 2;
         }
         // number of entries in the remote fifo (in 32-bit words)
         uint32_t GetNFifo() {
            return m_nEvtInFIFO;
         }
         uint64_t GetEvent(int i) {
            return m_dataFromTLU[i];
         }
         std::vector<uint64_t>* GetEventData() {
            return &m_dataFromTLU;
         }
         void ClearEventFIFO() {
            m_dataFromTLU.resize(0);
         }

         unsigned GetScaler(unsigned) const;

         void InitializeI2C(char DACaddr, char IDaddr);

         void SetDACValue(unsigned char channel, uint32_t value);

         void SetThresholdValue(unsigned char channel, float thresholdVoltage);

         void ConfigureInternalTriggerInterval(unsigned int value);

         void DumpEvents();
      private:
         HwInterface * m_hw;
         bool m_checkConfig;
         void SetRWRegister(const std::string & name, int value);
         void SetWRegister(const std::string & name, int value);
         uint32_t ReadRRegister(const std::string & name);
         char ReadI2CChar(char deviceAddr, char memAddr);
         void WriteI2CChar(char deviceAddr, char memAddr, char value);
         void WriteI2CCharArray(char deviceAddr, char memAddr, unsigned char *values, unsigned int len);
         uint32_t m_nEvtInFIFO;

         bool m_ipbus_verbose;

         char m_DACaddr;
         char m_IDaddr;

         uint64_t m_BoardID;

         std::vector<uint64_t> m_dataFromTLU;

         unsigned m_scalers[TLU_TRIGGER_INPUTS];
         unsigned m_vetostatus, m_fsmstatus, m_dutbusy, m_clockstat;

   };
}

#endif
