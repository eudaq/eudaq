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

* **Configuration**: During configuration, the device is powered using Peary's `powerOn()` command. After this, the producer waits for one second in order to allow the AIDA TLU to fully configure and make the clock available on the DUT outputs. Then, the `configure()` command of the Peary device interface is called. In addition, the following register names are looked for in the configuration and sent to the device:

    * `threshold`

* **DAQ Start / Stop**: During DAQ start and stop, the corresponding Peary device functions `daqStart()` and `daqStop()` are called.

Since the Peary device libraries are not thread-safe, all access to Peary libraries is guarded using C++11 `std::lock_guard` with a central class-member mutex to avoid concurrent device access. The Peary device manager itself ensures that no two instances are executed on the same hardware. This means, only one EUDAQ2 producer can be run on each Caribou board.
