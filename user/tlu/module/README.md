### AidaTLUProducer

This module communicates with the AIDA-TLU. A complete documentation can be found [here](https://www.ohwr.org/project/fmc-mtlu/blob/master/Documentation/Main_TLU.pdf)

In the following, all initilization and configuration parameters are listed. Note that bools have to be defined as integers with 1 = true, all others false

In the following the user relevant parameters are listed



# initilization:

* `initid`: Define the TLU initilization ID, defaults to `0`
* `ConnectionFile`: File that defines the interface connections, no default.
* `DeviceName`: Device name, no default
* `skipini`: Define it the complete initilization should be skipped. Defaults to `0,false`
* `verbose`: Define if verbose messaging should be enabled. Defaults to `0`
* `CONFCLOCK`: Define if the clock chip should be configured. Needs to be done at leat once after power up. Defaults to `1`
* `CLOCK_CFG_FILE`: the file path to the clock chip configuration. Defaults to the file shipped with eudaq `./../user/eudet/misc/hw_conf/aida_tlu/fmctlu_clock_config.txt`

# configuration:

* `verbose`: Define if verbose messaging should be enabled. Defaults to `0`
* `delayStart`: Define the delay before starting a run. Defaults to `0`
* `skipconf`: Skip the config stage
* `HDMI<channel>_set`: Define the direction of the HDMI interface for channels 1..4 . The maskis as follows: : 0 CONT, 1 SPARE, 2 TRIG, 3 BUSY (1 = driven by TLU, 0 = driven by DUT). Defaults to `0x1`
* `HDMI<channel>_clk`: Define the clock direction for channels 1..4. 1 =driven by TLU, 0 by DUT. Defaults to `0`
* `LEMOclk`: Also use the LEMO clock output. Defaults to `1`
* `DACThreshold<channel>`: Define the threshold for the 6 LEMO trigger inputs 0..5. Range from 1.2V to -1.2V. Defauls to `1.2V`
*`trigPol`*: 6 bit mask to select the edge to trigger on 1 == falling edge, 0== rising edge. Defaults to `0x3F` all falling
*`in<channel>_STR`: Stretch of the digitized input signals of channels 0..5 in 6.25ns units. Defaults to `0`
*`in<channel>_DEL`: Delay of the digitized input signals of channels 0..5 in 6.25ns units. Defaults to `0`
* `trigMaskHi` and `trigMaskLow`: Define the valied trigger inputs. Refer to TLU manual for details. Defaults to `0xFFFF` and `0xFFFE`
* `PMT<channel>_V`: Set the volatge for the PMTs 1..4. Defaults to `0`
* `DUTMask`: Bit mask to selected the active interfaces 1==on, 0 ==off. Defaults to `1`
* `DUTMaskMode`: Bit mask to select the interface per channel. 2 bits each. 0b00==EUDET mode, 0b01==AIDAMODEwithtrigger, 0b11==ADIA mode. Channel indexing: <3><2><1><0>. Defaults to 0xFF
* `InternalTriggerFreq`: Define the frequency of automatically generated triggers. Defaults to `0`
* `EnableRecordData`: Enable rcoding data. Defaults to `1` and should typically not be changed.
* `EUDAQ_DC`: Data collector to sent data towards.
* `DUTIgnoreBusy`: Mask to ignore the busy for DUT channels 0..3. If set to `1` other channels receive further triggers even if the channel is busy. Defaults to `0xF`
* `DUTIgnoreShutterVeto`: Ignore shutter features. Defaults to 1.
* Several shutter settings for linear collider modes that are not used at the test beam right now afaik