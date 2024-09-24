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

#define NPIXX 256
#define NPIXY 256

using namespace std;

class Timepix4Config {

public:

  Timepix4Config();
  void ReadXMLConfig( string fileName );

  const char* getTime() { return m_time; };
  const char* getUser() { return m_user; };
  const char* getHost() { return m_host; };

  map< string, int > getDeviceDACs() { return m_dacs; };
  vector< vector< int > > getMatrixDACs() { return m_matrixDACs; };
  vector< vector< bool > > getMatrixMask() { return m_matrixMask; };

  void unpackGeneralConfig( int config );

  string getChipID( int deviceID );

private:

  const char* m_time;
  const char* m_user;
  const char* m_host;

  map< string, int > m_dacs;

  vector< vector< int > > m_matrixDACs;
  vector< vector< bool > > m_matrixMask;

};

const map< string, int > TPX4_DAC_CODES = {
  { "IBIAS_PREAMP_ON"  , TPX4_IBIAS_PREAMP_ON },
  { "IBIAS_PREAMP_ON"  , TPX4_IBIAS_PREAMP_ON },
  { "IBIAS_PREAMP_OFF" , TPX4_IBIAS_PREAMP_OFF },
  { "VPREAMP_NCAS"     , TPX4_VPREAMP_NCAS },
  { "IBIAS_IKRUM"      , TPX4_IBIAS_IKRUM },
  { "VFBK"             , TPX4_VFBK },
  { "IBIAS_DISCS1_ON"  , TPX4_IBIAS_DISCS1_ON },
  { "IBIAS_DISCS1_OFF" , TPX4_IBIAS_DISCS1_OFF },
  { "IBIAS_DISCS2_ON"  , TPX4_IBIAS_DISCS2_ON },
  { "IBIAS_DISCS2_OFF" , TPX4_IBIAS_DISCS2_OFF },
  { "IBIAS_PIXELDAC"   , TPX4_IBIAS_PIXELDAC },
  { "IBIAS_TPBUFIN"    , TPX4_IBIAS_TPBUFIN },
  { "IBIAS_TPBUFOUT"   , TPX4_IBIAS_TPBUFOUT },
  { "VTP_COARSE"       , TPX4_VTP_COARSE },
  { "VTP_FINE"         , TPX4_VTP_FINE },
};

inline vector<TString> tokenise( TString line, const char* delim=" " ) {
  vector<TString> retvec;
  line = line.Strip();
  TObjArray* tokens = line.Tokenize( delim );
  for (int i=0; i<tokens->GetSize(); i++) {
    if (tokens->At(i) != nullptr) {
      TString t = dynamic_cast<TObjString *>(tokens->At(i))->GetString();
      retvec.push_back( t );
    }
  }
  return retvec;
}

template <typename T>
inline void pack (std::vector< unsigned char >& dst, T& data) {
  unsigned char * src = static_cast < unsigned char* >(static_cast < void * >(&data));
  dst.insert (dst.end (), src, src + sizeof (T));
}

template <typename T>
inline void unpack (vector <unsigned char >& src, int index, T& data) {
  copy (&src[index], &src[index + sizeof (T)], &data);
}

inline void my_handler( int s ) {

  cout << "\nCaught signal " << s << endl;
  cout << "Terminating Timepix4Producer" << endl;
  exit( 1 );

}
