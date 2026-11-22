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
#include "DRS_EUDAQ.h"


//////////////////////////

void DRSpack_event(void* Event, std::vector<uint8_t> *vec)
{
  // temporary event, used to correctly interpret the Event.
  // The same technique is used in the other pack routines as well
  CAEN_DGTZ_X742_EVENT_t *tmpEvent = (CAEN_DGTZ_X742_EVENT_t*)Event;
  for(int ig = 0; ig <MAX_X742_GROUP_SIZE; ig++) {
	vec->push_back(tmpEvent->GrPresent[ig]);
  }
  for(int ig = 0; ig <MAX_X742_GROUP_SIZE; ig++) {
    if(tmpEvent->GrPresent[ig]) {
	CAEN_DGTZ_X742_GROUP_t *Event_gr = &tmpEvent->DataGroup[ig];
	for(int ich =0 ; ich <MAX_X742_CHANNEL_SIZE; ich++) {
		FERSpack( 32,             Event_gr->ChSize[ich],     vec);
	}
	for(int ich =0 ; ich <MAX_X742_CHANNEL_SIZE; ich++) {
		for(int its =0 ; its <Event_gr->ChSize[ich]; its++) {
			//std::cout<<"----9999--- Event_gr->DataChannel[ich][its] "<<its<<" "<<Event_gr->DataChannel[ich][its]<<std::endl;
			FERSpack( 32,  static_cast<uint32_t>(Event_gr->DataChannel[ich][its]),     vec);
		}
	}
	FERSpack( 32,             static_cast<uint32_t>(Event_gr->TriggerTimeTag),     vec);
	FERSpack( 16,             static_cast<uint16_t>(Event_gr->StartIndexCell),     vec);
    }// end if group is present 
 }// end group loop

}



CAEN_DGTZ_X742_EVENT_t* DRSunpack_event(std::vector<uint8_t> *vec)
{
  std::vector<uint8_t> data(vec->begin(), vec->end());
  CAEN_DGTZ_X742_EVENT_t *tmpEvent = (CAEN_DGTZ_X742_EVENT_t *)malloc(sizeof(CAEN_DGTZ_X742_EVENT_t));
  memset(tmpEvent, 0, sizeof(CAEN_DGTZ_X742_EVENT_t));


  int index = 0;
  for (int ig = 0; ig < MAX_X742_GROUP_SIZE; ig++) {
    tmpEvent->GrPresent[ig] = data.at(index); index += 1;
  }

  for (int ig = 0; ig < MAX_X742_GROUP_SIZE; ig++) {
    if (tmpEvent->GrPresent[ig]) {
      for (int ich = 0; ich < MAX_X742_CHANNEL_SIZE; ich++) {
        tmpEvent->DataGroup[ig].ChSize[ich] = FERSunpack32(index, data); index += 4;
      }
      for (int ich = 0; ich < MAX_X742_CHANNEL_SIZE; ich++) {
        tmpEvent->DataGroup[ig].DataChannel[ich] = (float *)malloc(sizeof(float) * tmpEvent->DataGroup[ig].ChSize[ich]);  
        for (int its = 0; its < tmpEvent->DataGroup[ig].ChSize[ich]; its++) {
          tmpEvent->DataGroup[ig].DataChannel[ich][its] = FERSunpack32(index, data); index += 4;
        }
      }
      tmpEvent->DataGroup[ig].TriggerTimeTag = FERSunpack32(index, data); index += 4;
      tmpEvent->DataGroup[ig].StartIndexCell = FERSunpack16(index, data); index += 2;
    }
  }

  return tmpEvent;
}

CAEN_DGTZ_X742_EVENT_S_t DRSunpack_event_S(std::vector<uint8_t> *vec)
{
    std::vector<uint8_t> data(vec->begin(), vec->end());
    CAEN_DGTZ_X742_EVENT_S_t tmpEvent; // stack-allocated, no malloc
    memset(&tmpEvent, 0, sizeof(CAEN_DGTZ_X742_EVENT_S_t));

    int index = 0;
    for (int ig = 0; ig < MAX_X742_GROUP_SIZE; ig++) {
        tmpEvent.GrPresent[ig] = data.at(index);
        index += 1;
    }

    for (int ig = 0; ig < MAX_X742_GROUP_SIZE; ig++) {
        if (tmpEvent.GrPresent[ig]) {
            for (int ich = 0; ich < MAX_X742_CHANNEL_SIZE; ich++) {
                tmpEvent.DataGroup[ig].ChSize[ich] = FERSunpack32(index, data);
                index += 4;
            }
            for (int ich = 0; ich < MAX_X742_CHANNEL_SIZE; ich++) {
                for (uint32_t its = 0; its < tmpEvent.DataGroup[ig].ChSize[ich]; its++) {
                    tmpEvent.DataGroup[ig].DataChannel[ich][its] = FERSunpack32(index, data);
                    index += 4;
                }
            }
            tmpEvent.DataGroup[ig].TriggerTimeTag = FERSunpack32(index, data); index += 4;
            tmpEvent.DataGroup[ig].StartIndexCell = FERSunpack16(index, data); index += 2;
        }
    }

    return tmpEvent; // returned by value, no pointers or dynamic memory
}




void FreeDRSEvent(CAEN_DGTZ_X742_EVENT_t *event) {
    if (!event) return;
    for (int ig = 0; ig < MAX_X742_GROUP_SIZE; ig++) {
        if (event->GrPresent[ig]) {
            for (int ich = 0; ich < MAX_X742_CHANNEL_SIZE; ich++) {
                if (event->DataGroup[ig].DataChannel[ich]) {
                    free(event->DataGroup[ig].DataChannel[ich]);
                    event->DataGroup[ig].DataChannel[ich] = nullptr;
                }
            }
        }
    }
    free(event);
}


//////////////////////////


