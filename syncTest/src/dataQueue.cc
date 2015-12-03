#include "dataQueue.h"

#include "paketProducer.h"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/FileWriter.hh"
#include <chrono>
using namespace std;
using std::chrono::microseconds;
using std::chrono::duration_cast;
namespace eudaq {




  void dataQueue::pushPacket( std::shared_ptr<RawDataEvent> packet )
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_queue.push(packet);

  }

  std::shared_ptr<RawDataEvent> dataQueue::getPacket()
  {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    auto ret=m_queue.front();
    m_queue.pop();
    return ret;
  }

  void dataQueue::writeToFile( const char* filename )
  {
    m_writer = FileWriterFactory::Create("metaDaten"); 
//    m_writer->StartRun(1001); $$
    m_writer->SetFilePattern("");
  }

  void dataQueue::WritePacket(std::shared_ptr<RawDataEvent>  packet )
  {
//     if (m_writer.get()) {
//       try {
//         m_writer->WritePacket( packet );
//       } catch(const Exception & e) {
//         std::string msg = "Exception writing to file: ";
//         msg += e.what();
//         cout << msg << endl;
// 
//       }
//
 //   }
  }


  void dataQueue::writeNextPacket()
  { auto packet=getPacket();
    WritePacket(packet);

  }

  void dataQueue::addNewProducer(std::shared_ptr<paketProducer> prod)
  {
    m_producer.push_back(prod);
    m_starttimer = duration_cast<microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  }

  void dataQueue::trigger()
  {
    long long timeNow = duration_cast<microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    long long timeStamp = timeNow - m_starttimer;
    for (auto& e : m_producer)
    {
      e->getTrigger(timeStamp);
    }

  }

  bool dataQueue::empty()
  {
    return m_queue.empty();
  }


}