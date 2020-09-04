# Timepix3 Module for EUDAQ2

This module allows to integrate a Timepix3 sensor running with the SPIDR readout system into the EUDAQ2 eco system.
In addition, the fast ADC input on the SPIDR board can be used as a SPIDRTrigger auxiliary device.

Currently, this producer is in early development stage and has only been tested in the AIDA mode, i.e. receiving a global clock and a T0 signal at beginning of the run.
The producer interfaces the SPIDR DAQ software via a `SpidrController` to control the Timepix3. The following actions are performed in the different FSM stages:

* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n Timepix3Producer -t Timepix3 -r <tcp>
    ```

    will ask the `SpidrController` to establish a network connection, retrieve software and firmware versions, and determines the number of connected devices.

* **Configuration**: Before configuration of the device, the producer waits for one second in order to allow the AIDA TLU to fully configure and make the clock available on the DUT outputs. Then, the device is configured.

* **DAQ Start / Stop**: During DAQ start and stop, the corresponding SPIDR device functions are called.

Since the SPIDR device libraries are not thread-safe, all access to SPIDR libraries is guarded using C++11 `std::lock_guard` with a central class-member mutex to avoid concurrent device access.


## Data Converters to StandardEvent

### Timepix3RawEvent2StdEventConverter

The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `delta_t0`: Integer in microseconds as the criterion for the indirect T0 detection. If the Timepix3 timestamps jump back by more than this value, a 2nd T0 is assumed to have been recorded. The value needs to be passed as an integer without units, but a syntax such as `1e3` is supported. Defaults to `1e6` (corresponding to 1s).
* `calibration_path_tot`: Path to ToT calibration file. If this parameter is set, a conversion of the pixel time-over-threshold values to charge is applied. The file format needs to be `col | row | row | a (ADC/mV) | b (ADC) | c (ADC*mV) | t (mV) | chi2/ndf`.
* `calibration_path_toa`: Path to ToA calibration file. If this parameter is set, a timewalk correction is applied to each pixel timestamp. The file format needs to be `column | row | c (ns*mV) | t (mV) | d (ns) | chi2/ndf`.

### Timepix3TrigEvent2StdEventConverter

No parameters.
