# Trigger Logic Units (TLU) supported in EUDAQ2

## Building

By default the cmake-flag is activated ```USER_TLU_BUILD=ON```.
This builds the converter module for a standard event, for the online monitor for example.

If cmake finds the EUDET TLU or the AIDA TLU dependencies, these producers are automatically build (by switching on the flags ```USER_TLU_BUILD_EUDET``` or ```USER_TLU_BUILD_AIDA```).


## AIDA TLU producer

### Dependencies

It is only built if cmake finds the external dependencies:
- Cactus/Ipbus (Follow these [instructions](https://ipbus.web.cern.ch/ipbus/doc/user/html/software/install/compile.html#instructions). Cactus should be installed to /opt/cactus which is the default.)

Please find here the hardware [manual](https://www.ohwr.org/project/fmc-mtlu/blob/master/Documentation/Main_TLU.pdf).

### Usage

To start the EUDET TLU producer ```euCliProducer -n AidaTluProducer```. To use the provided ini- and conf-file as example, the run time name is to be set when starting:  ```euCliProducer -n AidaTluProducer -t aida_tlu```.
The usage with EUDAQ2 and EUDET-type telescopes is described [here](https://telescopes.desy.de/User_manual#Running_with_EUDAQ_2). Find the application (starting scripts and conf-file) for EUDET-type telescope in [user/eudet/misc](../../user/eudet/misc)

### Parameter for intialization

To be set in the ```*.ini```-file for EUDAQ as section assuming the tag is ```euCliProducer -n AidaTluProducer -t aida_tlu```

The ```[Producer.aida_tlu]```-section has the following parameters:
 - `initid` (*string*, optional): track your changes, e.g. using the date; default: `0`
 - `verbose` (*0* or *1*, optional): enable verbose mode for debugging; default: `0`
 - `skipini` (*0* or *1*, optional): set skipini to 1, if you want to skip the following init-step; default: `0`
 - `ConnectionFile` (*string*, required): absolute path to xml connection file (`file:///<absolute PATH>`) or relative path from execution directory (`file://<relative PATH>`); the xml file also defines the address table; default: ``
 - `DeviceName` (*string*, required): name (connection id) for indicating connection type given in xml connection file; default: ``
 - `confclock` (*0* or *1*, optional): Set confclock to 1 to configure clock, which is necessary after a power cycle; default: `1`
 - `ClockFile` (*string*, required): the to clock configuration file; default `./../../misc/aida_tlu/aida_tlu_clk_config.txt`

Further expert settings for which the default value is used:
 - `nDUTs = 4`: number of DUT inputs (HDMI), leave 4 even if you only use fewer inputs
 - `nTrgIn = 6`: number of Trigger inputs (LEMO), leave 6 even if you only use fewer inputs
 - `I2C_COREEXP_Addr = 0x21`: I2C address of the bus expander on Enclustra FPGA
 - `I2C_CLK_Addr = 0x68`: I2C address of the Si5345
 - `I2C_DAC1_Addr = 0x13`: I2C address of 1st AD5665R
 - `I2C_DAC2_Addr = 0x1F`: I2C address of 2nd AD5665R
 - `I2C_ID_Addr = 0x50`: I2C address of unique Id number EEPROM
 - `I2C_EXP1_Addr = 0x74`: I2C address of 1st expander PCA9539PW
 - `I2C_EXP2_Addr = 0x75`: I2C address of 2nd expander PCA9539PW
 - `I2C_DACModule_Addr = 0x1C`: I2C address of AD5665R on powermodule
 - `I2C_EXP1Module_Addr = 0x76`: I2C address of 1st expander PCA9539PW on powermodule
 - `I2C_EXP2Module_Addr = 0x77`: I2C address of 2nd expander PCA9539PW on powermodule
 - `I2C_pwrId_Addr = 0x51`: I2C address of power
 - `I2C_disp_Addr = 0x3A`: I2C address of display
 - `intRefOn = 0`: 0 = False (Internal Reference OFF), 1 = True
 - `VRefInt = 2.5`
 - `VRefExt = 1.3`

### Parameter for configuration
To be set in the ```*.conf```-file for EUDAQ as section assuming the tag is ```euCliProducer -n AidaTluProducer -t aida_tlu```

The ```[Producer.aida_tlu]```-section has the following parameters:

General:
- `confid` (*string*, optional): track your changes, e.g. using the date; default `0`
- `verbose` (*0* or *1*, optional): enable verbose mode for debugging; default: `0`
- `skipconf` (*0* or *1*, optional): set skipini to 1, if you want to skip the following init-step: ; default `0`
- `delayStart` (*int* in milliseconds, optional): ause the TLU to allow slow devices to get ready after the euRunControl has issued the DoStart() command; default `0`

Configure DUT connections and HDMI lines or use the python script `misc/tools/dut_configuration_helper.py`:
- `DUTMask` (*hex*): one hex word for operating up to 4 DUT channels (4-bit mask: 1 = active, 0 = off); default `1`
- `DUTMaskMode` (*hex*): two hex word for setting the DUT mode (8-bit mask: AIDA/EUDET without triggerID = 11, EUDET with triggerID  = 00, AIDA with triggerID = 01); default `0xff`
- `DUTMaskModeModifier` (*hex*); default `0xff`
- `DUTIgnoreBusy` (*hex*): ignore a busy line from a DUT channel (4-bit mask: 1 = ignore, 0 = listen to busy, e.g. EUDET mode); default `0xf`
- `DUTIgnoreShutterVeto`: ; default `1`
- `HDMI1_set` (*bit*): 4-bit-mask to determine direction of HDMI pins, Mask: 0 CONT, 1 SPARE, 2 TRIG, 3 BUSY (1 = driven by TLU, 0 = driven by DUT); for example standard/EUDET mode: 7; default `0b0111`
- `HDMI2_set` (*bit*): see above; default `0b0111`
- `HDMI3_set` (*bit*): see above; default `0b0111`
- `HDMI4_set` (*bit*): see above; default `0b0111`
- `HDMI1_clk` (*0* or *1*): Clock source (0 = driven by DUT, 1 = Si5345 by TLU, 2 = FPGA by TLU); default `1`
- `HDMI2_clk` (*0* or *1*): see above; default `1`
- `HDMI3_clk` (*0* or *1*): see above; default `1`
- `HDMI4_clk` (*0* or *1*): see above; default `1`
- `LEMOclk` (*0* or *1*): Enable/Disable clock on differential LEMO; default `1`

Configure trigger logic:
- `InternalTriggerFreq`: ; default `0`
- `trigMaskHi`: ; default `0xFFFF`
- `trigMaskLo`: ; default `0xFFFE`

Configure trigger inputs:
- `DACThreshold0`: ; default `1.2`
- `DACThreshold1`: ; default `1.2`
- `DACThreshold2`: ; default `1.2`
- `DACThreshold3`: ; default `1.2`
- `DACThreshold4`: ; default `1.2`
- `DACThreshold5`: ; default `1.2`
- `in0_STR`: ; default `0`
- `in1_STR`: ; default `0`
- `in2_STR`: ; default `0`
- `in3_STR`: ; default `0`
- `in4_STR`: ; default `0`
- `in5_STR`: ; default `0`
- `in0_DEL`: ; default `0`
- `in1_DEL`: ; default `0`
- `in2_DEL`: ; default `0`
- `in3_DEL`: ; default `0`
- `in4_DEL`: ; default `0`
- `in5_DEL`: ; default `0`

Configure power board:
- `PMT1_V`: ; default `0.0`
- `PMT2_V`: ; default `0.0`
- `PMT3_V`: ; default `0.0`
- `PMT4_V`: ; default `0.0`

Configure shutter option on trigger input:
- `EnableShutterMode`: ; default `0`
- `ShutterSource`: ; default `0`
- `ShutterOnTime`: ; default `0`
- `ShutterOffTime`: ; default `0`
- `ShutterVetoOffTime`: ; default `0`
- `InternalShutterInterval`: ; default `0`
- `EnableRecordData`: ; default `1`


## EUDET TLU producer

### Dependencies

It is only built if cmake finds the external dependencies:
- tlufirmware (copy these in user/tlu/extern)
- ZestSC1 (copy these in user/tlu/extern)
- libusb (Ubuntu: ```sudo apt install libusb-dev```)
On Unix: Copy the [udev-rule](misc/eudet_tlu/54-tlu.rules) in the proper folder of the OS.

Please find here the hardware [manual](https://telescopes.desy.de/File:EUDET-MEMO-2009-04.pdf).

### Usage

With the command line tool ```EudetTluControl``` all functionalities can be tested.
To start the EUDET TLU producer: ```euCliProducer -n EudetTluProducer```
The usage with EUDET-type telescopes is described [here](https://telescopes.desy.de/User_manual#3._Starting_EUDAQ_NI_and_TLU_producer).

### Parameter for configuration

To be set in the ```*.conf```-file for EUDAQ as section assuming the tag is ```euCliProducer -n EudetTluProducer -t eudet_tlu```

The ```[Producer.eudet_tlu]```-section has the following parameters:
- `DutMask` (*bit*, required): Mask for enabling the 6 DUT connections; default `1`
- `EnableDUTVeto`: ; default `0`
- `HandShakeMode`: ; default `63`
- `TriggerInterval` (*int*, optional): Interval in milliseconds for internally generated triggers (0 = off and trigger logic) ; default `0`
- `AndMask`: ; default `0xff`
- `OrMask`: ; default `0`
- `StrobePeriod`: ; default `0`
- `StrobeWidth`: ; default `0`
- `VetoMask`: ; default `0`
- `TrigRollover`: ; default `0`
- `Timestamps`: ; default `1`
- `ErrorHandler`: ; default `2`
- `PMTVcntlX`
- `PMTIDX`
- `PMTGainErrorX`
- `PMTOffsetErrorX`
- `PMTVcntlMod`: ; default `0`
- `ReadoutDelay`: ; default `1000`
- `TimestampPerRun`: ; default `0`
- `DebugLevel`: ; default `0`
- `BitFile`: ; default `""`
- `Version`: ; default `0`
- `DUTInputX`: ; default 


## Conversion

The conversion of raw data containing **TluRawDataEvent** is the same for both types of TLU. For example and if LCIO is built, you can convert by: ```euCliConverter -i data.raw -o data.slcio```



