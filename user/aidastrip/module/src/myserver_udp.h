/* Creates a datagram server.  The port 
 * number is passed as an argument.  This
 * server runs forever 
 *   
 * Author: Mengqing Wu <mengqing.wu@desy.de> @Oct,2017
 * -> server specialized on kpix daq data streaming client
 *
 * -> data structure sent from KPiX Data Net portal:
 *    + Header (uint32_t size of the next xml/data buffer)
 *    + Data/xml (uint32_t *buf or char* xml) -> cut into several bunches w/ max length of 1024 bytes 
 *    + Tail  (added by myself, uint32_t buf[1] = 0xFFFFFFFF)
 *
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <iostream>
#include <cstring>

/*for fd*/
#include <fcntl.h>

using namespace std;
void error(const char*);

class dataserver {
private:
  int m_sock, length;
  struct sockaddr_in m_server;

  char* m_charbuf;
  uint32_t *m_databuf;
  
  socklen_t fromlen;
  struct sockaddr_in from;

  int32_t dataFileFd_;

  bool m_debug;
  
  bool getaheader;
  bool buflost;
  uint32_t m_datasize;
  uint32_t m_datatype;
  int buf_counter;
  const uint32_t maxbufsize; // not implemented in code currently
  int sizeReComBuf;

public:
  enum DataType {
  RawData     = 0,
  XmlConfig   = 1,
  XmlStatus   = 2,
  XmlRunStart = 3,
  XmlRunStop  = 4,
  XmlRunTime  = 5
  };

  dataserver(int32_t port);
  dataserver()
    :fromlen(sizeof(struct sockaddr_in)),
    dataFileFd_(-1), maxbufsize(1024),
    m_datatype(6), m_sock(0) {};
  ~dataserver(){
    if (m_sock>0) close(m_sock);
  };
  
  int getSocket() {return m_sock;};
  int closeSocket(){
    if (m_sock>0)
      return close(m_sock);
    else
      return(0);
  };
  int openSocket(int32_t port);
  int poll(int &evt);
  void listen(bool unlimited);
  void dataHeadTail( uint32_t size);
  int setDebug(bool udebug){
    m_debug=udebug;
    return(0);
  };
};

//--------------------------------------------------------------------------------//

dataserver::dataserver(int32_t port)
  :fromlen(sizeof(struct sockaddr_in)),
   dataFileFd_(-1), maxbufsize(1024),
  m_datatype(6), m_sock(0)
{
  // this will port from 'port' opened on the *localhost* server
  m_sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sock < 0) error("Opening socket");
  length = sizeof(m_server);
  bzero(&m_server,length);
  m_server.sin_family=AF_INET;
  m_server.sin_addr.s_addr=INADDR_ANY;
  m_server.sin_port=htons(port);
  if (bind(m_sock,(struct sockaddr *)&m_server,length)<0) 
    error("[dataserver] binding");
}

int dataserver::openSocket(int32_t port){
  int res = 0;
  // this will port from 'port' opened on the *localhost* server
  m_sock=socket(AF_INET, SOCK_DGRAM, 0);
  if (m_sock < 0 )  error("Opening socket");
  length = sizeof(m_server);
  bzero(&m_server,length);
  m_server.sin_family=AF_INET;
  m_server.sin_addr.s_addr=INADDR_ANY;
  m_server.sin_port=htons(port);
  if (bind(m_sock,(struct sockaddr *)&m_server,length)<0) 
    error("[dataserver] binding");
  
  return res;
}

void dataserver::listen(bool unlimited){
  /*NAIVELY printout what you hear from ur client all in char*/
  //int buf_counter;
  char buf[90744];
  
  socklen_t fromlen;
  struct sockaddr_in from;
  fromlen = sizeof(struct sockaddr_in);
  
  do{
    puts("[Naive listen]\n [recvfrom] header or other info case");
    int buf_counter = recvfrom(m_sock, buf, 90744,0,
			   (struct sockaddr *)&from, &fromlen);
    if (buf_counter < 0){
      error("recvfrom");
      break;
    }
    
    puts("Received a datagram: ");
    puts(buf);
    printf("\n\t==> How long I am? %d\n", buf_counter);
  }while(unlimited);
  puts("[Naive listen finished]");
}

void dataserver::dataHeadTail (uint32_t size )
{
  
  //uint32_t size;
  //std::memcpy(&size, m_charbuf, sizeof size);
  
  //if (buflost) buflost = false;
  
  if (size == 0xFFFFFFFF) {
    cout<<"\n\t [Info] I found a buffer tail! => Next wait for a header!"<<endl;
    int size_exp=0;
    if (m_datatype==0) {cout<< "[rawData]";size_exp=m_datasize*4;}
    else if ( m_datatype<6 && m_datatype>0) {cout<<"[xmlData]";size_exp = m_datasize;}
    else cout<<"@0@ buggy!"<<endl;
    cout <<" => Cumulated size => "<< sizeReComBuf
	 << "; (expected) size => "<< size_exp <<endl;
    
    getaheader = false;
    sizeReComBuf=0;// recomb buffer size set to 0;
    m_datatype =6;
    m_datasize =0;
    
  }else{
    
    cout<<"\n\t [Info] I found a header! => Next wait for a data!"<<endl;
    getaheader=true;
    m_datasize = (size & 0x0FFFFFFF);
    m_datatype = (size>>28) & 0xF;
    
    
    cout<<"\n Size: "<< std::hex << size  <<endl;
    cout<<"\n dataSize: "<< std::hex << m_datasize <<"\n dataType: "
	    << std::dec<< m_datatype <<endl;
    
    if (m_datatype>=0 && m_datatype<6) {
      if (dataFileFd_>0) write(dataFileFd_, &size, 4);  /*xml/raw data*/
    }	  
    else  puts("[WARN] TOCHECK: header not for any data to write!");  /*other data*/	    
  }

}

//--------------------------------------------------------------------------------//
int dataserver::poll(int &evt){
     
  sizeReComBuf=0;
  buf_counter=0;
  
  getaheader=false;
  //buflost = false;
  m_datasize=0;
  m_datatype=6;
  
  /*output file descriptor*/
  string kpixfile("./server_kpixNetData.bin");
  dataFileFd_ = ::open(kpixfile.c_str(),O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  
  if (dataFileFd_<0){
    error("Error -> output bin file corrupted, check!");
    return (1);
  }

  while (1) {
    if (getaheader && m_datasize!=0){
      puts("[recvfrom] xml/rawdata case");
      
      switch (m_datatype) {
      case RawData :
	puts("\t[rawdata]\n");
	puts("KPiX RX: ");

	/*loop over all data buffer bunches sent under the n-1 buffer(header)*/
	m_databuf = new uint32_t[m_datasize*4];
	while( (buf_counter = recvfrom(m_sock,m_databuf,m_datasize*4,0,(struct sockaddr *)&from,&fromlen)) != 4 ) {
	  if (buf_counter < 0) error("recvfrom");

	  if (m_debug) write(1, m_databuf,buf_counter );
	  write(dataFileFd_, m_databuf, buf_counter);
	  sizeReComBuf+=buf_counter;

	  if (buf_counter!=m_datasize*4) // workpoint
	    printf("[Warn] Hmm... buf_counter (%d) != datasize*4 (%d)\n", buf_counter, m_datasize*4);
	}

	/*if i found a header/tail w/ fixed length of 4*/
	if (buf_counter==4){
	  //buflost = true;
	  if (sizeReComBuf==0) cout<< "\n\t [WARN] ==> I found missing oversized [rawData]! <=="<<endl;
	  else{ // this shall be a kpix cycle tail
	    evt++;
	    cout<< "\n @_@ COUNTER! Evt #"<< evt <<endl;
	  }
	  uint32_t buf_headtail;
	  std::memcpy(&buf_headtail, m_databuf, sizeof buf_headtail); // to be checked due to size
	  dataHeadTail(buf_headtail);
	  break;
	}
	break;
	
      case XmlConfig:   
      case XmlStatus:      
      case XmlRunStart:  
      case XmlRunStop:	   
      case XmlRunTime:
	puts("\t[xml]");
	puts("KPiX RX: \n");
	
	/*loop over all xml buffer bunches sent under the n-1 buffer(header)*/
	m_charbuf = new char[m_datasize];
	while ( (buf_counter = recvfrom(m_sock, m_charbuf, m_datasize, 0,(struct sockaddr *)&from,&fromlen)) != 4 ){
	  if (buf_counter < 0) error("recvfrom");
	  
	  if (m_debug) puts(m_charbuf);
	  write(dataFileFd_, m_charbuf, buf_counter);
	  sizeReComBuf+=buf_counter;

	  if (buf_counter!=m_datasize) // workpoint
	    printf("\n[Error] Hmm... buf_counter (%d) != xmlsize (%d)\n\t further check data type => %d\n",buf_counter, m_datasize, m_datatype);
	  
	}

	/*if i found a header/tail w/ fixed length of 4*/
	if (buf_counter==4) {
	  if (sizeReComBuf==0)
	    cout<< "\n\t [WARN] ==> I found missing oversized [xml]! <=="<<endl;
	  uint32_t buf_headtail;
	  std::memcpy(&buf_headtail, m_charbuf, sizeof buf_headtail);
	  dataHeadTail(buf_headtail);
	  break;
	}
	break;
	
      default:
	puts("unknown type");
	break;
      }
      
      
      printf("\n\t==> How long I am? %d\n", buf_counter);
      // m_datasize = 0;
      
    }else { 
      m_charbuf = new char[1024];
      puts("[recvfrom] header/tail or other cases");
      buf_counter = recvfrom(m_sock, m_charbuf, 1024,0,
			     (struct sockaddr *)&from,&fromlen);
      if (buf_counter < 0) error("recvfrom");
      
      if (m_debug){
	puts("Received a datagram: ");
	puts(m_charbuf);
      }
      printf("\n\t==> How long I am? %d\n", buf_counter);

      uint32_t buf_headtail;
      std::memcpy(&buf_headtail, m_charbuf, sizeof buf_headtail);

      if ( buf_headtail == 0xABABABAB ){
	cout<<"\t == Got client CLOSING signal, closing now =="<<endl;
	break;
      } else {
	dataHeadTail(buf_headtail);
      }
            
      if (buf_counter!=4) getaheader = false; // safety check in case any other information parsed
    }
    
  
    
  }// end of while loop

  /* ending */
  ::close(dataFileFd_);
  dataFileFd_ = -1;
  close(m_sock);
  
  return 0;
}



/* //--------------------------------------------------------------------------------// */
/* int main(int argc, char * argv[]){ */
/*  // port given from argv[1]: */
/*   if (argc < 2) { */
/*     fprintf(stderr, "ERROR, no port provided\n"); */
/*     exit(0); */
/*   } */

/*   dataserver mysr(atoi(argv[1])); */
/*   mysr.listen(false); */
/*   mysr.poll(); */
/*   return(0); */
  
/*   //int sock = mysr.getSocket(); */
/*   //mysr.listen(); */
/*   //return(0); */
  
/* } */

void error(const char *msg){
  perror(msg);
  exit(0);
}

