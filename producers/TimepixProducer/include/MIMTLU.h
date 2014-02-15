/*
 * MIMTLU.h
 *
 *  Created on: May 7, 2013
 *      Author: mbenoit
 */

#ifndef MIMTLU_H_
#define MIMTLU_H_

#include <sys/socket.h> 
#include <netdb.h>     
#include <iostream>
#include <fstream>
#include <cstring> 
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string.h>
#include <exception>
#include <stdexcept>

//#define DEBUGMTLU

class mimtlu_exception : public std::runtime_error 
{
public:
  mimtlu_exception(const std::string& message) 
    : std::runtime_error(message) 
  { };
};


struct mimtlu_event
{
  unsigned long int timestamp;
  unsigned char track;
  unsigned int tlu;
  char txt[17];
  mimtlu_event(unsigned long int _timestamp,unsigned char _track,  unsigned int _tlu)
  {
    timestamp=_timestamp;
    track=_track;
    tlu=_tlu;
    txt[16]=0;
  }
  mimtlu_event(const char *buf)
  {
    from_string(buf);
    
  }

  void from_string(const char *buf)
  {
    if (strlen(buf)!=16)
      throw mimtlu_exception("MIMTLU event parssing error");
    strncpy(txt,&buf[0],16);
    txt[16]=0;

    char tmp[16];
    //timestamp
    strncpy(tmp,&buf[0],10);
    tmp[10]=0;
    sscanf(tmp, "%lx", &timestamp);
    //track
    unsigned int tmpint;
    strncpy(tmp,&buf[10],2);
    tmp[2]=0;
    sscanf(tmp, "%x", &tmpint);
    track=tmpint&0xff;
    //tlu
    strncpy(tmp,&buf[12],4);
    tmp[4]=0;
    sscanf(tmp, "%x", &tlu);
    return;
  }
  
  const char * to_char(void)
  {
    return txt;
  }
};

class MIMTLU 
{
public:
  MIMTLU();
  int               Connect(char* IP,char* port);
  std::vector<mimtlu_event> GetEvents(void);
  void              Arm(void);
  void              SetNumberOfTriggers(const unsigned int n); 
  void              SetPulseLength(const unsigned int n);
  void              SetShutterLength(const unsigned int n);
  void              SetShutterMode(const unsigned int n);
private :
  unsigned int NTrigger;
  unsigned int PulseLength;
  int status;
  struct addrinfo host_info;       
  struct addrinfo *host_info_list; 
  int socketfd ; 
  int len, bytes_sent;
  ssize_t bytes_recieved;	
  char msg [1024];
  char incoming_data_buffer[1024];
  unsigned long int tluevtnr;

};

#endif /* MIMTLU_H_ */
