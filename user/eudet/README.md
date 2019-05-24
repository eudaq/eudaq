# EUDET-type beam telescopes

Mimosa26 sensors with NI-based DAQ: Building, NiProducer and usage at Telescope with description of different DataCollectors, Executabes and Converters.

## Building

By default the cmake-flag is activated ```USER_EUDET_BUILD=ON```.
This builds the NiProducer, the converter modules for a standard event and for the LCIO data format, different DataCollectors (DirectSaveDataCollector, EventIDSyncDataCollector, TriggerIDSyncDataCollector), and executables for inspecting data and offline event building. There are no dedicated building dependencies.

## NI Producer

EUDAQ interface to LabView module for FlexRIO FPGA based Mimosa DAQ.  

### Usage

To start the Ni producer ```euCliProducer -n NiProducer```. To use the provided ini- and conf-file as example, the run time name is to be set when starting:  ```euCliProducer -n NiProducer -t ni_mimosa```. Find the application (starting scripts and conf-file) for EUDET-type telescope in [user/eudet/misc](../../user/eudet/misc)

### Parameter for intialization
To be set in the ```*.ini```-file for EUDAQ as section assuming the tag is ```euCliProducer -n NiProducer -t ni_mimosa```

The ```[Producer.ni_mimosa]```-section has the following parameters:
- `NiIPaddr`: here the IP address of the NI crate has to be set; default `localhost`
- `NiConfigSocketPort`: hardcoded port for LabView program; default `49248`
- `NiDataTransportSocketPort`: hardcoded port for LabView program; default `49250`


### Parameter for configuration
To be set in the ```*.conf```-file for EUDAQ as section assuming the tag is ```euCliProducer -n NiProducer -t ni_mimosa```

The ```[Producer.ni_mimosa]```-section has the following parameters:
- `NumBoards`: number of boards used; default `6`
- `MimosaEn_n`: n from 1 to 6: flag for enabling data aquisition of the corresponding plane; defaults `1`
- `MimosaID_n`: n from 1 to 6: IDs for Mimosa planes, not to be changed; default `0` to `5`
- `TriggerType`: not needed/known; default `1`
- `Det`: not needed/known; default `255`
- `NiVersion`: not needed/known; default `1`
- `FPGADownload`: not needed/known; default `1`

## Standard components of EUDET-type beam telescopes

- Mimosa Sensors and NI DAQ: EUDAQ2 code here in ```user/eudet```
- EUDET or AIDA TLU: EUDAQ2 code in ```user/tlu```
- StdEventMonitor which is the EUDAQ1 OnlineMon (```EUDAQ_BUILD_STDEVENT_MONITOR=ON```)
- Reference Plane, for example FEI4, USBPix / STControl: EUDAQ2 code which is currently working is here: ```https://github.com/beam-telescopes/USBpix/tree/release_5.3_eudaq20```

### Running modes

In brackets the exemplary start scripts are quoted which can be found in ```user/eudet/misc/start_scripts```; the EUDAQ ini and conf-files in ```user/eudet/misc/conf```.

#### Standard (EUDET) mode

The EUDET TLU (```01_eudet_tlu```) or the AIDA TLU (```01_aida_tlu```) in EUDET mode can be used.

EUDAQ1-like (global busy) data taking synchronising by Event ID:
- online: using one ```EventIDSyncDataCollector``` connected to all producer (```02_eudet_tlu_telescope``` or ```02_aida_tlu_telescope_eventID-DC```)
- offline: using multiple ```DirectSaveDataCollector``` each connected to one producer (```02_aida_tlu_telescope```) and merge them offline using ```euCliMergerStandardEvtID```

If the devices are reading out the Trigger ID, the synchronisation can also happen by this:
- online: using one ```TriggerIDSyncDataCollector``` connected to all producer (```02_aida_tlu_telescope_triggerID```)
- offline: using multiple ```DirectSaveDataCollector``` each connected to one producer (```02_aida_tlu_telescope```) and merge them offline using ```euCliMergerStandardTrigID```

#### Mixed mode

This can be used with the AIDA TLU and the the ```DUTIgnoreBusy``` option in order to allow multiple triggers within one Mimosa read out:

Mixed mode data taking synchronising by Trigger ID:
- online: using one ```TriggerIDSyncDataCollector``` connected to all producer (```02_aida_tlu_telescope_triggerID```)
- offline: using multiple ```DirectSaveDataCollector``` each connected to one producer (```02_aida_tlu_telescope```) and merge them offline using ```euCliMergerMixedCombinedTrigID``` or ```euCliMergerMixedDuplicateMimosaTrigID``` depending on the analysis.

#### AIDA mode

The synchronisation by timestamps will be developed and can be in usage if a (quasi)-continous read-out of the Mimosa data stream is available.
This can happen with a new Mimosa DAQ, like the MMC3 board which is currently under development: https://github.com/SiLab-Bonn/pymosa

### User Manual

Wiki-Pages for operating EUDET-type beam telescopes: https://telescopes.desy.de/User_manual

## Executable

Besides the above described executables for offline event building ("merger"), there is a helpful command-line reader for inspect data: `euCliTeleReader`.
This reader has the same parameters as the standard reader `euCliReader`, as `-i` for the path and the name of the raw-file, `-e` for the first event, `-E` for the last event, and others. The std out shows a table with run number, event number, trigger ID of TLU, timestamps of TLU, and the trigger ID and the Pivot pixel of the Ni/Mimosa event.

## Conversion

Raw data containing **NiRawDataEvent** can be converted to a StandardEvent which can be interpreted by the StdEventMonitor ("OnlineMonitor").
If LCIO is built, you can convert by: ```euCliConverter -i data.raw -o data.slcio``` to the LCIO data format.
