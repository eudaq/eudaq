#include "IpbusHwController.h"
#include <algorithm>

namespace ipbus{

  IpbusHwController::IpbusHwController( const std::string &connectionFile, const std::string &deviceName ){
    uhal::ConnectionManager manager( connectionFile );
    m_hw = new uhal::HwInterface( manager.getDevice( deviceName ) );
  }

  IpbusHwController::IpbusHwController( const std::string &connectionFile, const std::string &deviceName, unsigned int blockSize ){
    uhal::ConnectionManager manager( connectionFile );
    m_hw = new uhal::HwInterface( manager.getDevice( deviceName ) );
    m_blockSize = blockSize;
    std::cout << "Ipbus m_blockSize = " << std::dec << m_blockSize << std::endl;
    for( unsigned int i=0; i<m_blockSize; i++ )
      m_data.push_back(0);
  }

  IpbusHwController::~IpbusHwController()
  {
    m_data.clear();
    delete m_hw;
  }
  
  uint32_t IpbusHwController::ReadRegister( const std::string &name )
  {
    try {
      uhal::ValWord< uint32_t > test = m_hw->getNode(name).read();
      m_hw->dispatch();
      if(test.valid()) {
	return test.value();
      } else {
	std::cout << "Error reading " << name << std::endl;
	return 0;
      }
    } catch (...) {
      return 0;
    }
  }

  void IpbusHwController::SetRegister( const std::string &name, uint32_t val )
  {
    try {
      uhal::ValHeader vhead = m_hw->getNode(name).write( val );
      m_hw->dispatch();
      while(1){
	if( vhead.valid() ) break;
	else
	  std::cout << "ValHeader is not valid" << std::endl;
      }
    } catch (...) {
      std::cout << "ValHeader could not be valid (strange to reach this point)" << std::endl;
      return;
    }
  }

  void IpbusHwController::ReadDataBlock( const std::string &name, uint32_t blkSize )
  {
    try {
      uhal::ValVector<uint32_t> data = m_hw->getNode(name.c_str()).readBlock(blkSize);
      m_hw->dispatch();
      if(data.valid()) {
  	CastTheData( data );
      } else {
  	std::cout << "Error reading " << name << std::endl;
  	return ;
      }
    } catch (...) {
      return;
    }
  }
  
  void IpbusHwController::ReadDataBlock( const std::string &name )
  {
    try {
      uhal::ValVector<uint32_t> data = m_hw->getNode(name.c_str()).readBlock(m_blockSize);
      m_hw->dispatch();
      if(data.valid()) {
      	CastTheData( data );
      } else {
      	std::cout << "Error reading " << name << std::endl;
      	return ;
      }
      // m_data = m_hw->getNode(name.c_str()).readBlock(m_blockSize).value();
      // m_hw->dispatch();

    } catch (...) {
      return;
    }
  }

  void IpbusHwController::SetUhalLogLevel(unsigned char lvl){
    switch(lvl){
    case 0:
      uhal::disableLogging();
      break;
    case 1:
      uhal::setLogLevelTo(uhal::Fatal());
      break;
    case 2:
      uhal::setLogLevelTo(uhal::Error());
      break;
    case 3:
      uhal::setLogLevelTo(uhal::Warning());
      break;
    case 4:
      uhal::setLogLevelTo(uhal::Notice());
      break;
    case 5:
      uhal::setLogLevelTo(uhal::Info());
      break;
    case 6:
      uhal::setLogLevelTo(uhal::Debug());
      break;
    default:
      uhal::setLogLevelTo(uhal::Debug());      
    }
  }
  
  void IpbusHwController::CastTheData(const uhal::ValVector<uint32_t> &data)
  {
    // for( uhal::ValVector<uint32_t>::const_iterator cit=data.begin(); cit!=data.end(); ++cit )
    //   m_data.push_back(*cit);
    // std::cout << "\t data.size() = " << data.size() << std::endl;
    // for( unsigned int i=0; i<data.size(); i++ )
    //   m_data[i]=data[i];
    std::copy(data.begin(), data.end(),m_data.begin());
  }

  void IpbusHwController::ResetTheData()
  {
    for( unsigned int i=0; i<m_data.size(); i++ )
      m_data[i]=0;
    //    m_data.clear();
  }

}
