
// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelBaseDetector.h"
#include "EUTelTLUDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelTLUDetector::EUTelTLUDetector() : EUTelBaseDetector() {
  _name = "TLU";
  // nothing else to do !
}

void EUTelTLUDetector::setAndMask(unsigned short value) { _andMask = value; }

void EUTelTLUDetector::setOrMask(unsigned short value) { _orMask = value; }

void EUTelTLUDetector::setVetoMask(unsigned short value) { _vetoMask = value; }

void EUTelTLUDetector::setDUTMask(unsigned short value) { _dutMask = value; }

void EUTelTLUDetector::setFirmwareID(unsigned short value) {
  _firmwareID = value;
}

void EUTelTLUDetector::setTimeInterval(short value) { _timeInterval = value; }

void EUTelTLUDetector::print(ostream &os) const {

  size_t w = 35;

  os << resetiosflags(ios::right) << setiosflags(ios::left) << setfill('.')
     << setw(w) << setiosflags(ios::left) << "Detector name "
     << resetiosflags(ios::left) << " " << _name << endl
     << setw(w) << setiosflags(ios::left) << "AndMask "
     << resetiosflags(ios::left) << " 0x" << to_hex(_andMask, 2) << endl
     << setw(w) << setiosflags(ios::left) << "OrMask "
     << resetiosflags(ios::left) << " 0x" << to_hex(_orMask, 2) << endl
     << setw(w) << setiosflags(ios::left) << "VetoMask "
     << resetiosflags(ios::left) << " 0x" << to_hex(_vetoMask, 2) << endl
     << setw(w) << setiosflags(ios::left) << "DUTMask "
     << resetiosflags(ios::left) << " 0x" << to_hex(_dutMask, 2) << endl
     << setw(w) << setiosflags(ios::left) << "FirmwareID "
     << resetiosflags(ios::left) << " " << _firmwareID << endl
     << setw(w) << setiosflags(ios::left) << "TimeInterval "
     << resetiosflags(ios::left) << " " << _timeInterval << setfill(' ')
     << endl;
}
