# Watchdog configuration

# array: email or phone numbers
ALERT_TARGET = ( "jgrosseo@cern.ch" )
#ALERT_TARGET = ( "jgrosseo@cern.ch", "41764875459@mail2sms.cern.ch" )

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