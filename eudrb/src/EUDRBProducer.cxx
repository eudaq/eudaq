#include "eudaq/Producer.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <cctype>

// stuff for VME/C-related business
#include <sys/ioctl.h>          //ioctl()
#include <unistd.h>             //close() read() write()
#include <sys/types.h>          //open()
#include <sys/stat.h>           //open()
#include <fcntl.h>              //open()
#include <stdlib.h>             //strtol()
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include "vmedrv.h"
#include "eudrblib.h"
#include "vmelib.h"

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
      fdOut(open("/dev/vme_m0", O_RDWR)),
      m_idoffset(0)
    {
      if (fdOut < 0) {
        EUDAQ_THROW("Open device failed Errno = " + to_string(errno));
      }
      if (getMyInfo() != 0) {
        EUDAQ_THROW("getMyInfo failed.  Errno = " + to_string(errno));
      }
    }
  void Process() {
    if (!started) {
      eudaq::mSleep(1);
      return;
    }

    EventDataReady_wait(fdOut,boards[0].BaseAddress);

    unsigned long total_bytes=0;

    EUDRBEvent ev(m_run, ++m_ev);

    for (size_t n_eudrb=0;n_eudrb<boards.size();n_eudrb++) {

      unsigned long readdata32=0;
      unsigned long address=boards[n_eudrb].BaseAddress|0x00400004;
      while ((readdata32&0x80000000)!=0x80000000) { // be sure that each board is really ready
        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
      }
      unsigned long number_of_bytes=(readdata32&0xfffff)*4; //last 20 bits
      //      printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes!=0) {
        /*
         *      MBLT READ from the âZSDataReadPortâ located
         *                      at the board BASE address+0x400000
         */
        address=boards[n_eudrb].BaseAddress|0x00400000;

        //      printf("mblt before: %ld\n",mblt_dstbuffer);
        //        printf("MBLT PRIMA !!!\n");
        // pad number of bytes to multiples of 8 in case of ZS
        vme_A32_D32_User_Data_MBLT_read((number_of_bytes+7)&~7,address,&buffer[0]);
        //        printf("MBLT DOPO!!!\n");

        //      for(int j=0;j<number_of_bytes/4;j++)
        if (m_ev<20 || m_ev%10==0) {
          printf("event   =0x%x, eudrb   =%3d, nbytes = %ld\n",m_ev,(int)n_eudrb,number_of_bytes);
          printf("\theader =0x%lx\n",buffer[0]);
          //    printf("buf&0x54... = 0x%lx\n",(buffer[number_of_bytes/4-2]&0x54000000));
          if ( (buffer[number_of_bytes/4-2]&0x54000000)==0x54000000) printf("\ttrailer=0x%lx\n",buffer[number_of_bytes/4-2]);
          else printf("\ttrailer=x%lx\n",buffer[number_of_bytes/4-3]);
        }

        /*for(int j=0;j<10;j++)
          {
          printf("word=%d %lx\n",j,mblt_dstbuffer[j]);
          }*/
        ev.AddBoard(n_eudrb,&buffer[0], number_of_bytes);
        total_bytes+=(number_of_bytes+7)&~7;
        //      printf("mblt after: %ld\n",mblt_dstbuffer);
      }
    }
    //mblt_dstbuffer=temp_buffer;
    if (total_bytes) {
      printf("before sendevent\n");
      SendEvent(ev);
      printf("after sendevent\n");
      if (m_ev<20 || m_ev%10==0)
        printf("*** Event %d of length %ld sent!\n",m_ev, total_bytes);
      n_error=0;
    } else {
      n_error++;
      if (n_error<5) EUDAQ_ERROR("Event length 0");
      else if (n_error==5) EUDAQ_ERROR("Event length 0 repeated more than 5 times, skipping!");
    }


    for (size_t n_eudrb=0; n_eudrb < boards.size(); n_eudrb++) {
      unsigned long address=boards[n_eudrb].BaseAddress;
      unsigned long readdata32=0, newdata32=0;
      /* read address first and only set the reset bit */
      vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
      newdata32=readdata32|0x40;
      vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
      vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
//      EUDRB_TriggerProcessingUnit_Reset(fdOut, BaseAddress[n_eudrb]);
    }
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      boards.clear();
      for (int i = 0; i < numboards; ++i) {
        unsigned addr = param.Get("Board" + to_string(i) + ".Addr", "Addr", 0x8000000*(i+2));
        std::string mode = param.Get("Board" + to_string(i) + ".Mode", "Mode", "");
        boards.push_back(BoardInfo(addr, mode));
      }

      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        EUDRB_Reset(fdOut, boards[n_eudrb].BaseAddress);
      }
      sleep(8);
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long readdata32=0xD0000000;
        unsigned long address=boards[n_eudrb].BaseAddress|0x10;
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
      }


      /*
       *        Setting Default Value for the Functional Control Status Register
       */

      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        std::cout << "EUDRB" << n_eudrb << "at address: 0x" << std::hex << boards[n_eudrb].BaseAddress
                  <<", mode: " << boards[n_eudrb].mode << std::dec << std::endl;
        EUDRB_CSR_Default(fdOut,boards[n_eudrb].BaseAddress);
        unsigned long address=boards[n_eudrb].BaseAddress;
        unsigned long readdata32=0, newdata32=0;
        /* read address first and only set the reset bit */
        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        newdata32=readdata32|0x40;
        vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
//        EUDRB_TriggerProcessingUnit_Reset(fdOut, BaseAddress[n_eudrb]);

        // read length of Pixels
        //      unsigned long int readdata32=0x41ff;
        //        unsigned long address=BaseAddress[n_eudrb]|0x00000004;
        //        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address); usleep(100000);
        //        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        //      printf("Size of Pixels (Mimotel or Mimo*2): 0x%lx\n",readdata32);


        // check for Zero Suppression
        if (boards[n_eudrb].zs) {
          EUDRB_ZS_On(fdOut,boards[n_eudrb].BaseAddress);
          //zs=true;
        } else {
          EUDRB_ZS_Off(fdOut,boards[n_eudrb].BaseAddress);
          //zs=false;
        }

      }

      SetStatus(eudaq::Status::LVL_OK, "Configured");
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
      // EUDRB reset
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        EUDRB_Reset(fdOut, boards[n_eudrb].BaseAddress);
      }
      sleep(8);
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long readdata32=0xD0000000;
        unsigned long address=boards[n_eudrb].BaseAddress|0x10;
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
      }
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        printf("address: %lx\n",boards[n_eudrb].BaseAddress);
        EUDRB_CSR_Default(fdOut,boards[n_eudrb].BaseAddress);
        unsigned long address=boards[n_eudrb].BaseAddress;
        unsigned long readdata32=0, newdata32=0;
        /* read address first and only set the reset bit */
        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        newdata32=readdata32|0x40;
        vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address);
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
//        EUDRB_TriggerProcessingUnit_Reset(fdOut, BaseAddress[n_eudrb]);
      }
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

  struct BoardInfo {
    BoardInfo(unsigned long addr, const std::string & mode) : BaseAddress(addr), mode(mode), zs(mode == "ZS") {}
    unsigned long BaseAddress;
    std::string mode;
    bool zs;
  };

  unsigned m_run, m_ev;
  bool done, started;
  int n_error;
  std::vector<unsigned long> buffer;
  std::vector<BoardInfo> boards;
  int fdOut;
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
