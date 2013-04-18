#include "eudaq/Producer.hh"
#include "eudaq/TimepixDummyProducer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"
#include <iostream>
#include <ostream>
#include <cctype>



TimepixDummyProducer::TimepixDummyProducer(const std::string & name,
    const std::string & runcontrol)
: eudaq::Producer(name, runcontrol), m_done(false), m_run(0) , m_ev(0)
{
  // First initialise the mutex attributes
  pthread_mutexattr_init(&m_mutexattr);

  // Inititalise the mutexes
  pthread_mutex_init( &m_done_mutex, 0 );
  pthread_mutex_init( &m_run_mutex, 0 );
  pthread_mutex_init( &m_ev_mutex, 0 );
}

TimepixDummyProducer::~TimepixDummyProducer()
{
  pthread_mutex_destroy( &m_done_mutex );
  pthread_mutex_destroy( &m_run_mutex );
  pthread_mutex_destroy( &m_ev_mutex );
}

bool TimepixDummyProducer::GetDone()
{
  bool retval;
  pthread_mutex_lock( &m_done_mutex );
  retval = m_done;
  pthread_mutex_unlock( &m_done_mutex );    
  return retval;
}

unsigned int TimepixDummyProducer::GetRunNumber()
{
  unsigned int retval;
  pthread_mutex_lock( &m_run_mutex );
  retval = m_run;
  pthread_mutex_unlock( &m_run_mutex );    
  return retval;
}

unsigned int TimepixDummyProducer::GetEventNumber()
{
  unsigned int retval;
  pthread_mutex_lock( &m_ev_mutex );
  retval = m_ev;
  pthread_mutex_unlock( &m_ev_mutex );    
  return retval;
}

unsigned int TimepixDummyProducer::GetIncreaseEventNumber()
{
  unsigned int retval;
  pthread_mutex_lock( &m_ev_mutex );
  retval = m_ev++;
  pthread_mutex_unlock( &m_ev_mutex );
  return retval;
}

void TimepixDummyProducer::SetDone(bool done)
{
  pthread_mutex_lock( &m_done_mutex );
  m_done = done;
  pthread_mutex_unlock( &m_done_mutex );    
}

void TimepixDummyProducer::SetEventNumber(unsigned int eventnumber)
{
  pthread_mutex_lock( &m_ev_mutex );
  m_ev = eventnumber;
  pthread_mutex_unlock( &m_ev_mutex );    
}

void TimepixDummyProducer::SetRunNumber(unsigned int runnumber)
{
  pthread_mutex_lock( &m_run_mutex );
  m_run = runnumber;
  pthread_mutex_unlock( &m_run_mutex );    
}

void TimepixDummyProducer::Event(unsigned short *timepixdata)
{
  eudaq::RawDataEvent ev("Timepix",GetRunNumber(), GetIncreaseEventNumber() );

  // a 128 kB data block, in this the data is stored in little endian
  unsigned char serialdatablock[131072];

  for (unsigned i=0; i < 65536 ; i ++)
  {
    // send little endian, i. e. the most significant first
    serialdatablock[2*i] = (timepixdata[i] & 0xFF00) >> 8 ;
    serialdatablock[2*i + 1] = timepixdata[i] & 0xFF ;
  }

  ev.AddBlock(0, serialdatablock , 131072);

  SendEvent(ev);
}

void TimepixDummyProducer::SimpleEvent()
{
  // a 64 kWord data block in the computer whatever endian format
  unsigned short rawdatablock[65536];

  for (unsigned i=0; i < 65536 ; i ++)
  {
    rawdatablock[i] = i;
  }

  //ev.SetTag("Debug", "foo");
  Event(rawdatablock);
}

void TimepixDummyProducer::BlobEvent()
{
  //    TestEvent ev(m_run, ++m_ev, str);
  //    //ev.SetTag("Debug", "foo");
  //    SendEvent(ev);
}


void TimepixDummyProducer::OnConfigure(const eudaq::Configuration & param) 
{
  std::cout << "Configuring." << std::endl;
  //    eventsize = param.Get("EventSize", 1);
  EUDAQ_INFO("Configured (" + param.Name() + ")");
  SetStatus(eudaq::Status::LVL_OK, "Configured (" + param.Name() + ")");
}

void TimepixDummyProducer::OnStartRun(unsigned param) 
{
  SetRunNumber( param );
  SetEventNumber( 1 ); // has to be 1 because BORE is event 0 :-(
  SendEvent(eudaq::RawDataEvent::BORE( "Timepix", param )); // send param instead of GetRunNumber
  std::cout << "Start Run: " << param << std::endl;
}

void TimepixDummyProducer::OnStopRun()
{
  SendEvent(eudaq::RawDataEvent::EORE("Timepix", GetRunNumber(), GetEventNumber()));
  std::cout << "Stop Run" << std::endl;
}

void TimepixDummyProducer::OnTerminate()
{
  std::cout << "Terminate (press enter)" << std::endl;
  SetDone( true );
}

void TimepixDummyProducer::OnReset()
{
  std::cout << "Reset" << std::endl;
  SetStatus(eudaq::Status::LVL_OK);
}

void TimepixDummyProducer::OnStatus()
{
  //std::cout << "Status - " << m_status << std::endl;
  //SetStatus(eudaq::Status::WARNING, "Only joking");
}

void TimepixDummyProducer::OnUnrecognised(const std::string & cmd, const std::string & param) 
{
  std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
  if (param.length() > 0) std::cout << " (" << param << ")";
  std::cout << std::endl;
  SetStatus(eudaq::Status::LVL_WARN, "Just testing");
}


int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Producer", "0.0", "A comand-line version of a timepix dummy Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "TimepixDummy", "string",
      "The name of this Producer");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    TimepixDummyProducer producer(name.Value(), rctrl.Value());
    bool help = true;
    do {
      if (help) {
        help = false;
        std::cout << "--- Commands ---\n"
          << "s      Send simple TimepixEvent\n"
          << "b      Send blob TimepixEvent\n"
          << "l msg  Send log message\n"
          << "o msg  Set status=OK\n"
          << "w msg  Set status=WARN\n"
          << "e msg  Set status=ERROR\n"
          << "q      Quit\n"
          << "?      \n"
          << "----------------" << std::endl;
      }
      std::string line;
      std::getline(std::cin, line);
      //std::cout << "Line=\'" << line << "\'" << std::endl;
      char cmd = '\0';
      if (line.length() > 0) {
        cmd = std::tolower(line[0]);
        line = eudaq::trim(std::string(line, 1));
      } else {
        line = "";
      }
      switch (cmd) {
        case '\0': // ignore
          break;
        case 's':
          producer.SimpleEvent();
          break;
        case 'b':
          producer.BlobEvent();
          break;
        case 'l':
          EUDAQ_USER(line);
          break;
        case 'o':
          producer.SetStatus(eudaq::Status::LVL_OK, line);
          break;
        case 'w':
          producer.SetStatus(eudaq::Status::LVL_WARN, line);
          break;
        case 'e':
          producer.SetStatus(eudaq::Status::LVL_ERROR, line);
          break;
        case 'q':
          producer.SetDone(true);
          break;
        case '?':
          help = true;
          break;
        default:
          std::cout << "Unrecognised command, type ? for help" << std::endl;
      }
    } while (!producer.GetDone());
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
