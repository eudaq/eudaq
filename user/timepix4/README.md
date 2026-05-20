# Timepix4 Module for EUDAQ2

This module allows to integrate a Timepix4 sensor running with the SPIDR4 readout system into the EUDAQ2 eco system.

Currently the producer works by connecting to a running tpx4sc from the tpx4tools provided by Nikhef as this is the easiest way to implement. The producer will simply send commands to the tpx4sc to perform the configuration etc. to avoid rewriting the entire communication of the tpx4sc.

To use an external t0 tpx4sc must be initialized using "-T" as an option. For an external shutter the option "-E" must be used.


* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n Timepix4Producer -t tpx4 -r <tcp>
    ```
    Initialization connects to as a client socket to the tpx4sc host socket via it's specified port.
    Options are "SPIDR_IP" for the IP address (Default: 192.168.1.10) of the SPIDR4 slow control and "SPIDR_Port" for the port of the spidr slow control (Default: 50051).
    

* **Configuration**: Currently allows for a setting of the Threshold using "threshold" (Default: 1000) and whether an external shutter should be used. It would then use it to only allow the tpx4 to take data using a shutter signal.

* **DAQ Start / Stop**: Sends a start and stop run to the system.
