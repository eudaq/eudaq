#include "eudaq/AidaFileReader.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/AidaPacket.hh"
#include "eudaq/Logger.hh"
#include <list>
#include "eudaq/FileSerializer.hh"

namespace eudaq {

  AidaFileReader::AidaFileReader(const std::string & file, const std::string & filepattern)
    : m_filename(FileNamer(filepattern).Set('X', ".raw").SetReplace('R', file)),
    m_des(m_filename),
    m_ev(PacketFactory::Create(m_des))
    {
  }

  AidaFileReader::~AidaFileReader() {
  }

  bool AidaFileReader::NextPacket(size_t skip) {
	  return false;
/*
    std::shared_ptr<eudaq::AidaPacket> packet = nullptr;
    bool result = m_des.ReadPacket(m_ver, ev, skip);
    if (ev) m_ev =  ev;
    return result;
*/
  }

  unsigned AidaFileReader::RunNumber() const {
    return -1; //m_ev->GetRunNumber();
  }

  const AidaPacket & AidaFileReader::GetPacket() const {
    return *m_ev;
  }


}
