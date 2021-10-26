# Installation
1. Install EUDAQ following the instructions in main README file. Remember to enable EUDAQ_BUILD_PYTHON and USER_ITS3_BUILD!
2. Install alpide-daq-software (https://gitlab.cern.ch/alice-its3-wp3/alpide-daq-software.git) following the instructions in the respective README file.
3. `pip3 install urwid urwid_timed_progress`

# Configuration
In `misc` dir modify ITS3start.sh, ITS3.ini and *.conf files according to your setup.

# Running
`./ITS3start.sh && tmux a -t ITS3`

# Stopping
Either
`CTRL+b : kill-session` from tmux
or
`tmux kill-session -t ITS3`