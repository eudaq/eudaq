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

  bool m_extShutter;

  map< string, int > m_dacs;

  vector< vector< int > > m_matrixDACs;
  vector< vector< bool > > m_matrixMask;

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
