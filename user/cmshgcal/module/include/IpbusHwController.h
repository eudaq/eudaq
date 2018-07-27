#ifndef H_IPBUSCONTROLLER_HH
#define H_IPBUSCONTROLLER_HH

#include <vector>
#include <string>
#include <iostream>

#include <uhal/uhal.hpp>

namespace ipbus {

  class IpbusHwController{
  public:
    IpbusHwController(const std::string & connectionFile, const std::string & deviceName);
    IpbusHwController(const std::string & connectionFile, const std::string & deviceName, unsigned int blockSize);
    ~IpbusHwController();
  private:
    uhal::HwInterface *m_hw;
    std::vector<uint32_t> m_data;
    unsigned int m_blockSize;
  public:
    uint32_t ReadRegister(const std::string & name);
    void SetRegister(const std::string & name, uint32_t val);
    void ReadDataBlock(const std::string & blkName, uint32_t blkSize);
    void ReadDataBlock(const std::string & blkName);
    void SetUhalLogLevel(unsigned char lvl);
    void ResetTheData();
    uhal::HwInterface *getInterface() const {return m_hw;};

    void addTrailerToData(uint32_t trailer){m_data.push_back(trailer);}
    std::vector<uint32_t> const & getData(){return m_data;}
  private:
    void CastTheData(const uhal::ValVector<uint32_t> &data);
  };
}

#endif
