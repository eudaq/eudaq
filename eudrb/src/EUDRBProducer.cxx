#include "eudaq/Producer.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <cctype>

// stuff for VME/C-related business
//#include <sys/ioctl.h>          //ioctl()
//#include <unistd.h>             //close() read() write()
//#include <sys/types.h>          //open()
//#include <sys/stat.h>           //open()
//#include <fcntl.h>              //open()
//#include <stdlib.h>             //strtol()
//#include <stdio.h>
//#include <string.h>
//#include <errno.h>
//#include <sys/time.h>

//#include "vmedrv.h"
#include "eudrblib.hh"
//#include "vmelib.h"

#include "VMEInterface.hh"

#define DMABUFFERSIZE   0x100000                               //MBLT Buffer Dimension

using eudaq::EUDRBEvent;
using eudaq::to_string;

// VME stuff, not very clean like this
//int fdOut;                                                                      //File descriptor of the dev_m#
//unsigned long int readdata32=0;                         //For Longword Read
//unsigned long int i=0;
//unsigned long int address=0;                            //VME address
//unsigned long int BaseAddress=0x30000000;       //EUDRB VME Base address
//unsigned long int number_of_bytes, total_bytes;                      //Number of byte for a MBLT read
//unsigned long int * mblt_dstbuffer=NULL, * temp_buffer=NULL;

class EUDRBProducer : public eudaq::Producer {
public:
  EUDRBProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      done(false),
      started(false),
      n_error(0),
      buffer(DMABUFFERSIZE),
      //fdOut(open("/dev/vme_m0", O_RDWR)),
      m_idoffset(0)
    {
      //if (fdOut < 0) {
      //  EUDAQ_THROW("Open device failed Errno = " + to_string(errno));
      //}
      //if (getMyInfo() != 0) {
      //  EUDAQ_THROW("getMyInfo failed.  Errno = " + to_string(errno));
      //}
    }
  void Process() {
    if (!started) {
      eudaq::mSleep(1);
      return;
    }

    struct timeval starttime, stoptime;
    struct timeval starttime2, stoptime2;
    //struct timeval starttime3, stoptime3;
    //struct timeval starttime4, stoptime4;
    gettimeofday(&starttime2,0);

    boards[0].EventDataReady_wait();

    unsigned long total_bytes=0;

    EUDRBEvent ev(m_run, ++m_ev);

    for (size_t n_eudrb=0;n_eudrb<boards.size();n_eudrb++) {

      unsigned long readdata32=0;
      //unsigned long address=boards[n_eudrb].BaseAddress|0x00400004;
      gettimeofday(&starttime,0);
      int i=0;
      bool badev=false;
      while ((readdata32 & 0x80000000) != 0x80000000) { // be sure that each board is really ready
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        readdata32 = boards[n_eudrb].Read(0x00400004);
        i++;
        if (i%200==0)  printf("waiting for ready %d cycles\n",i);
        if (i == 2000) break;
      }

      gettimeofday(&stoptime,0);
      stoptime.tv_usec-=starttime.tv_usec;
      stoptime.tv_sec-=starttime.tv_sec;
      if (m_ev<10 || m_ev%20==0) printf("Waiting for board %ld took %ld us\n",(long)n_eudrb,stoptime.tv_sec*1000000+stoptime.tv_usec );
      unsigned long number_of_bytes=(readdata32&0xfffff)*4; //last 20 bits
      //      printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes!=0) {
        /*
         *      MBLT READ from the âZSDataReadPortâ located
         *                      at the board BASE address+0x400000
         */
        gettimeofday(&starttime,0);
        //address=boards[n_eudrb].BaseAddress|0x00400000;

        if (number_of_bytes>405520 || number_of_bytes==0) {
          printf("Board: %ld, Event: %d, number of bytes = %ld\n",(long)n_eudrb,m_ev,number_of_bytes);
          EUDAQ_WARN("Board "  + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " too big or zero: " + to_string(number_of_bytes));
          badev=true;
        }
        //      printf("mblt before: %ld\n",mblt_dstbuffer);
        //        printf("MBLT PRIMA !!!\n");
        // pad number of bytes to multiples of 8 in case of ZS
        buffer.resize(((number_of_bytes+7)&~7) / 4);
        //vme_A32_D32_User_Data_MBLT_read((number_of_bytes+7)&~7,address,&buffer[0]);
        boards[n_eudrb].ReadBlock(0x00400000, buffer);
        //        printf("MBLT DOPO!!!\n");
        gettimeofday(&stoptime,0);
        stoptime.tv_usec-=starttime.tv_usec;
        stoptime.tv_sec-=starttime.tv_sec;
        if (m_ev<10 || m_ev%20==0) printf("MBLT took %ld us for %ld words\n",
                                          stoptime.tv_sec*1000000+stoptime.tv_usec,
                                          (long)buffer.size());

        // Reset the boards here, after the MBLT
        //unsigned long address=boards[n_eudrb].BaseAddress|0x10;
        //unsigned long readdata32=0xC0000000;
        if (n_eudrb==boards.size()-1) usleep(10000); // temporary fix
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
//         eudaq::mSleep(500);
//         std::cout << "DEBUG: testing read" << std::endl;
//         unsigned long ddd = boards[n_eudrb].Read(0);
//         std::cout << "DEBUG: testing write" << std::endl;
//         boards[n_eudrb].Write(0, ddd);
//         std::cout << "DEBUG: OK" << std::endl;
        boards[n_eudrb].Write(0x10, 0xC0000000);

        //      for(int j=0;j<number_of_bytes/4;j++)
        if (m_ev<10 || m_ev%20==0 || badev) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n",buffer[0]);
          //    printf("buf&0x54... = 0x%lx\n",(buffer[number_of_bytes/4-2]&0x54000000));
          if ( (buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n",buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n",buffer[number_of_bytes/4-3]);
          badev=false;
        }

        /*for(int j=0;j<10;j++)
          {
          printf("word=%d %lx\n",j,mblt_dstbuffer[j]);
          }*/
        //      number_of_bytes=0;
        gettimeofday(&starttime,0);
        ev.AddBoard(n_eudrb,&buffer[0], number_of_bytes);
        total_bytes+=(number_of_bytes+7)&~7;
        gettimeofday(&stoptime,0);
        stoptime.tv_usec-=starttime.tv_usec;
        stoptime.tv_sec-=starttime.tv_sec;
        if (m_ev<10 || m_ev%20==0) printf("Addboard took %ld us\n",stoptime.tv_sec*1000000+stoptime.tv_usec );
      }
    }
    //mblt_dstbuffer=temp_buffer;

    //            for (size_t n_eudrb=0;n_eudrb<boards.size();n_eudrb++) {
    //              // Reset the boards, here is critical and causing hangups, should do above!
    //              unsigned long address=boards[n_eudrb].BaseAddress|0x10;
    //              unsigned long readdata32=0xC0000000;
    //              //      if (n_eudrb==boards.size()-1) usleep(10000); // temporary fix
    //              vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
    //            }


    if (total_bytes) {
      gettimeofday(&starttime,0);
      //      printf("before sendevent\n");
      SendEvent(ev);
      gettimeofday(&stoptime,0);
      stoptime.tv_usec-=starttime.tv_usec;
      stoptime.tv_sec-=starttime.tv_sec;
      if (m_ev<10 || m_ev%20==0) printf("Sendevent took %ld us\n",stoptime.tv_sec*1000000+stoptime.tv_usec );
      //      printf("after sendevent\n");
      if (m_ev<10 || m_ev%20==0)
        printf("*** Event %d of length %ld sent!\n",m_ev, total_bytes);
      n_error=0;
    } else {
      n_error++;
      if (n_error<5) EUDAQ_ERROR("Event length 0");
      else if (n_error==5) EUDAQ_ERROR("Event length 0 repeated more than 5 times, skipping!");
    }
    gettimeofday(&stoptime2,0);
    stoptime2.tv_usec-=starttime2.tv_usec;
    stoptime2.tv_sec-=starttime2.tv_sec;
    if (m_ev<10 || m_ev%20==0) printf("*** Total took %ld us\n",stoptime2.tv_sec*1000000+stoptime2.tv_usec );

  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      boards.clear();
      for (int i = 0; i < numboards; ++i) {
        unsigned addr = param.Get("Board" + to_string(i) + ".Addr", "Addr", 0x8000000*(i+2));
        std::string mode = param.Get("Board" + to_string(i) + ".Mode", "Mode", "");
        boards.push_back(BoardInfo(addr, mode));
      }
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        //unsigned long address=boards[n_eudrb].BaseAddress;
        unsigned long readdata32 = boards[n_eudrb].Read(0);
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //readdata32|=0x80000000;
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //readdata32&=~(0x80000000);
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        boards[n_eudrb].Write(0, readdata32 | 0x80000000);
        readdata32 = boards[n_eudrb].Read(0);
        boards[n_eudrb].Write(0, readdata32 & ~0x80000000);
      }
      sleep(8);
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        //        EUDRB_Reset(fdOut, boards[n_eudrb].BaseAddress);
        unsigned long readdata32=0xD0000001;
        char mode[10];
        strcpy(mode,"Slave");
        if (n_eudrb==boards.size()-1) {
          readdata32=0xD0000000;
          strcpy(mode,"Master");
        }
        printf("Board: %ld is a %s\n",(long)n_eudrb,mode);
        //unsigned long address=boards[n_eudrb].BaseAddress|0x10;
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        boards[n_eudrb].Write(0x10, readdata32);
      }


      /*
       *        Setting Default Value for the Functional Control Status Register
       */

      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        std::cout << "EUDRB" << n_eudrb
                  <<", mode: " << boards[n_eudrb].mode << std::endl;
        boards[n_eudrb].EUDRB_CSR_Default();
        unsigned long /*address=boards[n_eudrb].BaseAddress,*/baseShift=0x800000;
        unsigned long /*readdata32=0,*/ newdata32=0;
        /* read address first and only set the reset bit */
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //newdata32=readdata32|0x40;
        //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        unsigned long data = boards[n_eudrb].Read(0);
        boards[n_eudrb].Write(0, data | 0x40);
        boards[n_eudrb].Write(0, data & ~0x40);

        // check for Zero Suppression
        if (boards[n_eudrb].zs) {
          boards[n_eudrb].EUDRB_ZS_On();
          //zs=true;


          printf("Downloading pedestals to board %ld, this takes 2-3 Minutes per board!\n",(long)n_eudrb);
          EUDAQ_INFO("Downloading pedestals to board"  + to_string(n_eudrb) +"be very patient");

          FILE *fp;
          char filename[80],dummy[100],fileno[1];

          int board,x,y,flag,subm,thresh2bit,ped2bit;
          float ped, thresh,sigma=2.0; // sigma is hardcoded for the moment
          sprintf(fileno,"%1ld",(long)n_eudrb);

          sprintf(filename,"../pedestal/ped%s.dat",fileno);
          // here we put in the uploading of pedestals:
          unsigned long offset=0x0;
          // VME is master of SRAM
          printf("Become Master of SRAM\n");
          //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
          //newdata32=readdata32|0x200;
          //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
          boards[n_eudrb].Write(0, boards[n_eudrb].Read(0) | 0x200);

          printf("Fill Matrices on board: %ld\n",(long)n_eudrb);

          if ( (fp = fopen(filename, "r") ) ) {
            printf("Opened file %s\n",filename);
            printf("\tDownloading pedestals to board: %ld\n",(long)n_eudrb);
            fgets(dummy,100,fp); fscanf(fp,"\n");
            printf("\t%s\n",dummy);
            fgets(dummy,100,fp); fscanf(fp,"\n");// skip 2 lines
            printf("\t%s\n",dummy);

            while (fscanf(fp,"%d %d %d %f %f %d\n",&board,&x,&y,&ped,&thresh,&flag)!=EOF) {
              //              printf("I read: board: %1d, x: %3d, y: %3d, ped: %2.3f, thresh: %2.3f, flag: %2d\n",board,x,y,ped,thresh,flag);

              subm=(x>>6);
              x=x%64;
              offset=((x+2)+(y<<7)+(subm<<18))<<2; // x+2, because 0 and 1 are dummy pixels
              unsigned long address=/*boards[n_eudrb].BaseAddress+*/baseShift+offset;
              thresh2bit=(int) (thresh*sigma); // prepare for 2bits complement
              ped2bit=(int) ped&0x1f; // prepare for 2bits complement

              //              newdata32=0xc;
              if (thresh2bit>31) thresh2bit=31;
              if (thresh2bit<-32) thresh2bit=-32;
              if (ped2bit>31) ped2bit=31;
              if (ped2bit<-32) ped2bit=-32;
              newdata32=thresh2bit;
              // temporary
              //newdata32=4;
              newdata32=newdata32+(ped2bit<<6);

              if (flag) newdata32=0x1f+(1<<11); // mask bad pixels as good as you can (high thresh and very low ped)


//            if ((y<2 || y>253) && (x<4||x>61)) {
//              printf("\tsubm=%2d,y=%3d,x=%3d,address=0x%8lx,data=0x%3lx,thresh2bit=%3d\n",subm,y,x,address,newdata32,thresh2bit);
//            }

              //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
              boards[n_eudrb].Write(address, newdata32);
            }
            fclose(fp);
          } else {
            EUDAQ_ERROR(std::string("Unable top open pedestal file ") + filename);
          }
          // now black out the dummy pixels
          //printf("\tMasking dummy pixels!\n");
          //newdata32=0x1f+(1<<11); // mask dummy pixels as good as you can (high thresh and very low ped)
//           for (int subm=0;subm<4;subm++) {
//             for (int y=0;y<256;y++) {
//               for (int x=0;x<2;x++) {
//                 offset=(x+(y<<7)+(subm<<18))<<2;
//                 address=/*boards[n_eudrb].BaseAddress+*/baseShift+offset;
// //                if ((y<3 || y>252) && (x<2))
// //                printf("\tsubm=%2d,y=%3d,x=%3d,address=0x%8lx,data=0x%lx\n",subm,y,x,address,newdata32);
// //              vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
//               }
//             }
//           }
          printf("\tdone!\n");



          // Release master of SRAM
          printf("\tRelease Master of SRAM\n");
          //address=boards[n_eudrb].BaseAddress;
          //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
          //newdata32=readdata32&~0x200;
          //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
          boards[n_eudrb].Write(0, boards[n_eudrb].Read(0) & ~0x200);
          // reset once more to be sure
          /* read address first and only set the reset bit */
          //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
          //newdata32=readdata32|0x40;
          //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
          //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
          unsigned long data = boards[n_eudrb].Read(0);
          boards[n_eudrb].Write(0, data | 0x40);
          boards[n_eudrb].Write(0, data & ~0x40);
          EUDAQ_INFO("Board" + to_string(n_eudrb) + "done!");

        } else {
          boards[n_eudrb].EUDRB_ZS_Off();
          //zs=false;
        }

      }

      for (size_t n_eudrb=0; n_eudrb < boards.size(); n_eudrb++) {
        //unsigned long address=boards[n_eudrb].BaseAddress|0x10;
        unsigned long readdata32=0xC0000000;
        /* read address first and only set the reset bit */
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        boards[n_eudrb].Write(0x10, readdata32);
      }
      std::cout << "...Configured (" << param.Name() << ")" << std::endl;

      EUDAQ_INFO("Configured (" + param.Name() + ")");
      SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Configuration Error");
    }
  }
  virtual void OnStartRun(unsigned param) {
    try {
      m_run = param;
      m_ev = 0;
      std::cout << "Start Run: " << param << std::endl;
      // EUDRB startup (activation of triggers etc)
      EUDRBEvent ev(EUDRBEvent::BORE(m_run));
      ev.SetTag("DET",  "MIMOTEL");
      std::string mode = boards[0].mode;
      for (size_t i = 1; i < boards.size(); ++i) {
        if (boards[i].mode != mode) mode = "Mixed";
      }
      ev.SetTag("MODE", mode);
      for (size_t i = 0; i < boards.size(); ++i) {
        ev.SetTag("MODE" + to_string(i), boards[i].mode);
      }
      ev.SetTag("BOARDS", to_string(boards.size()));
      //ev.SetTag("ROWS", to_string(256))
      //ev.SetTag("COLS", to_string(66))
      //ev.SetTag("MATS", to_string(4))
      for (size_t i = 0; i < boards.size(); ++i) {
        ev.SetTag("ID" + to_string(i), to_string(i + m_idoffset));
      }
      SendEvent(ev);
      started=true;
      SetStatus(eudaq::Status::LVL_OK, "Started");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Start Error");
    }
  }
  virtual void OnStopRun() {
    try {
      std::cout << "Stop Run" << std::endl;
      // EUDRB stop
      started=false;
      sleep(2); // fix for the moment
      SendEvent(EUDRBEvent::EORE(m_run, ++m_ev));
      SetStatus(eudaq::Status::LVL_OK, "Stopped");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Stop Error");
    }
  }
  virtual void OnTerminate() {
    std::cout << "Terminating..." << std::endl;
    done = true;
  }
  virtual void OnReset() {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Reset" << std::endl;
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        //unsigned long readdata32;
        //unsigned long address=boards[n_eudrb].BaseAddress;
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //readdata32|=0x80000000;
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        //vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //readdata32&=~(0x80000000);
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        unsigned long data = boards[n_eudrb].Read(0);
        boards[n_eudrb].Write(0, data | 0x80000000);
        data = boards[n_eudrb].Read(0);
        boards[n_eudrb].Write(0, data & ~0x80000000);
      }
      sleep(8);
      SetStatus(eudaq::Status::LVL_OK, "Reset");
    } catch (const std::exception & e) {
      printf("Caught exception: %s\n", e.what());
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    } catch (...) {
      printf("Unknown exception\n");
      SetStatus(eudaq::Status::LVL_ERROR, "Reset Error");
    }
  }
  virtual void OnStatus() {
    //std::cout << "Status " << m_status << std::endl;
  }
  virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Unrecognised command");
  }

  unsigned m_run, m_ev;
  bool done, started;
  int n_error;
  std::vector<unsigned long> buffer;
  //std::vector<unsigned long> buffer;
  std::vector<BoardInfo> boards;
  //int fdOut;
  int m_idoffset;
};

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ EUDRB Producer", "1.0", "The Producer task for reading out EUDRB boards via VME");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:7000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "EUDRB", "string",
                                   "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    EUDRBProducer producer(name.Value(), rctrl.Value());

    unsigned evtno;
    evtno=0;
    do {
      producer.Process();
      //      usleep(100);
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
