#include <iostream>
#include <ostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <bitset>

#include "TList.h"
#include "TString.h"
#include "TObjArray.h"
#include "TObjString.h"
#include "TObject.h"
#include "TClass.h"

#define NPIXX 448
#define NPIXY 512

using namespace std;

class Timepix4Config {

public:

  Timepix4Config();
  void ReadXMLConfig( string fileName );


  void unpackGeneralConfig( int config );


private:


  bool m_extShutter;

};
