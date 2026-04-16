/////////////////////////////////////////////////////////////////////
//                         2024 June 07                            //
//                   authors: Jordan Damgov                        //
//                email: Jordan.Damgov@ttu.edu                     //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#ifndef _DRS_EUDAQ_h
#define _DRS_EUDAQ_h

#include "CAENDigitizer.h"
#include <vector>
//#include "paramparser.h"
#include <map>
// shared structure
#include "FERS_EUDAQ_shm.h"


typedef struct
{
    uint32_t ChSize[MAX_X742_CHANNEL_SIZE]; // the number of samples stored in DataChannel array
    float    DataChannel[MAX_X742_CHANNEL_SIZE][1024]; // the array of ChSize samples
    uint32_t TriggerTimeTag;
    uint16_t StartIndexCell;
} CAEN_DGTZ_X742_GROUP_S_t;

typedef struct
{
    uint8_t                 GrPresent[MAX_X742_GROUP_SIZE]; // If the group has data the value is 1 otherwise is 0
    CAEN_DGTZ_X742_GROUP_S_t DataGroup[MAX_X742_GROUP_SIZE]; // the array of ChSize samples
} CAEN_DGTZ_X742_EVENT_S_t;



void make_header(int board, int DataQualifier, std::vector<uint8_t> *data);
// basic types of events
void DRSpack_event(void* Event, std::vector<uint8_t> *vec);
CAEN_DGTZ_X742_EVENT_t* DRSunpack_event(std::vector<uint8_t> *vec);
CAEN_DGTZ_X742_EVENT_S_t DRSunpack_event_S(std::vector<uint8_t> *vec);
void FreeDRSEvent(CAEN_DGTZ_X742_EVENT_t *event);

void FERSpack(int nbits, uint32_t input, std::vector<uint8_t> *vec);
uint16_t FERSunpack16(int index, const std::vector<uint8_t>& data);
uint32_t FERSunpack32(int index, const std::vector<uint8_t>& data);
uint64_t FERSunpack64(int index, const std::vector<uint8_t>& data);


#endif
