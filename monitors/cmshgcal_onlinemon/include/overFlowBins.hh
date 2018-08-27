#ifndef Overflow_HH_
#define Overflow_HH_

#include <TH1.h>

void handleOverflowBins(TH1* hist, bool doOver=true, bool doUnder=false, bool doErr=false);

#endif /* Overflow_HH_ */
