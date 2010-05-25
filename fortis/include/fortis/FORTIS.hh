// put this next definition somewhere more sensible....
#define FORTIS_DATATYPE_NAME "FORTIS"
#define WORDS_IN_ROW_HEADER 2

#define FORTIS_MAXROW 0x00FF

// the number of frames to wait after "stopping" flag is set true before haling run.
#define FORTIS_NUM_EOR_FRAMES 10

// rate in Hz
#define FORTIS_FRAME_RATE 30

// rate in Hz. ( this is optimistic ... )
#define MAX_TLU_TRIGGER_RATE 3000

#define MAX_TRIGGERS_PER_FORTIS_FRAME (int)(MAX_TLU_TRIGGER_RATE/FORTIS_FRAME_RATE)

// define debugging flags
#define FORTIS_DEBUG_FRAMEREAD 0x0001
#define FORTIS_DEBUG_EVENTSEND 0x0002
#define FORTIS_DEBUG_TRIGGERWORDS 0x0004
#define FORTIS_DEBUG_TRIGGERS 0x0008
#define FORTIS_DEBUG_PROCESSLOOP 0x0010
#define FORTIS_DEBUG_PRINTRAW 0x0020
#define FORTIS_DEBUG_PRINTRECOVERY 0x0040

struct ExecutableArgs
{
  std::string dir;
  std::string filename;
  std::string killcommand;
  std::string args;

};

void* startExecutableThread(void * args);
