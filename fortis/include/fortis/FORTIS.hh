// put this next definition somewhere more sensible....
#define FORTIS_DATATYPE_NAME "FORTIS"
#define WORDS_IN_ROW_HEADER 2

// rate in Hz
#define FORTIS_FRAME_RATE 30

// rate in Hz. ( this is optimistic ... )
#define MAX_TLU_TRIGGER_RATE 3000

#define MAX_TRIGGERS_PER_FORTIS_FRAME (int)(MAX_TLU_TRIGGER_RATE/FORTIS_FRAME_RATE)

struct ExecutableArgs
{
  std::string dir;
  std::string filename;
  std::string killcommand;
  std::string args;

};

void* startExecutableThread(void * args);
