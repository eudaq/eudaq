# Timepix3 Module for EUDAQ2

This module allows to integrate Timepix3 sensors running with the SPIDR readout system into the EUDAQ2 eco system.

Currently, this producer is in early development stage and has only been tested in the AIDA mode, i.e. receiving a global clock and a T0 signal at beginning of the run.
The producer interfaces the SPIDR device manager to add devices and to control them. The following actions are performed in the different FSM stages:

* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n Timepix3Producer -t Timepix3 -r <tcp>
    ```

    will ask SPIDR to instantiate a Timepix3 device. Device names are case sensitive and have to be available in the linked SPIDR installation.

* **Configuration**: During configuration, the device is powered using SPIDR's `powerOn()` command. After this, the producer waits for one second in order to allow the AIDA TLU to fully configure and make the clock available on the DUT outputs. Then, the `configure()` command of the SPIDR device interface is called.

* **DAQ Start / Stop**: During DAQ start and stop, the corresponding SPIDR device functions are called.

Since the SPIDR device libraries are not thread-safe, all access to SPIDR libraries is guarded using C++11 `std::lock_guard` with a central class-member mutex to avoid concurrent device access.


## Data Converters to StandardEvent

### Timepix3RawEvent2StdEventConverter

The following parameters can be passed in the configuration in order to influence the decoding behavior of this module:

* `delta_t_t0`: Integer in nanoseconds to determine the criterion for the indirect T0 detection. If the Timepix3 timestamps jump back by more than this value, a 2nd T0 is assumed to have been recorded. The value needs to be passed without units, but a syntax such as `1e3` is supported. Defaults to `1e6`.

### Timepix3TrigEvent2StdEventConverter

The following parameters can be passed...
