# Timepix4 Module for EUDAQ2

This module allows to integrate a Timepix4 sensor running with the SPIDR4 readout system into the EUDAQ2 eco system.

Currently the producer works by connecting to a running tpx4sc from the tpx4tools provided by Nikhef as this is the easiest way to implement. The producer will simply send commands to the tpx4sc to perform the configuration etc. to avoid rewriting the entire communication of the tpx4sc.


* **Initialisation**: The device to be instantiated is taken from the name of the producer (added with `-t <name>` on the command line). This means, the following command

    ```
    $ euCliProducer -n Timepix4Producer -t tpx4 -r <tcp>
    ```
    Initialization connects to as a client socket to the tpx4sc host socket via it's specified port.
    

* **Configuration**: TODO

* **DAQ Start / Stop**: TODO

The tpx4tools have no library that can be linked against. Not sure if in the future I will require it to find the tpx4sc or if like right now I set no requirements.

No parameters.
