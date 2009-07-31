#ifndef EUDAQ_INCLUDED_Info
#define EUDAQ_INCLUDED_Info

/*
 * This file contains information about which capabilities this version of EUDAQ supports,
 * to enable users of the library to conditionally enable functionality.
 */

// The Info.hh file itself exists
#define EUDAQ_INFO_FILE 1

// The DEPFET decoder is implemented
#define EUDAQ_DEPFET_DECODER 1

// The new plugin decoder mechanism is implemented
#define EUDAQ_NEW_DECODER

#endif // EUDAQ_INCLUDED_Info
