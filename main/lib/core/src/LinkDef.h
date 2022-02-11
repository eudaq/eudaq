#ifndef __EudaqCoreDICT__
#define __EudaqCoreDICT__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winconsistent-missing-override"

#include "eudaq/Exception.hh"

#ifdef __CINT__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

#pragma link C++ namespace eudaq;
#pragma link C++ class eudaq::Exception - !;
#pragma link C++ class eudaq::LoggedException - !;

#pragma link C++ class eudaq::FileNotFoundException - !;
#pragma link C++ class eudaq::FileExistsException - !;
#pragma link C++ class eudaq::FileNotWritableException - !;
#pragma link C++ class eudaq::FileReadException - !;
#pragma link C++ class eudaq::FileWriteException - !;
#pragma link C++ class eudaq::FileFormatException - !;
#pragma link C++ class eudaq::SyncException - !;
#pragma link C++ class eudaq::CommunicationException - !;
#pragma link C++ class eudaq::BusError - !;

#endif
#endif
