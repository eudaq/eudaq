#include "tlu/TLUController.hh"

#ifdef WIN32
ZESTSC1_ERROR_FUNC ZestSC1_ErrorHandler=NULL;  // set to NULL so that this function will not be called. it seems that this is only requiered on WINDOWS
char *ZestSC1_ErrorStrings[]={"dummy","dummy1"}; // needs to have some dummy strings but for now i dont know where they will be used again. 
#endif

int main() {
  tlu::TLUController tlu;
  tlu.ResetUSB();
  
  return 0;
}
