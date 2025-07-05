Python producer to steer a Keithley power supply (works for 2450 and 2470). 

Dependencies:
pyeudaq pyvisa-py numpy socket

Install EUDAQ with the `EUDAQ_BUILD_PYTHON` cmake flag.
```cmake .. -DEUDAQ_BUILD_PYTHON=ON```

Initialising connects to the sourcemeter and sets the compliance voltage. If specified, it also resets the sourcemeter - which allso sets the output voltage to zero, so that should be reserved for special cases.

Configuring reads the current voltage, then ramps to the target voltage (unless already on target)

During the run, voltage and current are printed with the chosen update frequency.

Init file parameters:
 * IPaddress (connection info - mandatory)
 * compliance (current limit in A)
 * voltageRange (e.g. 200 - if not specified, AUTO range will be used)
 * resetSourcemeter (if true, issue RST command)
 * biasLog (if specified, log current and voltage to this file during run)

Config file parameters
 * bias (target voltage - mandatory)
 * nsteps (numper of steps to make when ramping from current to target voltage)
 * steppause (how many seconds to wait between voltage steps)
 * precision (do not ramp if current/target voltage agree better than this)
 * update_every (seconds between measurements in run mode)


