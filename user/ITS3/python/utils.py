import pyeudaq
import logging
import random
import datetime
from timeout_decorator import timeout
from random_word import RandomWords
from quote import quote

def exception_handler(method):
    def inner(*args, **kwargs):
        try:
            return method(*args, **kwargs)
        except Exception as e:
            pyeudaq.EUDAQ_ERROR(f"Exception in {method.__name__}: {e}")
            raise e
    return inner

class LoggingToEUDAQ:
    def __init__(self, eudaq_log=None):
        self.eudaq_log = eudaq_log
    
    def write(self, msg):
        if self.eudaq_log is not None: # user specified
            self.eudaq_log(msg.strip())
        elif msg.startswith("DEBUG"): # redirect logging to corresponding levels
            pyeudaq.EUDAQ_DEBUG(msg[6:].strip())
        elif msg.startswith("INFO"):
            pyeudaq.EUDAQ_INFO(msg[5:].strip())
        elif msg.startswith("WARNING"):
            pyeudaq.EUDAQ_WARN(msg[8:].strip())
        elif msg.startswith("ERROR"):
            pyeudaq.EUDAQ_ERROR(msg[6:].strip())
        elif msg.startswith("CRITICAL"):
            pyeudaq.EUDAQ_ERROR(msg[9:].strip())
        else:
            pyeudaq.EUDAQ_DEBUG(msg.strip())

    def flush(self):
        pass

def logging_to_eudaq(level=logging.DEBUG):
    logging.basicConfig(level=level,stream=LoggingToEUDAQ(),format='%(levelname)s: %(message)s')

def str2int(string: str) -> int:
    if "0x" in string:
        return int(string,16)
    elif "0b" in string:
        return int(string,2)
    else:
        return int(string)

def easter_egg(d: int | None = None) -> str | None:
    """It all started as an Easter egg on 31/03/2024..."""
    try:
        if d is None:
            d = datetime.date.today().day
        e = "🐣🐬🦆🍄🎊🌋🤡😎👠💀💩👾🌕⏳🐸😎👠🙊🌵🍆"
        return e[d%len(e)]*random.randint(1, 21)
    except:
        return None

@timeout(3, use_signals=False)
def get_quote() -> dict[str, str]:
    return quote(RandomWords().get_random_word(), 1)[0]

def get_tip() -> str:
    tip = TIPS[random.randint(0, len(TIPS)-1)]
    tip += "\nFor more info see: https://twiki.cern.ch/twiki/bin/view/ALICE/ITS3WP3TBDAQManual"
    return tip

def get_quote_or_tip(quote_chance=0.8) -> str:
    r = random.uniform(0,1)
    if r < quote_chance:
        try:
            quote = get_quote()
            book = f', "{quote["book"]}"' if quote["book"] else ""
            ret = quote["quote"]+ "\n" + " "*20 + "- " + quote["author"]+book
        except:
            ret = get_tip()
    else:
        ret = get_tip()
    return ret

TIPS = [
    "Before and after every run set add information to eLog about the beam energy, particle species and all the accessible parameters of the beam (collimators...). Add it at least twice a day, every time it changes, and after every beam stop.",
    "At least twice a day transfer all the data to EOS. Include the config files.",
    "Keep a Run List and update it as the measurements are ongoing. See below for all the fields Run List must include.",
    "Designate the person (per setup) responsible for checking every run with online monitor. Is the data as you'd expect? Add the screenshots to the eLog.",
    "Designate a person to run corry analysis, at least on a sample basis (few runs per dataset). Is the data as you'd expect? Add the screenshots to the eLog.",
    "Every time the setup is changed (e.g. DUT change), take photo of the setup, what goes in, what comes out and add it to the eLog.",
    "For every alignment, add a logbook entry with all the information needed to understand what you did and why you did it.",
    "Every time you start data taking with new set of config files, add a logbook entry.",
    "Add an Issue report to eLog every time there is an issue.",
    "Add an eLog entry on the data taking status at least every 3 hours (when present in the TB area). If there is nothing to report, write only that.",
    "Write down trigger rates (on all triggering devices - per spill and per second)",
]

# keepin this for future reference TODO implement or remove
# class StreamToEUDAQ:
#     def __init__(self, eudaq_log=pyeudaq.EUDAQ_DEBUG):
#         self.buffer = ''
#         self.eudaq_log = eudaq_log

#     def write(self, buf):
#         self.buffer += buf
#         lines = self.buffer.splitlines(True)
#         for line in lines[0:-1]:
#             self.eudaq_log(line)
#         if lines[-1][:-2] == '\n':
#             self.eudaq_log(lines[-1])
#         else:
#             self.buffer = lines[-1]

#     def flush(self):
#         if self.buffer:
#             self.eudaq_log(self.buffer)
#             self.buffer = ''

# def redirect_stdout_to_eudaq():
#     sys.stdout = StreamToEUDAQ()

