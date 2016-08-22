Raspberry Pi Controller
=====

controller module for EUDAQ which allows using the GPIO pins of a RaspberryPi for various actions in your test beam setup.

### Description

This module inherits from the EUDAQ Controller class which inhertis from CommandReceiver but does not connect to a DataCollector and does not provide output data.
The controller uses the `wiringPi` library to initialize the RPi's GPIO interface and set the pins.


### Requirements

The 'wiringPi' library is required. Since Raspbian Jessy, it is available as a system library and can be installed via
```
sudo apt-get install wiringpi
```

IMPORTANT: the RPiController must be executed with root privileges! Without, the wiringPi library has no access to the GPIO interface. To make things worse, the library simply pulls the `exit()` command when executed without root instead of throwing an exception or returning some error code which makes it nearly impossible to catch upstream in the controller module - it will simply quit...


### Usage

Set the following parameters in your config file:

```
[Controller.RPiController]
pinnr = 13
waiting_time = 4000
```

where the first parameter is the GPIO pin number to be used and the second is the waiting time before the action is executed.

### Status of the module

Currently only one pin action is supported and executed after a waiting time at run start.
Future plans include to provide a list of pins to be used together with a set of actions (either set high or set low) at different states of the FSM (OnRunStart, OnRunStop OnConfigure, OnTerminate).