// put this next definition somewhere more sensible....
#define FORTIS_DATATYPE_NAME "FORTIS"
#define WORDS_IN_ROW_HEADER 2


struct ExecutableArgs
{
  std::string dir;
  std::string filename;
  std::string killcommand;
  std::string args;

};

void* startExecutableThread(void * args);
