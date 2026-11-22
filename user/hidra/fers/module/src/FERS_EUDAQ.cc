/////////////////////////////////////////////////////////////////////
//                         2023 May 08                             //
//                   authors: F. Tortorici                         //
//                email: francesco.tortorici@ct.infn.it            //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#include "eudaq/Producer.hh"
#include "FERS_Registers_5202.h"
#include "FERS_Registers_5215.h"
#include "FERSlib.h"
#undef max
#undef min
#include <iostream>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "eudaq/Monitor.hh"

//#include "configure.h"

#include "DataSender.hh"
#include "FERS_EUDAQ.h"

std::fstream runfile[MAX_NBRD]; // pointers to ascii output data files



// puts a nbits (16, 32, 64) integer into an 8 bits vector.
// bytes are in a machine-independent order

void FERSpack(int nbits, uint32_t input, std::vector<uint8_t> *vec)
{
	uint8_t out;// = (uint8_t)( input & 0x00FF);

	int n;
	//vec->push_back( out );
	for (int i=0; i<nbits/8; i++){
		n = 8*i;
		out = (uint8_t)( (input >> n) & 0xFF ) ;
		vec->push_back( out );
	}
}



// reads a 16/32 bits integer from a 8 bits vector
/*
uint16_t FERSunpack16(int index, std::vector<uint8_t> vec)
{
	uint16_t out = vec.at(index) + vec.at(index+1) * 256;
	return out;
}


uint32_t FERSunpack32(int index, std::vector<uint8_t> vec)
{
	uint32_t out = vec.at(index) 
                  +vec.at(index+1) *256 
                  +vec.at(index+2) *65536 
                  +vec.at(index+3) *16777216;
	return out;
}
uint64_t FERSunpack64(int index, std::vector<uint8_t> vec)
{
	uint64_t out = vec.at(index) 
		+vec.at(index+1) *256 
		+vec.at(index+2) *65536
		+vec.at(index+3) *16777216
		+( vec.at(index+4) 
		  +vec.at(index+5) *256
		  +vec.at(index+6) *65536
		  +vec.at(index+7) *16777216
		 )*4294967296;
	return out;
}
*/

uint16_t FERSunpack16(int index, const std::vector<uint8_t>& vec)
{
    return static_cast<uint16_t>(vec[index]) |
           (static_cast<uint16_t>(vec[index + 1]) << 8);
}


uint32_t FERSunpack32(int index, const std::vector<uint8_t>& vec) {
    if (index < 0 || static_cast<size_t>(index + 3) >= vec.size()) {
        throw std::out_of_range("Index out of bounds in FERSunpack32");
    }
    return static_cast<uint32_t>(vec[index]) |
           (static_cast<uint32_t>(vec[index + 1]) << 8) |
           (static_cast<uint32_t>(vec[index + 2]) << 16) |
           (static_cast<uint32_t>(vec[index + 3]) << 24);
}


uint64_t FERSunpack64(int index, const std::vector<uint8_t>& vec)
{
    return (static_cast<uint64_t>(vec[index + 0])      ) |
           (static_cast<uint64_t>(vec[index + 1]) <<  8) |
           (static_cast<uint64_t>(vec[index + 2]) << 16) |
           (static_cast<uint64_t>(vec[index + 3]) << 24) |
           (static_cast<uint64_t>(vec[index + 4]) << 32) |
           (static_cast<uint64_t>(vec[index + 5]) << 40) |
           (static_cast<uint64_t>(vec[index + 6]) << 48) |
           (static_cast<uint64_t>(vec[index + 7]) << 56);
}





void FERSpackevent(void* Event, int dataqualifier, std::vector<uint8_t> *vec)
{
  switch( dataqualifier )
  {
    case (DTQ_SPECT):
      FERSpack_spectevent(Event, vec);
      break;
    case (DTQ_TIMING): // List Event (Timing Mode only)
      FERSpack_listevent(Event, vec);
      break;
    case (DTQ_TSPECT): // Spectroscopy + Timing Mode (Energy + Tstamp)
      FERSpack_tspectevent(Event, vec);
      break;
    case (DTQ_COUNT):	// Counting Mode (MCS)
      FERSpack_countevent(Event, vec);
      break;
    case (DTQ_WAVE): // Waveform Mode
      FERSpack_waveevent(Event, vec);
      break;
    case (DTQ_TEST): // Test Mode (fixed data patterns)
      FERSpack_testevent(Event, vec);
      break;
    case (DTQ_STAIRCASE): // staircase event
      //EUDAQ_INFO("*** FERSpackevent > received a staircase event");
      FERSpack_staircaseevent(Event, vec);
  }
}

//////////////////////////

// Spectroscopy event
void FERSpack_spectevent(void* Event, std::vector<uint8_t> *vec)
{
  //int x_pixel = 8;
  //int y_pixel = 8;
  int nchan = 64;
  // temporary event, used to correctly interpret the Event.
  // The same technique is used in the other pack routines as well
  SpectEvent_t *tmpEvent = (SpectEvent_t*)Event;
  // the following group of vars is not really needed. Used for debug purposes.
  // This is valid also for the other pack routines
  double   tstamp_us      = tmpEvent->tstamp_us ;
  double   rel_tstamp_us      = tmpEvent->rel_tstamp_us ;
  uint64_t trigger_id     = tmpEvent->trigger_id;
  uint64_t chmask         = tmpEvent->chmask    ;
  uint64_t qdmask         = tmpEvent->qdmask    ;
  uint16_t energyHG[nchan];
  uint16_t energyLG[nchan];
  uint32_t tstamp[nchan]  ;
  uint16_t ToT[nchan]     ;

  FERSpack( 8*sizeof(double), tstamp_us,  vec);
  FERSpack( 8*sizeof(double), rel_tstamp_us,  vec);
  FERSpack( 64,             trigger_id, vec);
  FERSpack( 64,             chmask,     vec);
  FERSpack( 64,             qdmask,     vec);
  for (size_t i = 0; i<nchan; i++){
    energyHG[i] = tmpEvent->energyHG[i];
    FERSpack( 16,energyHG[i], vec);
  }
  for (size_t i = 0; i<nchan; i++){
    energyLG[i] = tmpEvent->energyLG[i];
    FERSpack( 16,energyLG[i], vec);
  }
  for (size_t i = 0; i<nchan; i++){
    tstamp[i]   = tmpEvent->tstamp[i]  ;
    FERSpack( 32,tstamp[i]  , vec);
  }
  for (size_t i = 0; i<nchan; i++){
    ToT[i]      = tmpEvent->ToT[i]     ;
    FERSpack( 16,ToT[i]     , vec);
  }

  //      //dump on console
        //std::cout<< "tstamp_us  "<< tstamp_us  <<std::endl;
        //std::cout<< "trigger_id "<< trigger_id <<std::endl;
        //std::cout<< "chmask     "<< chmask     <<std::endl;
        //std::cout<< "qdmask     "<< qdmask     <<std::endl;
  
        //for(size_t i = 0; i < y_pixel; ++i) {
        //  for(size_t n = 0; n < x_pixel; ++n){
        //    //std::cout<< (int)hit[n+i*x_pixel] <<"_";
        //    std::cout<< (int)energyHG[n+i*x_pixel] <<"_";
        //    //std::cout<< (int)energyLG[n+i*x_pixel] <<"_";
        //    //std::cout<< (int)tstamp  [n+i*x_pixel] <<"_";
        //    //std::cout<< (int)ToT     [n+i*x_pixel] <<"_";
        //  }
        //  std::cout<< "<<"<< std::endl;
        //}
        //std::cout<<std::endl;
}


//void FERSunpack_spectevent(SpectEvent_t* Event, std::vector<uint8_t> *vec)
SpectEvent_t FERSunpack_spectevent(std::vector<uint8_t> *vec)
{
  int nchan = 64;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  SpectEvent_t tmpEvent;

  int dsize = data.size();
  int tsize = sizeof( tmpEvent );
  if ( tsize != dsize ){
	  EUDAQ_THROW("*** WRONG DATA SIZE FOR SPECT EVENT! "+std::to_string(tsize)+" vs "+std::to_string(dsize)+" ***");
  } else {
	  //EUDAQ_WARN("Size of data is right");
  }

  bool debug = false; // toggle printing of debug info. This and if's below can be removed in production version
  int index = 0;
  int sd = sizeof(double);
  switch(8*sd)
  {
    case 8:
      tmpEvent.tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.tstamp_us = FERSunpack64(index, data);
  }
  index += sd; // each element of data is uint8_t

  switch(8*sd)
  {
    case 8:
      tmpEvent.rel_tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.rel_tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.rel_tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.rel_tstamp_us = FERSunpack64(index, data);
  }
  index += sd; // each element of data is uint8_t


  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read double tstamp_us: "+std::to_string(tmpEvent.tstamp_us));
  tmpEvent.trigger_id = FERSunpack64( index,data); index += 8;
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint64_t trigger_id: "+std::to_string(tmpEvent.trigger_id));
  tmpEvent.chmask     = FERSunpack64( index,data); index += 8;
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint64_t chmask: "+std::to_string(tmpEvent.chmask));
  tmpEvent.qdmask     = FERSunpack64( index,data); index += 8;
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint64_t qdmask: "+std::to_string(tmpEvent.qdmask));
  //
  uint64_t sum=0;
  for (int i=0; i<nchan; ++i) {
    tmpEvent.energyHG[i] = FERSunpack16(index,data); index += 2;
    sum+=tmpEvent.energyHG[i];
  }
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint16 energyHG[64], sum: "+std::to_string(sum));
  sum=0;
  for (int i=0; i<nchan; ++i) {
    tmpEvent.energyLG[i] = FERSunpack16(index,data); index += 2;
    sum+=tmpEvent.energyLG[i];
  }
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint16 energyLG[64], sum: "+std::to_string(sum));
  sum=0;
  for (int i=0; i<nchan; ++i) {
    tmpEvent.tstamp[i] = FERSunpack32(index,data); index += 4;
    sum+=tmpEvent.tstamp[i];
  }
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint32 tstamp[64], sum: "+std::to_string(sum));
  sum=0;
  for (int i=0; i<nchan; ++i) {
    tmpEvent.ToT[i] = FERSunpack16(index,data); index += 2;
    sum+=tmpEvent.ToT[i];
  }
  if (debug) EUDAQ_WARN("* index: "+std::to_string(index)+" read uint16 ToT[64], sum: "+std::to_string(sum));

  return tmpEvent;
}



//////////////////////////


//// List Event (timing mode only)
void FERSpack_listevent(void* Event, std::vector<uint8_t> *vec)
{
  ListEvent_t *tmpEvent = (ListEvent_t*) Event;
  uint16_t nhits = tmpEvent->nhits;
  uint8_t  channel[MAX_LIST_SIZE];
  uint32_t tstamp[MAX_LIST_SIZE];
  uint16_t ToT[MAX_LIST_SIZE];

  FERSpack(16, nhits, vec);
  for (size_t i = 0; i<MAX_LIST_SIZE; i++){
    channel[i] = tmpEvent->channel[i];
    vec->push_back(channel[i]);
  }
  for (size_t i = 0; i<MAX_LIST_SIZE; i++){
    tstamp[i] = tmpEvent->tstamp[i];
    FERSpack( 32,tstamp[i], vec);
  }
  for (size_t i = 0; i<MAX_LIST_SIZE; i++){
    ToT[i] = tmpEvent->ToT[i];
    FERSpack( 16,ToT[i], vec);
  }
}
ListEvent_t FERSunpack_listevent(std::vector<uint8_t> *vec)
{
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  ListEvent_t tmpEvent;
  int index = 0;
  tmpEvent.nhits = FERSunpack16(index, data); index +=2;
  for (int i=0; i<MAX_LIST_SIZE; ++i) {
    tmpEvent.channel[i] = data.at(index); index += 1;
  }
  for (int i=0; i<MAX_LIST_SIZE; ++i) {
    tmpEvent.tstamp[i] = FERSunpack32(index,data); index += 4;
  }
  for (int i=0; i<MAX_LIST_SIZE; ++i) {
    tmpEvent.ToT[i] = FERSunpack16(index,data); index += 2;
  }
  return tmpEvent;
}

//////////////////////////


//// Spectroscopy + Timing Mode (Energy + Tstamp)
void FERSpack_tspectevent(void* Event, std::vector<uint8_t> *vec)
{
  int nchan = 64;
  SpectEvent_t *tmpEvent = (SpectEvent_t*)Event;
  double   tstamp_us      = tmpEvent->tstamp_us ;
  uint64_t trigger_id     = tmpEvent->trigger_id;
  uint64_t chmask         = tmpEvent->chmask    ;
  uint64_t qdmask         = tmpEvent->qdmask    ;
  uint16_t energyHG[nchan];
  uint16_t energyLG[nchan];
  uint32_t tstamp[nchan]  ;
  uint16_t ToT[nchan]     ;

  FERSpack( 8*sizeof(double), tstamp_us,  vec);
  FERSpack( 64,             trigger_id, vec);
  FERSpack( 64,             chmask,     vec);
  FERSpack( 64,             qdmask,     vec);
  for (size_t i = 0; i<nchan; i++){
    energyHG[i] = tmpEvent->energyHG[i];
    FERSpack( 16,energyHG[i], vec);
  }
  for (size_t i = 0; i<nchan; i++){
    energyLG[i] = tmpEvent->energyLG[i];
    FERSpack( 16,energyLG[i], vec);
  }
  for (size_t i = 0; i<nchan; i++){
    tstamp[i]   = tmpEvent->tstamp[i]  ;
    FERSpack( 32,tstamp[i]  , vec);
  }
  for (size_t i = 0; i<nchan; i++){
    ToT[i]      = tmpEvent->ToT[i]     ;
    FERSpack( 16,ToT[i]     , vec);
  }

}
SpectEvent_t FERSunpack_tspectevent(std::vector<uint8_t> *vec)
{
  int nchan = 64;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  SpectEvent_t tmpEvent;

  int index = 0;
  int sd = sizeof(double);
  switch(8*sd)
  {
    case 8:
      tmpEvent.tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.tstamp_us = FERSunpack64(index, data);
  }
  index += sd;
  tmpEvent.trigger_id = FERSunpack64( index,data); index += 8;
  tmpEvent.chmask     = FERSunpack64( index,data); index += 8;
  tmpEvent.qdmask     = FERSunpack64( index,data); index += 8;
  for (int i=0; i<nchan; ++i) {
    tmpEvent.energyHG[i] = FERSunpack16(index,data); index += 2;
  }
  for (int i=0; i<nchan; ++i) {
    tmpEvent.energyLG[i] = FERSunpack16(index,data); index += 2;
  }
  for (int i=0; i<nchan; ++i) {
    tmpEvent.tstamp[i] = FERSunpack32(index,data); index += 4;
  }
  for (int i=0; i<nchan; ++i) {
    tmpEvent.ToT[i] = FERSunpack16(index,data); index += 2;
  }
  return tmpEvent;
}


//////////////////////////

//// Counting Event
void FERSpack_countevent(void* Event, std::vector<uint8_t> *vec)
{
  int nchan=64;
  CountingEvent_t* tmpEvent = (CountingEvent_t*) Event;
  FERSpack( 8*sizeof(double), tmpEvent->tstamp_us,  vec);
  FERSpack(	64, tmpEvent->trigger_id, vec);
  FERSpack(	64, tmpEvent->chmask, vec);
  for (size_t i = 0; i<nchan; i++){
    FERSpack( 32,tmpEvent->counts[i], vec);
  }
  FERSpack(	32, tmpEvent->t_or_counts, vec);
  FERSpack(	32, tmpEvent->q_or_counts, vec);
}
CountingEvent_t FERSunpack_countevent(std::vector<uint8_t> *vec)
{
  int nchan=64;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  CountingEvent_t tmpEvent;
  int index = 0;
  int sd = sizeof(double);
  switch(8*sd)
  {
    case 8:
      tmpEvent.tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.tstamp_us = FERSunpack64(index, data);
  }
  index += sd;
  tmpEvent.trigger_id = FERSunpack64(index,data); index += 8;
  tmpEvent.chmask = FERSunpack64(index,data); index += 8;
  for (size_t i = 0; i<nchan; i++){
    tmpEvent.counts[i] = FERSunpack32(index,data); index += 4;
  }
  tmpEvent.t_or_counts = FERSunpack32(index,data); index += 4;
  tmpEvent.q_or_counts = FERSunpack32(index,data); index += 4;
  return tmpEvent;
}

//////////////////////////


//// Waveform Event
void FERSpack_waveevent(void* Event, std::vector<uint8_t> *vec)
{
  int n = MAX_WAVEFORM_LENGTH;
  WaveEvent_t* tmpEvent = (WaveEvent_t*) Event;
  FERSpack( 8*sizeof(double), tmpEvent->tstamp_us,  vec);
  FERSpack(64, tmpEvent->trigger_id, vec);
  FERSpack(16, tmpEvent->ns, vec);
  for (int i=0; i<n; i++) FERSpack(16, tmpEvent->wave_hg[i], vec);
  for (int i=0; i<n; i++) FERSpack(16, tmpEvent->wave_lg[i], vec);
  for (int i=0; i<n; i++) vec->push_back(tmpEvent->dig_probes[i]);
}
WaveEvent_t FERSunpack_waveevent(std::vector<uint8_t> *vec)
{
  int n = MAX_WAVEFORM_LENGTH;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  WaveEvent_t tmpEvent;
  int index = 0;
  int sd = sizeof(double);
  switch(8*sd)
  {
    case 8:
      tmpEvent.tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.tstamp_us = FERSunpack64(index, data);
  }
  index += sd;
  tmpEvent.trigger_id = FERSunpack64(index,data); index += 8;
  tmpEvent.ns = FERSunpack16(index,data); index += 2;
  for (int i=0; i<n; i++){
    tmpEvent.wave_hg[i] = FERSunpack16(index,data); index += 2;
  }
  for (int i=0; i<n; i++){
    tmpEvent.wave_lg[i] = FERSunpack16(index,data); index += 2;
  }
  for (int i=0; i<n; i++){
    tmpEvent.dig_probes[i] = vec->at(index); index += 1;
  }
  return tmpEvent;
}

//////////////////////////


//// Test Mode Event (fixed data patterns)
void FERSpack_testevent(void* Event, std::vector<uint8_t> *vec)
{
  int n = MAX_TEST_NWORDS;
  TestEvent_t* tmpEvent = (TestEvent_t*) Event;
  FERSpack( 8*sizeof(double), tmpEvent->tstamp_us,  vec);
  FERSpack(64, tmpEvent->trigger_id, vec);
  FERSpack(16, tmpEvent->nwords, vec);
  for (int i=0; i<n; i++) FERSpack(32, tmpEvent->test_data[i], vec);
}
TestEvent_t FERSunpack_testevent(std::vector<uint8_t> *vec)
{
  int n = MAX_TEST_NWORDS;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  TestEvent_t tmpEvent;
  int index = 0;
  int sd = sizeof(double);
  switch(8*sd)
  {
    case 8:
      tmpEvent.tstamp_us = data.at(index); break;
    case 16:
      tmpEvent.tstamp_us = FERSunpack16(index, data); break;
    case 32:
      tmpEvent.tstamp_us = FERSunpack32(index, data); break;
    case 64:
      tmpEvent.tstamp_us = FERSunpack64(index, data);
  }
  index += sd;
  tmpEvent.trigger_id  = FERSunpack64(index, data); index+=8; 
  tmpEvent.nwords      = FERSunpack16(index, data); index+=2;
  for (int i=0; i<n; i++){
    tmpEvent.test_data[i] = FERSunpack32(index, data); index+=4;
  }
  return tmpEvent;
}

//////////////////////////
//	uint16_t threshold;
//	uint16_t shapingt; // enum, see FERS_Registers.h
//	uint32_t dwell_time; // in seconds, divide hitcnt by this to get rate
//	uint32_t chmean; // over channels, no division by time
//	uint32_t HV; // 1000 * HV from HV_Get_Vbias( handle, &HV);
//	uint32_t Tor_cnt;
//	uint32_t Qor_cnt;
//	uint32_t hitcnt[FERSLIB_MAX_NCH];
//} StaircaseEvent_t;
void FERSpack_staircaseevent(void* Event, std::vector<uint8_t> *vec){
  //EUDAQ_INFO("*** FERSpack_staircaseevent > starting encoding");
  int n = FERSLIB_MAX_NCH_5202;
  StaircaseEvent_t* tmpEvent = (StaircaseEvent_t*) Event;
  int index=0;
  FERSpack(16, tmpEvent->threshold, vec);  index+=2;
  FERSpack(16, tmpEvent->shapingt, vec);   index+=2;
  FERSpack(32, tmpEvent->dwell_time, vec); index+=4;
  FERSpack(32, tmpEvent->chmean, vec);     index+=4;
  FERSpack(32, tmpEvent->HV,  vec);        index+=4;
  FERSpack(32, tmpEvent->Tor_cnt, vec);    index+=4;
  FERSpack(32, tmpEvent->Qor_cnt, vec);    index+=4;
  for (int i=0; i<n; i++)
  {
	  FERSpack(32, tmpEvent->hitcnt[i], vec); index+=4;
  }
  //EUDAQ_INFO("*** FERSpack_staircaseevent > event cast: size " +std::to_string(sizeof(*tmpEvent)));
  //EUDAQ_INFO("*** FERSpack_staircaseevent > index: "+std::to_string(index));
  //EUDAQ_INFO("*** FERSpack_staircaseevent > threshold: "+std::to_string(tmpEvent->threshold));
};
StaircaseEvent_t FERSunpack_staircaseevent(std::vector<uint8_t> *vec){
  int n = FERSLIB_MAX_NCH_5202;
  std::vector<uint8_t> data( vec->begin(), vec->end() );
  StaircaseEvent_t tmpEvent;
  int index = 0;
  tmpEvent.threshold  = FERSunpack16(index, data); index+=2;
  tmpEvent.shapingt   = FERSunpack16(index, data); index+=2;
  tmpEvent.dwell_time = FERSunpack32(index, data); index+=4;
  tmpEvent.chmean     = FERSunpack32(index, data); index+=4;
  tmpEvent.HV         = FERSunpack32(index, data); index+=4;
  tmpEvent.Tor_cnt    = FERSunpack32(index, data); index+=4; 
  tmpEvent.Qor_cnt    = FERSunpack32(index, data); index+=4; 
  for (int i=0; i<n; i++){
    tmpEvent.hitcnt[i] = FERSunpack32(index, data); index+=4;
  }
  //EUDAQ_WARN("FERSunpack_staircaseevent: sizeof vec: "+std::to_string(vec->size()) +" index: "+std::to_string(index) +" sizeof struct: "+std::to_string(sizeof(tmpEvent)));
  //dump_vec("FERSunpack_staircaseevent",vec, 16);
  return tmpEvent;

};


//////////////////////////
// fill "data" with some info
void make_header(int board, int PID, std::vector<uint8_t> *data)
{
	uint8_t n=0;
	std::vector<uint8_t> vec;

	// this are needed
	vec.push_back(static_cast<uint8_t> (board));

        // Append PID (2 bytes)
        vec.push_back(static_cast<uint8_t>((PID & 0xFF00) >> 8)); // High byte
        vec.push_back(static_cast<uint8_t>(PID & 0x00FF));        // Low byte
	n+=3;

	// put everything in data, prefixing header with its size
	data->push_back(n);
	//std::cout<<"***** make header > going to write "<< std::to_string(n) <<" bytes" <<std::endl;
	for (int i=0; i<n; i++)
	{
		//std::cout<<"***** make header > writing "<< std::to_string(vec.at(i)) <<std::endl;
		data->push_back( vec.at(i) );
	}
}

// reads back essential header info (see params)
// prints them w/ board ID info with EUDAQ_WARN
// returns index at which raw data starts
int read_header(std::vector<uint8_t> *vec, int *board, int *PID)
{
	std::vector<uint8_t> data(vec->begin(), vec->end());
	int index = data.at(0);

	if(data.size() < index+1)
		EUDAQ_THROW("Unknown data");

	*board = data.at(1);
	*PID =  (static_cast<int>(data.at(2) << 8) | static_cast<int>(data.at(3)));

	return index+1; // first header byte is header size, then index bytes
};


void make_headerFERS(int board, int PID, float HV, float Isipm, float tempDET, float tempFPGA, std::vector<uint8_t> *data)
{
    uint8_t n = 0;
    std::vector<uint8_t> vec;

    // Board ID (1 byte)
    vec.push_back(static_cast<uint8_t>(board));

    // PID (2 bytes, big-endian)
    vec.push_back(static_cast<uint8_t>((PID & 0xFF00) >> 8)); // High byte
    vec.push_back(static_cast<uint8_t>(PID & 0x00FF));        // Low byte
    n += 3;

    auto append_float_be = [&](float value) {
        uint8_t *p = reinterpret_cast<uint8_t *>(&value);
        vec.push_back(p[3]); // Most significant byte
        vec.push_back(p[2]);
        vec.push_back(p[1]);
        vec.push_back(p[0]); // Least significant byte
        n += 4;
    };

    append_float_be(HV);
    append_float_be(Isipm);
    append_float_be(tempDET);
    append_float_be(tempFPGA);

    // Prefix vector with header size
    data->push_back(n);
    for (int i = 0; i < n; i++) {
        data->push_back(vec.at(i));
    }
}





int read_headerFERS(std::vector<uint8_t> *vec, int *board, int *PID, float *HV, float *Isipm, float *tempDET, float *tempFPGA)
{
    std::vector<uint8_t> data(vec->begin(), vec->end());
    int index = data.at(0);

    if (data.size() < index + 1)
        EUDAQ_THROW("Unknown data");

    *board = data.at(1);
    *PID = (static_cast<int>(data.at(2)) << 8) | static_cast<int>(data.at(3));

    // Read float from big-endian byte sequence
    auto read_float_be = [&](int offset, float *dest) {
        uint8_t temp[4];
        temp[3] = data.at(offset);     // MSB
        temp[2] = data.at(offset + 1);
        temp[1] = data.at(offset + 2);
        temp[0] = data.at(offset + 3); // LSB
        std::memcpy(dest, temp, sizeof(float));
    };

    read_float_be(4, HV);
    read_float_be(8, Isipm);
    read_float_be(12, tempDET);
    read_float_be(16, tempFPGA);

    return index + 1; // Header size + header bytes
}


// dump a vector
void dump_vec(std::string title, std::vector<uint8_t> *vec, int start, int stop){
	int n = vec->size();
	if (stop > 0) n = std::min(n, stop);
	std::string printme;
	for (int i=start; i<n; i++){
		printme = "--- "+title+" > vec[" + std::to_string(i) + "] = "+ std::to_string( vec->at(i) );
		EUDAQ_WARN( printme );
	}
};




///////////////////////  FUNCTIONS IN ALPHA STATE  /////////////////////
///////////////////////  NO DEBUG IS DONE. AT ALL! /////////////////////

void initshm( int shmid )
{
	struct shmseg* shmp = (shmseg*)shmat(shmid, NULL, 0);

	std::memset(shmp->DRS_last_trigID, 0, sizeof(shmp->DRS_last_trigID));
	std::memset(shmp->FERS_last_trigID, 0, sizeof(shmp->FERS_last_trigID));

	std::memset(shmp->SUMenergyLG, 0, sizeof(shmp->SUMenergyLG));
	std::memset(shmp->SUMenergyHG, 0, sizeof(shmp->SUMenergyHG));
        shmp->SUMevt=0;

        for (int igr = 0; igr<MAX_NGR; igr++) {
  	    //shmp->connectedboards[igr] = {0};
	    for (int i=0; i<MAX_NBRD; i++)
	    {
		shmp->HVbias[igr][i]=0;
        	shmp->tempFPGA[igr][i]=0;
	        shmp->tempDet[igr][i]=0;
        	shmp->hv_Vmon[igr][i]=0;
	        shmp->hv_Imon[igr][i]=0;
        	shmp->hv_status_on[igr][i]=0;
		//shmp->handle[igr][i]=0;
		//shmp->nchannels[i] = FERSLIB_MAX_NCH_5202; // 64
		//memset(shmp->IP       [i], '\0', MAXCHAR);
	    }
	}
}

/*
void openasciistream(shmseg* shmp, int brd){
	std::map<int, std::string> asciiheader;
	asciiheader[ 0             ] = "#INVALID"              ;
	asciiheader[ DTQ_SPECT     ] = "#tstamp_us,energyHG[]" ;
	asciiheader[ DTQ_TIMING    ] = "#nhits,ToT[]"          ;
	asciiheader[ DTQ_COUNT     ] = "#tstamp_us,counts[]"   ;
	asciiheader[ DTQ_WAVE      ] = "#tstamp_us,wave_hg[]"  ;
	asciiheader[ DTQ_TSPECT    ] = "#tstamp_us,energyHG[]" ;
	asciiheader[ DTQ_TEST      ] = "#tstamp_us,test_data[]";
	asciiheader[ DTQ_STAIRCASE ] = "#rateHz,hitcnt[]"      ;

	EUDAQ_INFO("openasciistream called for board "+std::to_string(brd));
	std::fstream readme;
	readme.open("readme_test.txt",std::ios::out);
	readme<<"board: "<< brd << std::endl;
	//readme<<"IP:"<< shmp->IP[brd] << std::endl;
	readme<<"HVbias:"<< std::to_string(shmp->HVbias[brd]) << std::endl;
	//readme<<"channels:"<< std::to_string(shmp->nchannels[brd]) << std::endl;
	readme<<"handle:"<< std::to_string(shmp->handle[brd]) << std::endl;
	readme.close();
	EUDAQ_INFO("Created readme");

	for (int i=0; i<shmp->connectedboards; i++){
		runfile[i].open(
			"board_" + std::to_string(i)+"_test.txt"
			,std::ios::out);
		if (!runfile[i]){
		EUDAQ_WARN("NOT Opened ascii brd "+std::to_string(brd));
		} else {
		EUDAQ_WARN("Opened ascii brd "+std::to_string(brd));
		}
	}
}
void closeasciistream(shmseg* shmp){
	for (int i=0; i<shmp->connectedboards; i++){
		runfile[i].close();
		EUDAQ_WARN("closed ascii file brd "+std::to_string(i));
	};
}

void writeasciistream(int brd, std::string buffer){
	//EUDAQ_WARN("writeasciistream called with brd = "+std::to_string(brd)+", buffer = "+buffer);
	runfile[brd] << buffer <<std::endl;
}
*/


/*
void dumpshm( struct shmseg* shmp, int brd )
{
	std::string output = "*** shmp["+std::to_string(brd)+"]="+
	"";
	EUDAQ_WARN(output);
	output = "*** shmp["+std::to_string(brd)+"]="+
	" IP:"+shmp->IP[brd]+
	" HVbias:"+std::to_string(shmp->HVbias[brd])+
	//" channels:"+std::to_string(shmp->nchannels[brd])+
	" handle:"+std::to_string(shmp->handle[brd])+
	"";
	EUDAQ_WARN(output);
}
*/
