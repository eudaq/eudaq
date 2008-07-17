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
      juststopped(false),
      n_error(0),
      buffer(DMABUFFERSIZE),
      fdOut(open("/dev/vme_m0", O_RDWR)),
      m_idoffset(0),
      m_version(-1)
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

    struct timeval starttime, stoptime;
    struct timeval starttime2, stoptime2;
    //struct timeval starttime3, stoptime3;
    //struct timeval starttime4, stoptime4;
    gettimeofday(&starttime2,0); 

    unsigned long total_bytes=0;

    EUDRBEvent ev(m_run, m_ev+1);

    for (size_t n_eudrb=0;n_eudrb<boards.size();n_eudrb++) {

      //unsigned long readdata32=0;
      unsigned long address=boards[n_eudrb].BaseAddress|0x00400004;
      gettimeofday(&starttime,0);
      //int i=0;
      bool badev=false;
      //while ((readdata32&0x80000000)!=0x80000000) { // be sure that each board is really ready
      //  vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
      //  i++;
      //  if (i%2000==0)  printf("waiting for ready %d cycles\n",i);
      //}
      int datasize = EventDataReady_size(fdOut, boards[n_eudrb].BaseAddress);
      if (datasize == 0 && n_eudrb == 0) {
        if (juststopped) started = false;
        break;
      }

      gettimeofday(&stoptime,0);
      stoptime.tv_usec-=starttime.tv_usec;
      stoptime.tv_sec-=starttime.tv_sec;
/*      if (m_ev<10 || m_ev%100==0) printf("Waiting for board %d took %ld us\n",n_eudrb,stoptime.tv_sec*1000000+stoptime.tv_usec );*/
      unsigned long number_of_bytes=datasize*4;
      //      printf("number of bytes = %ld\n",number_of_bytes);
      if (number_of_bytes!=0) {
        /*
         *      MBLT READ from the âZSDataReadPortâ located
         *                      at the board BASE address+0x400000
         */
        gettimeofday(&starttime,0);
        address=boards[n_eudrb].BaseAddress|0x00400000;

        if (number_of_bytes==0) {
          printf("Board: %d, Event: %d, empty\n",(int)n_eudrb,m_ev);
          EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " empty");
          badev=true;
        } else if (number_of_bytes>1048576) {
          printf("Board: %d, Event: %d, number of bytes = %ld\n",(int)n_eudrb,m_ev,number_of_bytes);
          EUDAQ_WARN("Board " + to_string(n_eudrb) +" in Event " + to_string(m_ev) + " too big: " + to_string(number_of_bytes));
          badev=true;
        }
        //      printf("mblt before: %ld\n",mblt_dstbuffer);
        //        printf("MBLT PRIMA !!!\n");
        // pad number of bytes to multiples of 8 in case of ZS
        //unsigned long debug_before, debug_after;
        //vme_A32_D32_User_Data_SCT_read(fdOut, &debug_before, boards[n_eudrb].BaseAddress|0x00400004);
        vme_A32_D32_User_Data_MBLT_read((number_of_bytes+7)&~7,address,&buffer[0]);
        //vme_A32_D32_User_Data_SCT_read(fdOut, &debug_after, boards[n_eudrb].BaseAddress|0x00400004);
        //std::cout << "DEBUG " << m_ev << ", " << n_eudrb << ", "
        //          << eudaq::hexdec(debug_before) << ", " << eudaq::hexdec(debug_after)
        //          << std::endl;
        if (number_of_bytes > 16) ev.SetFlags(eudaq::Event::FLAG_HITS);
        //        printf("MBLT DOPO!!!\n");
        gettimeofday(&stoptime,0);
        stoptime.tv_usec-=starttime.tv_usec;
        stoptime.tv_sec-=starttime.tv_sec;
        if (m_ev<10 || m_ev%100==0) printf("MBLT took %ld us\n",stoptime.tv_sec*1000000+stoptime.tv_usec );

        // Reset the boards here, after the MBLT
        //address=boards[n_eudrb].BaseAddress|0x10;
        //unsigned long readdata32=0xC0000000;
        //      if (n_eudrb==boards.size()-1) usleep(10000); // temporary fix
        //vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);

        //      for(int j=0;j<number_of_bytes/4;j++)
        if (m_ev<10 || m_ev%100==0 || badev) {
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
        if (m_ev<10 || m_ev%100==0) printf("Addboard took %ld us\n",stoptime.tv_sec*1000000+stoptime.tv_usec );
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
      ++m_ev;
      SendEvent(ev);
      gettimeofday(&stoptime,0);
      stoptime.tv_usec-=starttime.tv_usec;
      stoptime.tv_sec-=starttime.tv_sec;
      if (m_ev<10 || m_ev%100==0) printf("Sendevent took %ld us\n",stoptime.tv_sec*1000000+stoptime.tv_usec );
      //      printf("after sendevent\n");
      if (m_ev<10 || m_ev%100==0)
        printf("*** Event %d of length %ld sent!\n",m_ev, total_bytes);
      n_error=0;
      gettimeofday(&stoptime2,0);
      stoptime2.tv_usec-=starttime2.tv_usec;
      stoptime2.tv_sec-=starttime2.tv_sec;
      if (m_ev<10 || m_ev%100==0) printf("*** Total took %ld us\n",stoptime2.tv_sec*1000000+stoptime2.tv_usec );
    }
  }
  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;
      int numboards = param.Get("NumBoards", 0);
      m_idoffset = param.Get("IDOffset", 0);
      m_version = param.Get("Version", -1);
      boards.clear();
      for (int n_eudrb = 0; n_eudrb < numboards; ++n_eudrb) {
        unsigned addr = param.Get("Board" + to_string(n_eudrb) + ".Addr", "Addr", 0);
        std::string mode = param.Get("Board" + to_string(n_eudrb) + ".Mode", "Mode", "");
        std::string det = param.Get("Board" + to_string(n_eudrb) + ".Det", "Det", "MIMOTEL");
        boards.push_back(BoardInfo(addr, mode, det));
      }
      bool unsync = param.Get("Unsynchronized", 0);
      std::cout << "Running in " << (unsync ? "UNSYNCHRONIZED" : "synchronized") << " mode" << std::endl;
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long address=boards[n_eudrb].BaseAddress;
        unsigned long data = 0;
        if (boards[n_eudrb].zs) data |= 0x20; // ZS mode
        if (n_eudrb==boards.size()-1 || unsync) data |= 0x2000; // Internal Timing
        vme_A32_D32_User_Data_SCT_write(fdOut, 0x80000000, address); // Reset
        vme_A32_D32_User_Data_SCT_write(fdOut, data, address); // Set mode
        unsigned long mimoconf = 0x48d00000;
        if (m_version == 1) {
          if (boards[n_eudrb].det != "MIMOTEL") EUDAQ_THROW("EUDRB Version 1 only reads MIMOTEL (not "
                                                            + boards[n_eudrb].det + ")");
          mimoconf |= (1 << 16);
        } else if (m_version == 2) {
          const int adcdelay = 0x7 & param.Get("Board" + to_string(n_eudrb) + ".AdcDelay", "AdcDelay", 2);
          const int clkselect = 0xf & param.Get("Board" + to_string(n_eudrb) + ".ClkSelect", "ClkSelect", 1);
          unsigned long reg01 = 0x00ff2000;
          unsigned long reg23 = 0x0000000a;
          unsigned long reg45 = 0x8040001f | (adcdelay << 8) | (clkselect << 12);
          unsigned long reg6  = 0;
          int pdrd =  param.Get("Board" + to_string(n_eudrb) + ".PostDetResetDelay", "PostDetResetDelay", -1);
          if (boards[n_eudrb].det == "MIMOTEL") {
            reg01 |= 65;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 16896;
            mimoconf |= (1 << 16);
          } else if (boards[n_eudrb].det == "MIMOSA18") {
            reg01 |= 255;
            if (pdrd < 0) pdrd = 261;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 65535;
            mimoconf |= (3 << 16);
          } else if (boards[n_eudrb].det == "MIMOSTAR2") {
            reg01 |= 65;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 8448;
            mimoconf |= (2 << 16);
          } else if (boards[n_eudrb].det == "MIMOSA5") {
            reg01 |= 511;
            if (pdrd < 0) pdrd = 4;
            reg23 |= (0xfff & pdrd) << 16;
            reg6   = 0x3ffff;
          } else {
            EUDAQ_THROW("Unknown detector type: " + boards[n_eudrb].det);
          }
          vme_A32_D32_User_Data_SCT_write(fdOut, reg01, address+0x20);
          vme_A32_D32_User_Data_SCT_write(fdOut, reg23, address+0x24);
          vme_A32_D32_User_Data_SCT_write(fdOut, reg45, address+0x28);
          vme_A32_D32_User_Data_SCT_write(fdOut, reg6 , address+0x2c);
          vme_A32_D32_User_Data_SCT_write(fdOut, data, address);
        } else {
          EUDAQ_THROW("Must set Version = 1 or 2 in config file");
        }
        data = 0xd0000001;
        if (n_eudrb==boards.size()-1 || unsync) data = 0xd0000000;
        vme_A32_D32_User_Data_SCT_write(fdOut, data, address+0x10);
        eudaq::mSleep(100);
        int marker1 = param.Get("Board" + to_string(n_eudrb) + ".Marker1", "Marker1", -1);
        int marker2 = param.Get("Board" + to_string(n_eudrb) + ".Marker2", "Marker1", -1);
        if (marker1 >= 0 || marker2 >= 0) {
          if (marker1 < 0) marker1 = marker2;
          if (marker2 < 0) marker2 = marker1;
          std::cout << "Setting board " << n_eudrb << " markers to "
                    << eudaq::hexdec(marker1, 8) << ", " << eudaq::hexdec(marker2, 8) << std::endl;
          vme_A32_D32_User_Data_SCT_write(fdOut, 0x48100800 | (marker1 & 0xff), address+0x10);
          vme_A32_D32_User_Data_SCT_write(fdOut, 0x48100700 | (marker2 & 0xff), address+0x10);
        }
        vme_A32_D32_User_Data_SCT_write(fdOut, mimoconf, address+0x10);
      }
      std::cout << "Waiting for boards to reset..." << std::endl;

      for (int i = 0; i < 150; ++i) {
        eudaq::mSleep(100);
        bool waiting = false;
        for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
          unsigned long data = 0;
          vme_A32_D32_User_Data_SCT_read(fdOut, &data, boards[boards.size()-1].BaseAddress);
          if (data & 0x02000000) {
            waiting = true;
            break;
          }
        }
        if (!waiting) break;
      }
      std::cout << "OK" << std::endl;
// new loop added by Angelo
      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        unsigned long address=boards[n_eudrb].BaseAddress;
        unsigned long data = 0x40;
        if (boards[n_eudrb].zs) data |= 0x20;
        if (n_eudrb==boards.size()-1) data |= 0x2000;
        vme_A32_D32_User_Data_SCT_write(fdOut, data, address);
        data=0;
        if (boards[n_eudrb].zs) data |= 0x20;
        if (n_eudrb==boards.size()-1) data |= 0x2000;
        vme_A32_D32_User_Data_SCT_write(fdOut, data, address);
      }

      for (size_t n_eudrb = 0; n_eudrb < boards.size(); n_eudrb++) {
        // check for Zero Suppression
        if (boards[n_eudrb].zs) {
          int thresh = param.Get("Board" + to_string(n_eudrb) + ".Thresh", "Thresh", 10);
          int ped = param.Get("Board" + to_string(n_eudrb) + ".Ped", "Ped", 0);
          std::string fname = param.Get("Board" + to_string(n_eudrb) + ".PedestalFile", "PedestalFile", "");
          if (thresh < 0 && fname == "") {
            fname = "ped%.dat";
            EUDAQ_WARN("Config missing pedestal file name, using " + fname);
          }
          size_t n = fname.find('%');
          if (n != std::string::npos) {
            fname = std::string(fname, 0, n) + to_string(n_eudrb) + std::string(fname, n+1);
          }
          if (fname != "") fname = "../pedestal/" + fname;
          pedestal_t peds = (fname != "") ? ReadPedestals(fname)
            : GeneratePedestals(thresh, ped);
          LoadPedestals(boards[n_eudrb], peds);

          std::string msg = "Board " + to_string(n_eudrb) + " pedestals loading";
          if (fname != "") msg += " from file " + fname;
          else msg += " values = " + to_string(ped) + ", " + to_string(thresh);
          puts(msg.c_str());
          EUDAQ_INFO(msg);
        }
      }
      // END of loading pedestal loop

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
      std::string det = boards[0].det;
      for (size_t i = 1; i < boards.size(); ++i) {
        if (boards[i].det != det) det = "Mixed";
      }
      ev.SetTag("VERSION", to_string(m_version));
      ev.SetTag("DET", det);
      for (size_t i = 0; i < boards.size(); ++i) {
        ev.SetTag("DET" + to_string(i), boards[i].det);
      }
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
      eudaq::mSleep(100);
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
      std::cout << "Stopping Run" << std::endl;
      // EUDRB stop
      juststopped = true;
      while (started) {
        eudaq::mSleep(100);
      }
      juststopped = false;
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
        unsigned long readdata32;
        unsigned long address=boards[n_eudrb].BaseAddress;
        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        readdata32|=0x80000000;
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
        vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,address);
        readdata32&=~(0x80000000);
        vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,address);
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

  typedef  std::vector<unsigned> pedestal_t;
  pedestal_t GeneratePedestals(int thresh = 10, int ped = 0) const {
    return pedestal_t(1UL<<20, thresh | (ped << 6));
  }
  pedestal_t ReadPedestals(const std::string & filename) const {
    pedestal_t result(1UL<<20);
    FILE *fp;
    unsigned long /*readdata32,*/ newdata32, offset;
    int board,x,y,flag,subm,thresh2bit,ped2bit;
    float ped, thresh,sigma=2.0; // sigma is hardcoded for the moment
    //printf("Fill Matrices on board: %lx\n",address);
    char dummy[100];
    if ( (fp = fopen(filename.c_str(), "r") ) ) {
      printf("Opened file %s\n",filename.c_str());
      //printf("\tDownloading pedestals to board: %d\n",n_eudrb);
      fgets(dummy,100,fp); fscanf(fp,"\n");
      printf("\t%s\n",dummy);
      fgets(dummy,100,fp); fscanf(fp,"\n");// skip 2 lines
      printf("\t%s\n",dummy);
      
      while (fscanf(fp,"%d %d %d %f %f %d\n",&board,&x,&y,&ped,&thresh,&flag)!=EOF) {
        //printf("I read: board: %1d, x: %3d, y: %3d, ped: %2.3f, thresh: %2.3f, flag: %2d\n",board,x,y,ped,thresh,flag);
        subm=(x>>6);
        if (subm == 1 || subm == 2) subm = 3-subm; // fix for submatrix ordering
        x=x%64;
        offset=((x+2)+(y<<9)+(subm<<18))<<2; // x+2, because 0 and 1 are dummy pixels
        thresh2bit=(int) (thresh*sigma); // prepare for 2bits complement
        ped2bit=(int) ped&0x1f; // prepare for 2bits complement
        //              newdata32=0xc;
        if (thresh2bit>31) thresh2bit=31;
        if (thresh2bit<-32) thresh2bit=-32;
        if (ped2bit>31) ped2bit=31;
        if (ped2bit<-32) ped2bit=-32;
        newdata32=(thresh2bit & 0x3f) | ((ped2bit & 0x3f) << 6);
        
        if (flag) newdata32=0x1f+(1<<11); // mask bad pixels as good as you can (high thresh and very low ped)
        
//            if ((y<2 || y>253) && (x<4||x>61)) {
//              printf("\tsubm=%2d,y=%3d,x=%3d,address=0x%8lx,data=0x%3lx,thresh2bit=%3d\n",subm,y,x,address,newdata32,thresh2bit);
//            }
        //vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address+baseShift+offset);
        result[offset/4] = newdata32;
      }
      fclose(fp);
    } else {
      EUDAQ_ERROR(std::string("Unable top open pedestal file ") + filename);
    }
    // now black out the dummy pixels
    printf("\tMasking dummy pixels!\n");
    newdata32=0x1f+(1<<11); // mask dummy pixels as good as you can (high thresh and very low ped)
    for (int subm=0;subm<4;subm++) {
      for (int y=0;y<256;y++) {
        for (int x=0;x<2;x++) {
          offset=(x+(y<<7)+(subm<<18))<<2;
//              if ((y<3 || y>252) && (x<2))
//                printf("\tsubm=%2d,y=%3d,x=%3d,address=0x%8lx,data=0x%lx\n",subm,y,x,address,newdata32);
//              vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,address+baseShift+offset);
        }
      }
    }
    printf("\tdone!\n");
    return result;
  }

  struct BoardInfo {
    BoardInfo(unsigned long addr, const std::string & mode, const std::string & det)
      : BaseAddress(addr), mode(mode), det(det), zs(mode == "ZS") {}
    unsigned long BaseAddress;
    std::string mode, det;
    bool zs;
  };

  void LoadPedestals(const BoardInfo & board, const pedestal_t & peds) {
    unsigned long readdata32, newdata32;
    // here we put in the uploading of pedestals:
    // VME is master of SRAM
    printf("Become Master of SRAM\n");
    vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,board.BaseAddress);
    newdata32=readdata32|0x200;
    vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,board.BaseAddress);
    for (size_t i = 0; i < peds.size(); ++i) {
      vme_A32_D32_User_Data_SCT_write(fdOut, peds[i], board.BaseAddress + 0x800000 + i*4);
    }
    // Release master of SRAM
    printf("\tRelease Master of SRAM\n");
    vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,board.BaseAddress);
    newdata32=readdata32&~0x200;
    vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,board.BaseAddress);
    // reset once more to be sure
    /* read address first and only set the reset bit */
    vme_A32_D32_User_Data_SCT_read(fdOut,&readdata32,board.BaseAddress);
    newdata32=readdata32|0x40;
    vme_A32_D32_User_Data_SCT_write(fdOut,newdata32 ,board.BaseAddress);
    vme_A32_D32_User_Data_SCT_write(fdOut,readdata32,board.BaseAddress);
    //EUDAQ_INFO("Board " + to_string(n_eudrb) + " done!");
  }
  unsigned m_run, m_ev;
  bool done, started, juststopped;
  int n_error;
  std::vector<unsigned long> buffer;
  std::vector<BoardInfo> boards;
  int fdOut;
  int m_idoffset, m_version;
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
