#ifndef dataQueue_h__
#define dataQueue_h__
#include <memory>
#include <queue>
#include <mutex>


#include <chrono>
namespace eudaq{

  class paketProducer;
  class RawDataEvent;
  class FileWriter;

  class dataQueue{
  public:
    bool empty();
    void addNewProducer(std::shared_ptr<eudaq::paketProducer> prod);
    void trigger();
    void pushPacket(std::shared_ptr<RawDataEvent> packet);
    std::shared_ptr<RawDataEvent> getPacket();
    void writeToFile(const char* filename);
    void WritePacket(std::shared_ptr<RawDataEvent>  packet);
    void writeNextPacket();
    std::queue<std::shared_ptr<RawDataEvent>> m_queue;
    std::mutex m_queueMutex;
    std::shared_ptr<FileWriter> m_writer;
    std::vector<std::shared_ptr<paketProducer>> m_producer;
   long long m_starttimer;
  };
}
#endif // dataQueue_h__
