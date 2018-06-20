#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

namespace eudaq {
  class Timepix3Event2StdEventConverter: public eudaq::StdEventConverter{
    typedef std::vector<uint8_t>::const_iterator datait;
  public:
    bool Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = eudaq::cstr2hash("Timepix3Raw");
  private:
    unsigned char unpackXorY( std::vector<unsigned char> data, size_t offset ) const;
    unsigned short unpackTOT(  std::vector<unsigned char> data, size_t offset ) const;
    uint64_t unpackTS( std::vector<unsigned char> data, size_t offset ) const;
  };

  namespace{
    auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<Timepix3Event2StdEventConverter>(Timepix3Event2StdEventConverter::m_id_factory);
  }

  bool Timepix3Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
    auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

    // No event
    if(!ev) {
      return false;
    }

    // Bad event
    if (ev->NumBlocks() != 2 || ev->GetBlock(0).size() < 20 ||
    ev->GetBlock(1).size() < 20) {
      EUDAQ_WARN("Ignoring bad event " + std::to_string(ev->GetEventNumber()));
      return false;
    }
    std::vector<unsigned char> data = ev->GetBlock( 1 ); // block 1 is pixel data

    // Create a StandardPlane representing one sensor plane
    eudaq::StandardPlane plane(0, "TPX3", "Timepix3");

    // Size of one pixel data chunk: 12 bytes = 1+1+2+8 bytes for x,y,tot,ts
    const unsigned int PIX_SIZE = 12;
    plane.SetSizeZS( 256, 256, ( data.size() ) / PIX_SIZE );

    std::vector<unsigned char> ZSDataX;
    std::vector<unsigned char> ZSDataY;
    std::vector<unsigned short> ZSDataTOT;
    std::vector<uint64_t> ZSDataTS;
    size_t offset = 0;
    unsigned char aWord = 0;

    for( unsigned int i = 0; i < ( data.size() ) / PIX_SIZE; i++ ) {

      ZSDataX   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 0 ) );
      ZSDataY   .push_back( unpackXorY( data, offset + sizeof( aWord ) * 1 ) );
      ZSDataTOT .push_back( unpackTOT(  data, offset + sizeof( aWord ) * 2 ) );
      ZSDataTS  .push_back( unpackTS(   data, offset + sizeof( aWord ) * 4 ) );

      offset += sizeof( aWord ) * PIX_SIZE;
    }

    for( size_t i = 0 ; i < ZSDataX.size(); ++i ) {
      plane.SetPixel( i, ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
    }

    // Add the plane to the StandardEvent
    d2->AddPlane(plane);

    // Indicate that data was successfully converted
    return true;
  }

  unsigned char Timepix3Event2StdEventConverter::unpackXorY( std::vector<unsigned char> data, size_t offset ) const {
    return data[offset];
  }

  unsigned short Timepix3Event2StdEventConverter::unpackTOT(  std::vector<unsigned char> data, size_t offset ) const {
    unsigned short tot = 0;
    for( unsigned int j = 0; j < 2; j++ ) {
      tot = tot | ( data[offset+j] << j*8 );
    }
    return tot;
  }

  uint64_t Timepix3Event2StdEventConverter::unpackTS( std::vector<unsigned char> data, size_t offset ) const {
    uint64_t ts = 0;
    for( unsigned int j = 0; j < 8; j++ ) {
      ts = ts | ( data[offset+j] << j*8 );
    }
    return ts;
  }
}
