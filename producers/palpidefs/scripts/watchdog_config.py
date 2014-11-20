# Watchdog configuration

# comma separated list: email or phone numbers e.g. 41764875459@mail2sms.cern.ch (does not seem to work with cernmx, to be debugged)
ALERT_TARGET = "jgrosseo@cern.ch"

# time in seconds between problem occured and alert sent
ALERT_GRACE_TIME = 120

# repetition time of alerts in seconds
ALERT_REPEAT_TIME = 3600

# time in seconds allowed since the raw data file has been touched last
LAST_EVENT_TIME = 60

# maximum allowed file size in bytes for raw data file
FILE_SIZE_LIMIT = 2 * 1000 * 1000 * 1000

# time period in which out of syncs are analyzed 
OUT_OF_SYNC_TIME = 60

# number of allowed out of syncs in the given time period
OUT_OF_SYNC_LIMIT = 100