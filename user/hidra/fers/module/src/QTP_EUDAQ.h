/////////////////////////////////////////////////////////////////////
//                         2024 June 07                            //
//                   authors: Jordan Damgov                        //
//                email: Jordan.Damgov@ttu.edu                     //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#ifndef _QTP_EUDAQ_h
#define _QTP_EUDAQ_h

#include "CAENDigitizer.h"
#include <vector>
//#include "paramparser.h"
#include <map>
// shared structure
#include "FERS_EUDAQ_shm.h"

void make_header(int board, int DataQualifier, std::vector<uint8_t> *data);
// basic types of events
void QTPpack_event(uint16_t ADCdata[32], std::vector<uint8_t> *vec);
void QTPunpack_event(std::vector<uint8_t> *vec, uint16_t (&ADCdata)[32]);

void FERSpack(int nbits, uint32_t input, std::vector<uint8_t> *vec);
uint16_t FERSunpack16(int index, const std::vector<uint8_t>& data);
uint32_t FERSunpack32(int index, const std::vector<uint8_t>& data);
uint64_t FERSunpack64(int index, const std::vector<uint8_t>& data);


#endif
