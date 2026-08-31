/////////////////////////////////////////////////////////////////////
//                         2023 May 08                             //
//                   authors: F. Tortorici                         //
//                email: francesco.tortorici@ct.infn.it            //
//                        notes:                                   //
/////////////////////////////////////////////////////////////////////

#include "eudaq/Producer.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "eudaq/Monitor.hh"

//#include "configure.h"

#include "DataSender.hh"
#include "QTP_EUDAQ.h"


//////////////////////////

void QTPpack_event(uint16_t ADCdata[32], std::vector<uint8_t> *vec)
{

    for (int i = 0; i < 32; i++) {
	FERSpack( 16, ADCdata[i], vec);
    }

}


void QTPunpack_event(std::vector<uint8_t> *vec, uint16_t (&ADCdata)[32])
{

  std::vector<uint8_t> data( vec->begin(), vec->end() );


  int index = 0;

  for(int ig = 0; ig <32; ig++) {
	ADCdata[ig] =  FERSunpack16(index, data); index+=2;
  }

}



//////////////////////////


