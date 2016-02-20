#include <ostream>
#include <iostream>
#include <time.h>

#include "eudaq/BufferSerializer.hh"
#include "eudaq/AidaPacket.hh"

using std::cout;

namespace eudaq {
  EUDAQ_DEFINE_PACKET(EventPacket, str2type("-EVWRAP-"));

  AidaPacket::AidaPacket(Deserializer &ds) {
    m_header = DeserializeHeader(ds);
    m_meta_data = MetaData(ds);
    DeserializeData(ds);
    ds.read(checksum);
  }

  AidaPacket::AidaPacket(PacketHeader &header, Deserializer &ds) {
    m_header = header;
    m_meta_data = MetaData(ds);
    DeserializeData(ds);
    ds.read(checksum);
  }

  AidaPacket::AidaPacket(const PacketHeader &header, const MetaData &meta) {
    m_header = header;
    m_meta_data = meta;
  }

  void AidaPacket::SerializeHeader(Serializer &s) const {
    uint64_t *arr = (uint64_t *)&m_header.data;
    for (int i = 0; i < sizeof(m_header.data) / sizeof(uint64_t); i++)
      s.write(arr[i]);
  }

  void AidaPacket::SerializeMetaData(Serializer &s) const {
    m_meta_data.Serialize(s);
  }

  void AidaPacket::Serialize(Serializer &ser) const {
    SerializeHeader(ser);
    SerializeMetaData(ser);
    ser.write(m_data_size);
    ser.append((const unsigned char *)m_data, m_data_size * sizeof(uint64_t));
    ser.write(ser.GetCheckSum());
  }

  void AidaPacket::DeserializeData(Deserializer &ds) {
    ds.read(m_data_size);
    uint64_t *tmp = new uint64_t[m_data_size];
    ds.read((unsigned char *)tmp, m_data_size * sizeof(uint64_t));
    placeholder = std::unique_ptr<uint64_t[]>(tmp);
    m_data = placeholder.get();
  }

  AidaPacket::PacketHeader AidaPacket::DeserializeHeader(Deserializer &ds) {
    PacketHeader header;
    uint64_t *arr = (uint64_t *)&header.data;
    for (int i = 0; i < sizeof(header.data) / sizeof(uint64_t); i++)
      ds.read(arr[i]);
    return header;
  }

  const uint64_t *const AidaPacket::bit_mask() {
    static uint64_t *array = NULL;
    if (!array) {
      array = new uint64_t[65];
      array[0] = 0;
      array[1] = 1;
      for (int i = 2; i <= 64; i++) {
        array[i] = (array[i - 1] << 1) + 1;
      }
    }
    return array;
  }

  uint64_t AidaPacket::str2type(const std::string &str) {
    uint64_t result = 0;
    for (size_t i = str.length(); i > 0; i--) {
      result <<= 8;
      result |= str[i - 1];
    }
    // cout << "str2type: str=>" << str << "< uint64=" << std::hex << result <<
    // std::endl;
    return result;
  }

  std::string AidaPacket::type2str(uint64_t id) {
    // std::cout << "id2str(" << std::hex << id << std::dec << ")" <<
    // std::flush;
    std::string result(8, '\0');
    for (int i = 0; i < 8; ++i) {
      result[i] = (char)(id & 0xff);
      id >>= 8;
    }
    for (int i = 7; i >= 0; --i) {
      if (result[i] == '\0') {
        result.erase(i);
        break;
      }
    }
    // std::cout << " = " << result << std::endl;
    return result;
  }

  const AidaPacket::PacketIdentifier &AidaPacket::identifier() {
    static PacketIdentifier packet_identifier;
    if (packet_identifier.string.empty()) {
      packet_identifier.string = "#PACKET#";
      packet_identifier.number = str2type(packet_identifier.string);
    }
    return packet_identifier;
  }

  void AidaPacket::Print(std::ostream &os) const {
    os << "Type=" << type2str(GetPacketType())
       << ", packet#=" << GetPacketNumber();
  }

  std::ostream &operator<<(std::ostream &os, const AidaPacket &packet) {
    packet.Print(os);
    return os;
  }

  EventPacket::EventPacket(const Event &ev) : m_ev(&ev) {
    m_header.data.packetType = get_type();
    m_header.data.packetSubType = 0;
    m_header.data.packetNumber = m_ev->GetEventNumber();

    m_meta_data.add(false, MetaData::Type::TRIGGER_COUNTER,
                    m_ev->GetEventNumber());
    m_meta_data.add(false, MetaData::Type::TRIGGER_TIMESTAMP,
                    m_ev->GetTimestamp());
  }

  EventPacket::EventPacket(PacketHeader &header, Deserializer &ds) {
    m_header = header;
    m_meta_data = MetaData(ds);
    m_ev = EventFactory::Create(ds);
    ds.read(checksum);
  }

  void EventPacket::Serialize(Serializer &ser) const {
    // std::cout << "Serialize ev# = " << std::hex << GetPacketNumber() <<
    // std::endl;
    SerializeHeader(ser);
    SerializeMetaData(ser);
    m_ev->Serialize(ser);
    ser.write(ser.GetCheckSum());
  }

  std::shared_ptr<AidaPacket> PacketFactory::Create(Deserializer &ds) {
    AidaPacket::PacketHeader header = AidaPacket::DeserializeHeader(ds);
    int long long id = header.data.packetType;
    // std::cout << "Create id = " << std::hex << id << std::dec << std::endl;
    packet_creator cr = GetCreator(id);
    if (cr)
      return cr(header, ds);
    return std::shared_ptr<AidaPacket>(new AidaPacket(header, ds));
  };

  PacketFactory::map_t &PacketFactory::get_map() {
    static map_t s_map;
    return s_map;
  }

  void PacketFactory::Register(uint64_t id,
                               PacketFactory::packet_creator func) {
    // TODO: check id is not already in map
    get_map()[id] = func;
  }

  PacketFactory::packet_creator PacketFactory::GetCreator(int id) {
    // TODO: check it exists...
    return get_map()[id];
  }
}
