#include "tlu/TLUAddresses.hh"
#include "tlu/TLU_address_map_v0-2.h"

#define BEAM_TRIGGER_FSM_RESET_BIT 4
#define DMA_CONTROLLER_RESET_BIT 5
#define TRIGGER_SCALERS_RESET_BIT 6
#define DUT_LED_ADDRESS EUDAQ_TLU_MISSING

#ifndef I2C_BUS_LEMO_ADC
#define I2C_BUS_LEMO_ADC EUDAQ_TLU_MISSING
#endif
#ifndef I2C_BUS_LEMO_LED_IO_VB
#define I2C_BUS_LEMO_LED_IO_VB I2C_BUS_LEMO_LED_IO
#endif

namespace tlu {

#define EUDAQ_TLU_REG(r) r,

  TLUAddresses v0_2 = {
    EUDAQ_TLU_REGISTERS
    0
  };

}
