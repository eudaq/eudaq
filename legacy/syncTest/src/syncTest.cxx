#include "dataQueue.h"
#include "paketProducer.h"
#include "eudaq/OptionParser.hh"

#include "eudaq/PluginManager.hh"

#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/FileWriter.hh"

using namespace eudaq;
using namespace std;
bool isTLUContainer(shared_ptr<RawDataEvent> tlu){
 // return tlu->GetPacketSubType() == 3; //todo Implement this correctly 
  return false;
}
int main(){

  eudaq::OptionParser op("EUDAQ Sync test", "1.0", "", 1);

  try{


    dataQueue dq;

    auto slowP = make_shared<paketProducer>("slowP");


    uint64_t meta = 111;
    const char * dataString = "hallo dass ist ein test   ";
    uint64_t * data = (uint64_t*) (dataString);
    size_t size = 3;
    slowP->addDummyData(&meta, 1, data, size);
    dq.addNewProducer(slowP);
    slowP->addDataQueue(&dq);


    auto fastP = make_shared<paketProducer>("fastP");
    uint64_t meta1 = 222;
    const char * dataString1 = "hallo dass ist ein test2  ";
    uint64_t * data1 = (uint64_t*) (dataString1);
    size_t size1 = 3;
    fastP->addDummyData(&meta1, 1, data1, size1);
    dq.addNewProducer(fastP);
    fastP->addDataQueue(&dq);

    auto multiP = make_shared<paketProducer>("multi");
    uint64_t meta2 = 333;
    const char * dataString2 = "hallo dass ist ein test3  ";
    uint64_t * data2 = (uint64_t*) (dataString2);
    size_t size2 = 3;
    multiP->addDummyData(&meta2, 1, data2, size2);
    dq.addNewProducer(multiP);
    multiP->addDataQueue(&dq);



    slowP->startDataTaking(100);
    fastP->startDataTaking(0);
    multiP->startDataTaking(20);
    for (int i = 0; i < 500; ++i)
    {
      dq.trigger();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));

    }


    slowP->stopDataTaking();
    fastP->stopDataTaking();
    multiP->stopDataTaking();
    //	auto writer1(AidaFileWriterFactory())
    //   GenericPacketSync<eudaq::AidaPacket> sync;

    //    sync->addBOREEvent(packet1);
    //   sync->addBOREEvent(packet3);
    //   sync->addBOREEvent(packet2);

    //  sync->PrepareForEvents();
    std::unique_ptr<SyncBase> sync;
    sync->addBORE_Event(0, *PluginManager::ExtractEventN(dq.getPacket(), 0));
    sync->addBORE_Event(0, *PluginManager::ExtractEventN(dq.getPacket(), 0));
    sync->addBORE_Event(0, *PluginManager::ExtractEventN(dq.getPacket(), 0));
    sync->PrepareForEvents();
    while (!dq.empty()){
      auto pack = dq.getPacket();
      for (size_t i = 0; i < PluginManager::GetNumberOfROF(*pack); i++)
      {
        auto ev = PluginManager::ExtractEventN(pack, i);
        sync->AddEventToProducerQueue(0, ev);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    do
    {

    } while (sync->SyncFirstEvent());
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto writer=FileWriterFactory::Create("meta");
    writer->SetFilePattern("test.txt");
    writer->StartRun(0);
    std::shared_ptr<eudaq::Event> det;
    while (sync->getNextEvent(det))
    {
      writer->WriteBaseEvent(*det);
    }
    cout << "end" << endl;
  }
  catch (...){
    return op.HandleMainException();

  }

  return 0;
}
