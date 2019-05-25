# IP variables for start scripts
export RUNCONTROLIP=127.0.0.1
export TLUIP=127.0.0.1
export NIIP=127.0.0.1

echo "Check IPs and files pathes in telescope.ini and in the conf-files"

# eudaq2 binaries
export EUDAQ=/opt/eudaq2/
export PATH=$PATH:$EUDAQ/bin

echo "Choose a default starting_script to start EUDAQ."
