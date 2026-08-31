/////////////////////////////////////////////////////////////////////
//                         2023 May 08                             //
//                   authors: F. Tortorici                         //
//                email: francesco.tortorici@ct.infn.it            //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#ifndef _FERS_EUDAQ_h
#define _FERS_EUDAQ_h

#include <vector>
//#include "paramparser.h"
#include <map>

#include "FERS_EUDAQ_shm.h"


extern std::fstream runfile[MAX_NBRD]; // pointers to ascii output data files

#define DTQ_STAIRCASE 10
// staircase datatype
typedef struct {
	uint16_t threshold;
	uint16_t shapingt; // enum, see FERS_Registers.h
	uint32_t dwell_time; // in seconds, divide hitcnt by this to get rate
	uint32_t chmean; // over channels, no division by time
	uint32_t HV; // 1000 * HV from HV_Get_Vbias( handle, &HV);
	uint32_t Tor_cnt;
	uint32_t Qor_cnt;
	uint32_t hitcnt[FERSLIB_MAX_NCH_5202];
} StaircaseEvent_t;

// known types of event (for event length checks for instance in Monitor)
static const std::map<uint8_t, int> event_lengths =
{
{ DTQ_SPECT     , sizeof(SpectEvent_t)    },
{ DTQ_TIMING    , sizeof(ListEvent_t)     },
{ DTQ_COUNT     , sizeof(CountingEvent_t) },
{ DTQ_WAVE      , sizeof(WaveEvent_t)     },
{ DTQ_TSPECT    , sizeof(SpectEvent_t)    },
{ DTQ_TEST      , sizeof(TestEvent_t)     },
{ DTQ_STAIRCASE , sizeof(StaircaseEvent_t)}
};                                
                                  
                                  
//////////////////////////        
// use this to pack every kind of  event
void FERSpackevent(void* Event, int dataqualifier, std::vector<uint8_t> *vec);
//////////////////////////        
                                  
// for each kind of event: the "pack" is used by FERSpackevent,
// whereas the "unpack" is meant sto be used individually

// basic types of events 
void FERSpack_spectevent(void* Event, std::vector<uint8_t> *vec);
SpectEvent_t FERSunpack_spectevent(std::vector<uint8_t> *vec);

void FERSpack_listevent(void* Event, std::vector<uint8_t> *vec);
ListEvent_t FERSunpack_listevent(std::vector<uint8_t> *vec);

void FERSpack_tspectevent(void* Event, std::vector<uint8_t> *vec);
SpectEvent_t FERSunpack_tspectevent(std::vector<uint8_t> *vec);

void FERSpack_countevent(void* Event, std::vector<uint8_t> *vec);
CountingEvent_t FERSunpack_countevent(std::vector<uint8_t> *vec);

void FERSpack_waveevent(void* Event, std::vector<uint8_t> *vec);
WaveEvent_t FERSunpack_waveevent(std::vector<uint8_t> *vec);

void FERSpack_testevent(void* Event, std::vector<uint8_t> *vec);
TestEvent_t FERSunpack_testevent(std::vector<uint8_t> *vec);

// advanced
void FERSpack_staircaseevent(void* Event, std::vector<uint8_t> *vec);
StaircaseEvent_t FERSunpack_staircaseevent(std::vector<uint8_t> *vec);

/////////////////

// utilities used by the above methods

// fill "data" with some info
void make_header(int board, int DataQualifier, std::vector<uint8_t> *data);
void make_headerFERS(int board, int PID, float HV, float Isipm, float tempDET, float tempFPGA, std::vector<uint8_t> *data);

// reads back essential header info (see params)
// prints them w/ board ID info with EUDAQ_WARN
// returns index at which raw data starts
int read_header(std::vector<uint8_t> *data, int *board, int *PID);
int read_headerFERS(std::vector<uint8_t> *vec, int *board, int *PID, float *HV, float *Isipm, float *tempDET, float *tempFPGA);

//void dump_vec(std::string title, std::vector<uint8_t> *vec, int start=0, int stop=0);

void FERSpack(int nbits, uint32_t input, std::vector<uint8_t> *vec);
//uint16_t FERSunpack16(int index, std::vector<uint8_t> vec);
//uint32_t FERSunpack32(int index, std::vector<uint8_t> vec);
//uint64_t FERSunpack64(int index, std::vector<uint8_t> vec);

uint16_t FERSunpack16(int index, const std::vector<uint8_t>& data);
uint32_t FERSunpack32(int index, const std::vector<uint8_t>& data);
uint64_t FERSunpack64(int index, const std::vector<uint8_t>& data);

//// to test:
//  uint16_t num16 = 0xabcd;
//  uint32_t num32 = 0x12345678;
//  std::vector<uint8_t> vec;
//  FERSpack(32, num32, &vec);
//  std::printf("FERSpack   : num32 = %x test32= %x %x %x %x\n", num32, vec.at(0), vec.at(1), vec.at(2), vec.at(3));
//  FERSpack(16, num16, &vec);
//  std::printf("FERSpack   : num16 = %x test16= %x %x\n", num16, vec.at(4), vec.at(5));
//
//  uint32_t num32r = FERSunpack32( 0, vec );
//  uint16_t num16r = FERSunpack16( 4, vec );
//  std::printf("FERSunpack : num32r = %x\n", num32r);
//  std::printf("FERSunpack : num16r = %x\n", num16r);


///////////////////////  FUNCTIONS IN ALPHA STATE  /////////////////////
///////////////////////  NO DEBUG IS DONE. AT ALL! /////////////////////

void initshm( int shmid );
void openasciistream(shmseg* shmp, int brd );
void closeasciistream(shmseg* shmp);
void writeasciistream(int brd, std::string buffer);
void dumpshm( struct shmseg* shmp, int brd );


//
// IN ORDER TO ACCESS IT:
//
// 1) general note: the current board number variable, say
// int brd;
// must be private in both producer and monitor 
//
// 2) in the PRODUCER:
// put this in the global section of the file:
//
// struct shmseg *shmp;
// int shmid;
//
// 3) in the MONITOR:
// put this in the global section of the file:
//
// extern struct shmseg *shmp;
// extern int shmid;
//
// 4) this goes in DoInitialise() of both PRODUCER and MONITOR
// 
// shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644|IPC_CREAT);
// if (shmid == -1) {
//	perror("Shared memory");
// }
// shmp = (shmseg*)shmat(shmid, NULL, 0);
// if (shmp == (void *) -1) {
//	perror("Shared memory attach");
// }
//
// 5) in DoInitialise() of PRODUCER, also add
//
// initshm( shmid );
//
// 6) that's it! you can now access the content of the shared structure via pointer, for example
//
// shmp->connectedboards++;
//
//
// CLEAN UP
// in the DoTerminate of the producer (since the latter is the class that created the shared memory, it is its responsibility to also destroy it)
//
//if (shmdt(shmp) == -1) {
//	perror("shmdt");
//}

// WARNING!!!
// for no logical reason (solar flare? gone fishing? wrong
// days of the month?) the SHM_KEY may suddenly become
// invalid, causing a crash of producer in the init stage.
// Just change it to something else 
// Tried (bad) keys: 0x1234

#endif
