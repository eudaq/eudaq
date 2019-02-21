#include "CaribouEvent2StdEventConverter.hh"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CLICpix2Event2StdEventConverter>(CLICpix2Event2StdEventConverter::m_id_factory);
}

bool CLICpix2Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  conf->Print(std::cout);
  // No event
  if(!ev) {
    return false;
  }

  // Bad event
  if (ev->NumBlocks() != 2) {
    EUDAQ_WARN("Ignoring bad frame " + std::to_string(ev->GetEventNumber()));
    return false;
  }

  // Block 1 is timestamps:
  std::vector<unsigned char> data = ev->GetBlock( 1 ); // block 1 is pixel data

  // Create a StandardPlane representing one sensor plane
  eudaq::StandardPlane plane(0, "CLICpix2", "CLICpix2");

  // Size of one pixel data chunk: 12 bytes = 1+1+2+8 bytes for x,y,tot,ts
  // const unsigned int PIX_SIZE = 12;
  // plane.SetSizeZS( 128, 128, data.size() / PIX_SIZE );
  //
  // for( size_t i = 0 ; i < ZSDataX.size(); ++i ) {
  //   plane.SetPixel( i, ZSDataX[i], ZSDataY[i], ZSDataTOT[i] );
  // }

  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Indicate that data was successfully converted
  return true;
}
