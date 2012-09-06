#include "eudaq/Monitor.hh"
#include "eudaq/Logger.hh"
#include "eudaq/PluginManager.hh"

#define EUDAQ_MAX_EVENTS_PER_IDLE 1000

namespace eudaq {

/* Michal Wysokinski : the following code was used for profiling */
/*
    //static int counter_events_for_online_monitor = 0;
    static struct timeval then, now, evt_then, evt_now;
//    static double single_event_time = 0;
    static double evt_clock = 0.0;
*/
  Monitor::Monitor(const std::string & name, const std::string & runcontrol, const unsigned lim,
                   const unsigned skip_, const std::string & datafile) :
    CommandReceiver("Monitor", name, runcontrol, false),
    m_run(0),
    m_callstart(false),
    m_reader(0),
    limit(lim),
    skip(100-skip_)
  {
    if (datafile != "") {
      // set offline
      m_reader = counted_ptr<FileReader>(new FileReader(datafile));
      PluginManager::Initialize(m_reader->GetDetectorEvent()); // process BORE
      //m_callstart = true;
      std::cout << "DEBUG: Reading file " << datafile << " -> " << m_reader->Filename() << std::endl;
      //OnStartRun(m_run);
    }
    StartThread();
  }

  bool Monitor::ProcessEvent() {
    if (!m_reader.get()) return false;

    unsigned evt_number = m_reader->GetDetectorEvent().GetEventNumber();
    if(limit > 0 && evt_number > limit)
        return true;

/* Michal Wysokinski : the following code was used for profiling */
/*
    if (limit > 0 && evt_number % limit == 0)
    {
        if(evt_number == 0)
            gettimeofday(&then, NULL);
        else
        {
            gettimeofday(&now, NULL);
            double clock = (now.tv_sec -then.tv_sec) + (1e-6 * (now.tv_usec -then.tv_usec));
            std::cout<< "Total execution time: " << std::scientific << clock << " s" << std::endl;
            std::cout<< "Analysed events: " << evt_number << std::endl;
            evt_clock /= (double)evt_number;
            std::cout<< std::scientific << "Execution time per single event: " << evt_clock << std::endl;
            exit(0);
        }
    }
    gettimeofday(&evt_then, NULL);
*/

//    std::cout << "processevent" << std::endl;
//move to the first line of the method    if (!m_reader.get()) return false;


    if (!m_reader->NextEvent()) return false;
    if (evt_number % 1000 == 0) {
      std::cout << "ProcessEvent " << m_reader->GetDetectorEvent().GetEventNumber()
                << (m_reader->GetDetectorEvent().IsBORE() ? "B" : m_reader->GetDetectorEvent().IsEORE() ? "E" : "")
                << std::endl;
    }

/* Michal Wysokinski : the following code was used for profiling */
/*
    if(skip > 0 && (evt_number % 100 >= skip))
    {
//        std::cout<< "Skipping event nr: " << evt_number << std::endl;
        gettimeofday(&evt_now, NULL);
        evt_clock += (evt_now.tv_sec -evt_then.tv_sec) + (1e-6 * (evt_now.tv_usec -evt_then.tv_usec));
        return true;
    }
*/

    try {
      const DetectorEvent & dev = m_reader->GetDetectorEvent();
      if (dev.IsBORE()) m_lastbore = counted_ptr<DetectorEvent>(new DetectorEvent(dev));
      OnEvent(PluginManager::ConvertToStandard(dev));
//        ++counter_events_for_online_monitor;
    } catch (const InterruptedException &) {
      return false;
    }

/* Michal Wysokinski : the following code was used for profiling */
/*
    gettimeofday(&evt_now, NULL);
    evt_clock += (evt_now.tv_sec -evt_then.tv_sec) + (1e-6 * (evt_now.tv_usec -evt_then.tv_usec));
*/
    return true;
  }

  void Monitor::OnIdle() {
    //std::cout << "..." << std::endl;
    if (m_callstart) {
      m_callstart = false;
      OnStartRun(m_run);
    }
    bool processed = false;
    for (int i = 0; i < EUDAQ_MAX_EVENTS_PER_IDLE; ++i) {
      if (ProcessEvent()) {
        processed = true;
      } else {
        //if (offline) OnTerminate();
        break;
      }
    }
    if (!processed) mSleep(1);
  }

  void Monitor::OnStartRun(unsigned param) {
    std::cout << "run " << param << std::endl;
    m_run = param;
    m_reader = counted_ptr<FileReader>(new FileReader(to_string(m_run)));
    PluginManager::Initialize(m_reader->GetDetectorEvent()); // process BORE
    EUDAQ_INFO("Starting run " + to_string(m_run));
  }

  void Monitor::OnStopRun() {
    m_reader->Interrupt();
  }

}
