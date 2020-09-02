# Caribou Module for EUDAQ2

This module allows to integrate devices running with the Caribou readout system into the EUDAQ2 eco system.

Currently, this producer is in early development stage and has only been tested in the AIDA mode, i.e. receiving a global clock and a T0 signal at beginning of the run.
The producer interfaces the Peary device manager to add devices and to control them. The following actions are performed in the different FSM stages:

* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n CaribouProducer -t CLICpix2 -r <tcp>
    ```

    will ask Peary to instantiate a device of type `CLICpix2`. Device names are case sensitive and have to be available in the linked Peary installation.
    In addition, the following configuration keys are available for the initialisation:

    * `log_level`: Set the Peary-internal logging verbosity for output on the terminal of the producer. Please refer to the Peary documentation for more information
    * `config_file`: Configuration file in Peary-format to be passed to the device. It should be noted, that the file access will happen locally by the producer, i.e. the value has to point to a file locally available to the producer on the Caribou system.

* **Configuration**: During configuration, the device is powered using Peary's `powerOn()` command. After this, the producer waits for one second in order to allow the AIDA TLU to fully configure and make the clock available on the DUT outputs. Then, the `configure()` command of the Peary device interface is called.

    In addition, a register value can be overwritten at the configure stage by setting the name and value in the EUDAQ configuration file, e.g.

    ```toml
    register_key = "threshold"
    register_value = 123
    ```

    Both keys need to be set, of one or the other is missing, no change of configuration is attempted.

    The producer itself reads the value `number_of_subevents` which allows buffering. If set to a value larger than zero, events are first buffered and upon reaching the desired buffer depth, they are collectively sent as sub-events of a Caribou event of the same type.
    A value of, for example, `number_of_subevents = 100` would therefore result in one event being sent to the DataCollector every 100 events read from the device. This event would contain 100 sub-events with the individual data blocks. This is completely transparent to data analysis performed in Corryvreckan using the EventLoaderEUDAQ2 and can be used to reduce the number of packets sent via the network.

* **DAQ Start / Stop**: During DAQ start and stop, the corresponding Peary device functions `daqStart()` and `daqStop()` are called.

Since the Peary device libraries are not thread-safe, all access to Peary libraries is guarded using C++11 `std::lock_guard` with a central class-member mutex to avoid concurrent device access. The Peary device manager itself ensures that no two instances are executed on the same hardware. This means, only one EUDAQ2 producer can be run on each Caribou board.


## Data Converters to StandardEvent

### CLICTDEvent2StdEventConverter

The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `countingmode`: Boolean for configuring the frame decoder to indicate counting mode data. Defaults to `true`.
* `longcnt`: Boolean for configuring the frame decoder to interpret the pixel value as single long 13-bit counter. Defaults to `false`.
* `discard_tot_below`: Integer value to discard pixels with certain ToT values. All pixels with a ToT below this value will be discarded immediately and not returned by the decoder. This only works when a ToT value is available, i.e. not when `longcnt` is enabled. If a pixel has a ToT value of 0 and this setting is `1`, the hit will be discarded - if it is set to `0`, the hit is kept. Defaults to `-1`, i.e. no pixels are discarded.
* `discard_toa_below`: Integer value to discard pixels with certain ToA values. All pixels with a ToA below this value will be discarded immediately and not returned by the decoder. This only works when a ToA value is available, i.e. not with `countingmode` enabled. If a pixel has a ToA value of 0 and this setting is `1`, the hit will be discarded - if it is set to `0`, the hit is kept. Defaults to `-1`, i.e. no pixels are discarded.
* `pixel_value_toa`: Boolean to select which value to return as pixel raw value. Can bei either `0` for ToT or `1` for ToA, defaults to `0`.

### CLICpix2Event2StdEventConverter

The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `countingmode`: Boolean for configuring the frame decoder to indicate counting mode data. Defaults to `true`.
* `longcnt`: Boolean for configuring the frame decoder to interpret the pixel value as single long 13-bit counter. Defaults to `false`.
* `comp`: Boolean to switch on/off the pixel compression of CLICpix2 for interpreting the frame data. Defaults to `true`.
* `sp_comp`: Boolean to switch on/off the super-pixel compression of CLICpix2 for interpreting the frame data. Defaults to `true`.
* `discard_tot_below`: Integer value to discard pixels with certain ToT values. All pixels with a ToT below this value will be discarded immediately and not returned by the decoder. This only works when a ToT value is available, i.e. not when `longcnt` is enabled. If a pixel has a ToT value of 0 and this setting is `1`, the hit will be discarded - if it is set to `0`, the hit is kept. Defaults to `-1`, i.e. no pixels are discarded.
* `discard_toa_below`: Integer value to discard pixels with certain ToA values. All pixels with a ToA below this value will be discarded immediately and not returned by the decoder. This only works when a ToA value is available, i.e. not with `countingmode` enabled. If a pixel has a ToA value of 0 and this setting is `1`, the hit will be discarded - if it is set to `0`, the hit is kept. Defaults to `-1`, i.e. no pixels are discarded.

### ATLASPixEvent2StdEventConverter

The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `clkdivend2`: Value of clkdivend2 register in ATLASPix specifying the speed of TS2 counter. Default is `7`.
* `clock_cycle`:  Clock period of the hit timestamps, defaults to `8` (ns). The value needs to be provided in `ns`.
