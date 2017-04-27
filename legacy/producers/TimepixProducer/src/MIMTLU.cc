#include "MIMTLU.h"
#include <time.h>

extern time_t t0 ;
using namespace std;

MIMTLU::MIMTLU() 
{
  NTrigger =1;
}

char * get_time(void);

int MIMTLU::Connect(char* IP,char* port)
{
  memset(&host_info, 0, sizeof host_info);
  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  // Address
  status = getaddrinfo(IP,port, &host_info, &host_info_list);
  if (status != 0)
    {
    cout << "[MiMTLU] getaddrinfo error" << gai_strerror(status) << endl;
    return -1;
  };

  // Socket
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,host_info_list->ai_protocol);

  if (socketfd == -1) {
    std::cout << "[MiMTLU] Socket error " << endl;
    return -2;
  };

  // Connection
  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)  {
    std::cout << "connect error " << gai_strerror(status) << " " << host_info.ai_flags << endl;
    return -3;
  }
  else {
    std::cout << "[MiMTLU] connect established " << endl;
    SetNumberOfTriggers(1);
    return 1;
  }

}

void MIMTLU::SetShutterLength(const unsigned int n)
{
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] SetShutterLength "<<n<<std::endl;
#endif
  memset(msg,0,1024);
  sprintf(msg,"S %i\r\n",n);
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
  sleep(1);
}

  
void MIMTLU::SetNumberOfTriggers(const unsigned int n){
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] SetNumberOfTriggers "<<n<<std::endl;
#endif
  if (n<1 || n>40) return;
  memset(msg,0,1024);
  sprintf(msg,"T %i\r\n",n);
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
  NTrigger = n;
  sleep(1);
}

void MIMTLU::SetPulseLength(const unsigned int n){
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] SetPulseLength "<<n<<std::endl;
#endif
 
  memset(msg,0,1024);
  sprintf(msg,"P %i\r\n",n);
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
  PulseLength = n;
  sleep(1);
}

void MIMTLU::SetShutterMode(const unsigned int n){
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] SetShutterMode "<<n<<std::endl;
#endif
 
  memset(msg,0,1024);
  sprintf(msg,"M %i\r\n",n);
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
  sleep(1);
}

void MIMTLU::Arm()
{
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] Arm "<<std::endl;
#endif
  memset(msg,0,16);
  sprintf(msg,"A\r\n");
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
}

std::vector<mimtlu_event> MIMTLU::GetEvents()
{
#ifdef DEBUGMTLU
  std::cout << get_time() << " [MiMTLU] GetEvents "<<std::endl;
#endif
  std::vector<mimtlu_event> events;
  memset(msg,0,1024);
  sprintf(msg,"X\r\n");
  bytes_sent = send(socketfd,msg, strlen(msg), 0);
  int bytes_expected=18*NTrigger;
  bytes_recieved = recv(socketfd, incoming_data_buffer,bytes_expected, 0);
  incoming_data_buffer[bytes_recieved]=0;
 // std::cout << incoming_data_buffer<<std::endl;
  // If no data arrives, the program will just wait here until some data arrives.
  if (bytes_recieved == 0) 
    throw mimtlu_exception("[MiMTLU] Host shut down");

  if (bytes_recieved == -1)
    throw mimtlu_exception("[MiMTLU] Receive error");

  if (bytes_recieved != bytes_expected) 
    throw mimtlu_exception("[MiMTLU] Unexpected number of bytes received");

  unsigned int i;
  char tmp[20];
  for(i=0;i<NTrigger;i++)
  {
    strncpy(tmp,&incoming_data_buffer[18*i],16);
    tmp[16]=0;
#ifdef DEBUGMTLU
    std::cout<<"[MiMTLU] evnt: "<<tmp<<endl;
#endif
    mimtlu_event evn(tmp);
    events.push_back(evn);
  }
  return events;
}


