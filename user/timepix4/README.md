# Timepix4 Module for EUDAQ2

This module allows to integrate a Timepix4 sensor running with the SPIDR readout system into the EUDAQ2 eco system.

Currently, this producer is in early development stage and has only been tested in the AIDA mode, i.e. receiving a global clock and a T0 signal at beginning of the run.
The producer interfaces the SPIDR DAQ software via a `SpidrController` to control the Timepix3. The following actions are performed in the different FSM stages:

* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n Timepix4Producer -t Timepix4 -r <tcp>
    ```

    will ask the `SpidrController` to establish a network connection, retrieve software and firmware versions, and determines the number of connected devices.

* **Configuration**: Before configuration of the device, the producer waits for one second in order to allow the AIDA TLU to fully configure and make the clock available on the DUT outputs. Then, the device is configured.

* **DAQ Start / Stop**: During DAQ start and stop, the corresponding SPIDR device functions are called.

Since the SPIDR device libraries are not thread-safe, all access to SPIDR libraries is guarded using C++11 `std::lock_guard` with a central class-member mutex to avoid concurrent device access.

No parameters.
