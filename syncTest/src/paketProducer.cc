#include "paketProducer.h"
#include "eudaq\AidaPacket.hh"
#include <ctime>
#include <windows.h>
using std::chrono::microseconds;
using std::chrono::duration_cast;
using namespace std;

namespace eudaq{
  void paketProducer::startDataTaking( int readoutSpeed )
  {
	  auto pack = make_shared<AidaPacket>(m_type, AidaPacket::str2type(m_paketName));
	  std::vector<uint64_t> data = { 1 };

	  pack->setDataCopy(data);
	  pack->GetMetaData().add(0, 1, 0);
	  m_dataqueue->pushPacket(pack);
    m_readoutSpeed=readoutSpeed;
    m_thread=std::unique_ptr<std::thread>(new std::thread(&paketProducer::run,this));
  }

  void paketProducer::addDummyData( uint64_t* meta_data, size_t meta_data_size, uint64_t* data, size_t data_size )
  {
    m_data.emplace_back(meta_data,meta_data_size,data,data_size);
  }

  void paketProducer::run()
  {
   if (m_readoutSpeed==0)
   {
     return;
   }
    while (m_running)
    {

      newPacket();
      Sleep(m_readoutSpeed);
    
    }
  }

  void paketProducer::addDataQueue( dataQueue* dqueue )
  {
    if (!m_dataqueue)
    {
      m_dataqueue=dqueue;
    }else{
      std::cout<< " data queue already defined " <<std::endl;
    }

  }

  void paketProducer::stopDataTaking()
  {
    m_running = false;
    m_thread->join();
	newPacket();
  }

  void paketProducer::getTrigger(long long timeStamp)
  {
    if (m_readoutSpeed==0)
    {
      newPacket();
    }
    lock_guard<std::mutex> m(m_mutex);
    m_pack->GetMetaData().add(1, m_type, static_cast<uint64_t>(timeStamp));
  }

  void paketProducer::newPacket()
  {
    lock_guard<std::mutex> m(m_mutex);
    if (m_pack)
    {
      std::string dummy = "new Event producer " + m_paketName + "\n";
      std::cout << dummy;
      m_dataqueue->pushPacket(m_pack);
    }
	m_pack = make_shared<AidaPacket>(m_type,AidaPacket::str2type(m_paketName));

    
    for (auto &e : m_data){
   //   m_pack->SetData(e.m_data, e.m_data_size);
	//  m_pack->copyData();
    }
  }

}