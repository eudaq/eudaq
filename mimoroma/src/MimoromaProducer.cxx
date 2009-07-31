#include "eudaq/Producer.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include "eudaq/OptionParser.hh"

#include <iostream>
#include <ostream>
#include <cctype>
#include <vector>
#include <cassert>

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

extern "C" {
#include "daq_beam.h"
}


using eudaq::RawDataEvent;
using eudaq::to_string;
using std::vector;


class MimoromaProducer : public eudaq::Producer {

public:
  
  MimoromaProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol),
      done(false),
      started(false),
      juststopped(false),
      fdOut(open("/tmp/sis1100", O_RDWR)),
      m_rawData12(NULL),
      m_rawData34(NULL),
      m_rawData56(NULL),
      m_rawData78(NULL)
  {
      if (fdOut < 0) {
        EUDAQ_THROW("Open device failed Errno = " + to_string(errno));
      }
    }
  

  void Process() {
    if (!started) {
      eudaq::mSleep(1);
      return;
    }


    u_int32_t membank = m_param.Get("MemoryBank",1);
    u_int32_t ADCAddress = m_param.Get("ADCAddress", 0x30000000);
    u_int32_t FPGAAddress = m_param.Get("FPGAAddress", 0x40000000);

    // arm for sampling
    vme_A32D32_write( fdOut, ADCAddress + 0x10, membank );
    
    // issue a key start
    vme_A32D32_write( fdOut, ADCAddress + 0x30, 0x7 );
    
    // unforce the busy
    vme_A32D16_write( fdOut, FPGAAddress + 0x4C, 0);

    // poll the data ready bit
    u_int32_t sampleClock;
    vme_A32D32_read( fdOut, ADCAddress + 0x10, &sampleClock );
    
    while ( ( sampleClock & 0x1 ) == 0x1 ) {
      
      vme_A32D32_read( fdOut, ADCAddress + 0x10, &sampleClock );

    }

    // ok we have got a trigger
    // read the data here from the ADC
    unsigned int noOfReadWords;
    unsigned int samples = m_param.Get("NumberOfSamples", 16400);

    vme_A32BLT32_read( fdOut, ADCAddress + 0x400000, m_rawData12, samples, &noOfReadWords);
    vme_A32BLT32_read( fdOut, ADCAddress + 0x480000, m_rawData34, samples, &noOfReadWords);
    vme_A32BLT32_read( fdOut, ADCAddress + 0x500000, m_rawData56, samples, &noOfReadWords);
//     vme_A32BLT32_read( fdOut, ADCAddress + 0x580000, m_rawData78, samples, &noOfReadWords);

    unsigned short int iTriggerLSB, iTriggerMSB;
    vme_A32D16_read(fdOut, FPGAAddress + 0x003C, &iTriggerLSB);
    vme_A32D16_read(fdOut, FPGAAddress + 0x003E, &iTriggerMSB);

    int iTrigger = (iTriggerMSB<<16)+iTriggerLSB;

    const size_t headerSize = 28;
    long int header[ headerSize ];
    header[0] = 0x0;
    header[1] = m_ev;
    header[2] = m_ev;
    header[3] = iTrigger;
    for (int i=0;i<24;i++) {  //remaining fields of the header are 0
      header [4+i] = 0x0;
    }
    
    vector<short > headerVec( reinterpret_cast< short* > (&header[0]),
			      reinterpret_cast< short* > (&header[ headerSize ] ));

    assert( headerVec.size() == headerSize * 2 );

    vector<short > dataChannel1;
    vector<short > dataChannel2;
    vector<short > dataChannel3;
    vector<short > dataChannel4;
    vector<short > dataChannel5;
//     vector<short > dataChannel6;
//     vector<short > dataChannel7;
//     vector<short > dataChannel8;
    

    for ( size_t i = 0; i < noOfReadWords; ++i ) { 

      dataChannel1.push_back( (m_rawData12[ i ] >> 16 ) & 0xFFFF );
      dataChannel3.push_back( (m_rawData34[ i ] >> 16 ) & 0xFFFF );
      dataChannel5.push_back( (m_rawData56[ i ] >> 16 ) & 0xFFFF );
//       dataChannel7.push_back( (m_rawData78[ i ] >> 16 ) & 0xFFFF );

      dataChannel2.push_back( (m_rawData12[ i ] >> 0 ) & 0xFFFF );
      dataChannel4.push_back( (m_rawData34[ i ] >> 0 ) & 0xFFFF );
//       dataChannel6.push_back( (m_rawData56[ i ] >> 0 ) & 0xFFFF );
//       dataChannel8.push_back( (m_rawData78[ i ] >> 0 ) & 0xFFFF );

    }
   
    unsigned int trailer = 0x89abcdef;
    unsigned short msbTrailer = ( trailer >> 16 ) ;
    unsigned short lsbTrailer = ( trailer & 0xFFFF );

    vector< short > trailerVec;
    trailerVec.push_back( lsbTrailer );
    trailerVec.push_back( msbTrailer );

    vector< short > eventVector;
    for ( size_t i = 0 ; i < headerVec.size() ; ++i ) {
      eventVector.push_back( headerVec[i] );
    }

    for ( size_t i = 0 ; i < dataChannel1.size() ; ++i ) {
      eventVector.push_back( dataChannel1[i] );
    }
    
    for ( size_t i = 0 ; i < dataChannel2.size() ; ++i ) {
      eventVector.push_back( dataChannel2[i] );
    }

    for ( size_t i = 0 ; i < dataChannel3.size() ; ++i ) {
      eventVector.push_back( dataChannel3[i] );
    }

    for ( size_t i = 0 ; i < dataChannel4.size() ; ++i ) {
      eventVector.push_back( dataChannel4[i] );
    }

    for ( size_t i = 0 ; i < dataChannel5.size() ; ++i ) {
      eventVector.push_back( dataChannel5[i] );
    }

//     for ( size_t i = 0 ; i < dataChannel6.size() ; ++i ) {
//       eventVector.push_back( dataChannel6[i] );
//     }

//     for ( size_t i = 0 ; i < dataChannel7.size() ; ++i ) {
//       eventVector.push_back( dataChannel7[i] );
//     }

//     for ( size_t i = 0 ; i < dataChannel8.size() ; ++i ) {
//       eventVector.push_back( dataChannel8[i] );
//     }
    
    for ( size_t i = 0 ; i < trailerVec.size() ; ++i ) {
      eventVector.push_back( trailerVec[i] );
    }

    RawDataEvent ev("MIMOROMA", m_run, m_ev);
    ev.AddBlock(0, eventVector);
    SendEvent( ev );
    

    if ( m_ev < 10 ) {
      std::cout << "Sent event " << m_ev << std::endl;
    } else if ( m_ev % 10 == 0 ) {
      std::cout << "Sent event " << m_ev << std::endl;
    }

    ++m_ev;
  }


  void print_return_code (int returnCode) {
    if (returnCode == 0) printf ("Successful   \n");
    if (returnCode != 0) {
      printf ("ERROR   ");
      printf("(returnCode = %d)\n",returnCode);
    }	
  }


  virtual void OnConfigure(const eudaq::Configuration & param) {
    SetStatus(eudaq::Status::LVL_OK, "Wait");
    
    m_param = param;


    try {
      std::cout << "Configuring (" << param.Name() << ")..." << std::endl;

      int returnCode;  

      // resetting the vme bus
      std::cout << "Resetting the VME bus... " ;
      returnCode = vmesysreset(fdOut);
      print_return_code(returnCode);
      sleep(1);
      

      u_int32_t baseADC = param.Get("ADCAddress", 0x30000000);
      u_int32_t baseV1495 = param.Get("FPGAAddress", 0x40000000);

      {

	std::cout << "Configuring the FPGA ... " << std::endl;

	// this scope is just for V1495 programming
	u_int16_t data16;
	u_int32_t offset;

	// setting the initial row
	data16 = param.Get("InitialRow", 0x0);
	offset = 0x0020;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);
	
	// setting the initial col
	data16 = param.Get("InitialCol", 0x0);
	offset = 0x0022;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);	

	// setting the number of row
	data16 = param.Get("NumRow", 0x40);
	offset = 0x0024;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);
	
	// setting the number of col
	data16 = param.Get("NumCol", 0x10);
	offset = 0x0026;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the number of row to reset
	data16 = param.Get("NumRowRst", 0x10);
	offset = 0x0028;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the number of reset cycles
	data16 = param.Get("NumRstCycle", 0x28);
	offset = 0x002A;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the clock divider
	data16 = param.Get("ClkDivider", 0x28);
	offset = 0x002C;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the number of frame
	data16 = param.Get("NumFrame", 0x20);
	offset = 0x002E;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the working mode
	data16 = param.Get("WorkingMode", 0x262);
	offset = 0x0032;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the no_sparse register
	data16 = param.Get("NoSparseReg", 0x0);
	offset = 0x0034;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the sparse reg
	data16 = param.Get("SparseReg", 0x0);
	offset = 0x0036;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// setting the address reg
	data16 = param.Get("AddressReg", 0x0);
	offset = 0x0038;
	vme_A32D16_write(fdOut,baseV1495 + offset,data16);

	// reading back the configuration
	for ( u_int32_t off = 0; off < 0x38; off+=2 ) {
	  
	  vme_A32D16_read(fdOut,baseV1495+off,&data16);
	  printf("   offset 0x%X, data 0x%X \n", off, data16);

	}

	//reseting the trigger counter on FPGA (iTrigger will be saved in the data file along with iEvent for debugging purpose)
	vme_A32D16_write(fdOut, baseV1495 + 0x004A, 0); 

	

      } // this is the end of the FPGA configuring scope 

      // let's open another scope for the ADC configuration
      {

	std::cout << "Configuring the ADC ... " << std::endl;
	u_int32_t data32;
	
	// general board reset
	vme_A32D32_write(fdOut, baseADC + 0x20, 0x7);
	
	// acquisition register
	data32 = param.Get("AcqReg", 0x6100);
	vme_A32D32_write(fdOut, baseADC+ 0x10, data32);

	// event configuration
	data32 = param.Get("EvtCfgReg", 0x8);
	vme_A32D32_write(fdOut, baseADC+ 0x100000, data32);

	if ( m_rawData12 != NULL ) { delete [] m_rawData12; }
	m_rawData12 = new unsigned int[ param.Get("NumberOfSamples",16400) ];

	if ( m_rawData34 != NULL ) { delete [] m_rawData34; }
	m_rawData34 = new unsigned int[ param.Get("NumberOfSamples",16400) ];

	if ( m_rawData56 != NULL ) { delete [] m_rawData56; }
	m_rawData56 = new unsigned int[ param.Get("NumberOfSamples",16400) ];

	if ( m_rawData78 != NULL ) { delete [] m_rawData78; }
	m_rawData78 = new unsigned int[ param.Get("NumberOfSamples",16400) ];

      } // end of ADC conf.

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
      
      RawDataEvent ev(RawDataEvent::BORE( "MIMOROMA", m_run ) );
      ev.SetTag("InitialRow", to_string( m_param.Get("InitialRow", 0x0) )) ;
      ev.SetTag("InitialCol", to_string( m_param.Get("InitialCol", 0x0) )) ;
      ev.SetTag("NumRow", to_string( m_param.Get("NumRow", 0x40) )) ;
      ev.SetTag("NumCol", to_string( m_param.Get("NumCol", 0x10) )) ;
      ev.SetTag("NumRowRst", to_string( m_param.Get("NumRowRst", 0x10) )) ;
      ev.SetTag("NumRstCycle", to_string( m_param.Get("NumRstCycle", 0x28) )) ;
      ev.SetTag("ClkDivider", to_string( m_param.Get("ClkDivider", 0x28) )) ;
      ev.SetTag("NumFrame", to_string( m_param.Get("NumFrame", 0x20) )) ;
      ev.SetTag("WorkingMode", to_string( m_param.Get("WorkingMode", 0x262) )) ;
      ev.SetTag("NoSparseReg", to_string( m_param.Get("NoSparseReg", 0x0) )) ;
      ev.SetTag("SparseReg", to_string( m_param.Get("SparseReg", 0x0) )) ;
      ev.SetTag("AddressReg", to_string( m_param.Get("AddressReg", 0x0) )) ;
      ev.SetTag("ADCAddress", to_string( m_param.Get("ADCAddress", 0x30000000) )) ;
      ev.SetTag("FPGAAddress", to_string( m_param.Get("FPGAAddress", 0x40000000) )) ;
      ev.SetTag("NumberOfSamples", to_string( m_param.Get("NumberOfSamples", 16400) )) ;
      ev.SetTag("AcqReg", to_string( m_param.Get("AcqReg", 0x6100) )) ;
      ev.SetTag("EvtCfgReg", to_string( m_param.Get("EvtCfgReg", 0x8) )) ;
      ev.SetTag("MemoryBank", to_string( m_param.Get("MemoryBank", 1) )) ;
      ev.SetTag("ChannelToSample", to_string( m_param.Get("ChannelToSample", 1) )) ;
      
      SendEvent( ev );
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
      juststopped = true;
      while (started) {
        eudaq::mSleep(100);
      }
      juststopped = false;
      SendEvent(RawDataEvent::EORE("MIMOROMA", m_run, ++m_ev));
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
    
    if ( m_rawData12 != NULL ) { delete [] m_rawData12; }
    if ( m_rawData34 != NULL ) { delete [] m_rawData34; }
    if ( m_rawData56 != NULL ) { delete [] m_rawData56; }
    if ( m_rawData78 != NULL ) { delete [] m_rawData78; }
    
    done = true;
  }
  
  virtual void OnReset() {
    std::cout << "Resetting ..." << std::endl;
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
  bool done, started, juststopped;
  int fdOut;
  eudaq::Configuration  m_param;

  unsigned int * m_rawData12;
  unsigned int * m_rawData34;
  unsigned int * m_rawData56;
  unsigned int * m_rawData78;


};

    int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Mimoroma Producer", "1.0", "The Producer task for reading out Mimoroma chip via VME");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:7000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "Mimoroma", "string",
                                   "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    MimoromaProducer producer(name.Value(), rctrl.Value());

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




