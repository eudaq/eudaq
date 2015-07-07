#include "Timepix3Config.h"
#include <iostream>

int main() {
 
  Timepix3Config* myTimepix3Config = new Timepix3Config();
  std::cout << myTimepix3Config << std::endl;
  myTimepix3Config->ReadXMLConfig( "/home/vertextb/SPIDR/software/trunk/python/ui/x.t3x" );

  return 0;
}
