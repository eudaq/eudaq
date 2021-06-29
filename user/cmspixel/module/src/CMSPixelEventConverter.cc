#include "CMSPixelEvent2StdEventConverter.hh"

#include "dictionaries.h"
#include "constants.h"

using namespace pxar;
using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPixelDUT2StdEventConverter>(CMSPixelDUT2StdEventConverter::m_id_factory);
  auto dummy1 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPixelREF2StdEventConverter>(CMSPixelREF2StdEventConverter::m_id_factory);
  auto dummy2 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPixelTRP2StdEventConverter>(CMSPixelTRP2StdEventConverter::m_id_factory);
  auto dummy3 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<CMSPixelQUAD2StdEventConverter>(CMSPixelQUAD2StdEventConverter::m_id_factory);
}

uint8_t CMSPixelBaseConverter::m_roctype, CMSPixelBaseConverter::m_tbmtype;
size_t CMSPixelBaseConverter::m_planeid;
size_t CMSPixelBaseConverter::m_nplanes;
std::string CMSPixelBaseConverter::m_detector;
bool CMSPixelBaseConverter::m_rotated_pcb;
bool CMSPixelBaseConverter::m_is_initialized = false;
pxar::evtSource CMSPixelBaseConverter::src;
pxar::passthroughSplitter CMSPixelBaseConverter::splitter;
pxar::dtbEventDecoder CMSPixelBaseConverter::decoder;
pxar::dataSink<pxar::Event *> CMSPixelBaseConverter::Eventpump;
bool CMSPixelBaseConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const {

  EUDAQ_DEBUG("Starting event decoding");

  // If we receive the EORE print the collected statistics:
  if (d1->IsEORE()) {
    // Set decoder to INFO level for statistics printout:
    //std::cout << "Decoding statistics for detector " << m_detector << std::endl;
    //pxar::Log::ReportingLevel() = pxar::Log::FromString("INFO");
    //decoder.getStatistics().dump();
    return true;
  }
  // Check if we have BORE:
  else if (d1->IsBORE()) {
    EUDAQ_INFO("Starting initialization...");
    Initialize(d1, conf);
    return true;
  }

  if(!m_is_initialized) {
    return false;
  }

  auto in_raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  // No event
  if(!in_raw) {
    return false;
  }

  // Check of we have more than one data block:
  if (in_raw->NumBlocks() > 1) {
    EUDAQ_ERROR("Only one data block is expected!");
    return false;
  }

  // Transform from EUDAQ data, add it to the datasource:
  src.AddData(TransformRawData(in_raw->GetBlock(0)));
  // ...and pull it out at the other end:
  pxar::Event *evt = Eventpump.Get();

  if (m_tbmtype <= TBM_EMU) {
    // If we have no TBM or a TBM Emulator, assume this is
    // some sort of multi-plane telescope:
    GetMultiPlanes(d2, m_planeid, evt);
  } else {
    // If we have a real TBM attached, this is probably
    // a module with just one sensor plane:
    GetSinglePlane(d2, m_planeid, evt);
  }


  d2->SetEventN(d1->GetEventN());
  d2->SetTriggerN(d1->GetTriggerN());

  // Identify the detetor type
  d2->SetDetectorType("CMSPixel");
  return true;
}

void CMSPixelBaseConverter::Initialize(eudaq::EventSPC bore, eudaq::ConfigurationSPC conf) const {
  DeviceDictionary *devDict;
  std::string roctype = bore->GetTag("ROCTYPE", "");
  std::string tbmtype = bore->GetTag("TBMTYPE", "notbm");

  m_detector = bore->GetTag("DETECTOR", "");
  std::string pcbtype = bore->GetTag("PCBTYPE", "");
  m_rotated_pcb =
      (pcbtype.find("-rot") != std::string::npos ? true : false);

  // Get the number of planes:
  m_nplanes = bore->GetTag("PLANES", 1);

  m_roctype = devDict->getInstance()->getDevCode(roctype);
  m_tbmtype = devDict->getInstance()->getDevCode(tbmtype);

  if (m_roctype == 0x0) {
    EUDAQ_THROW("Data contains invalid ROC type: " + roctype);
  }
  if(m_tbmtype == 0x0) {
EUDAQ_THROW("Data contains invalid TBM type: " + tbmtype);
  }

  std::cout << "CMSPixel Converter initialized with detector " << m_detector
            << ", Event Type " << event_type() << ", TBM type " << tbmtype
            << " (" << static_cast<int>(m_tbmtype) << ")"
            << ", ROC type " << roctype << " ("
            << static_cast<int>(m_roctype) << ")" << std::endl;

  if (m_detector == "TRP") {
    m_planeid = 6;
  } // TRP
  else if (m_detector == "DUT") {
    m_planeid = 7;
  } // DUT
  else if (m_detector == "REF") {
    m_planeid = 8;
  } // REF
  else {
    m_planeid = 9;
  } // QUAD

  // Set decoder to reasonable verbosity (still informing about problems:
  pxar::Log::ReportingLevel() = pxar::Log::FromString("WARNING");
  //pxar::Log::ReportingLevel() = pxar::Log::FromString("DEBUGPIPES");

  // Connect the data source and set up the pipe:
  src = evtSource(0, m_nplanes, 0, m_tbmtype, m_roctype, FLAG_DISABLE_EVENTID_CHECK);
  src >> splitter >> decoder >> Eventpump;

  EUDAQ_DEBUG("Finished initializing CMSPixel converter for detector " + m_detector);
  m_is_initialized = true;
}

inline uint16_t CMSPixelBaseConverter::roc_to_mod_row(uint8_t roc, uint16_t row) {
  if (roc < 8)
    return row;
  else
    return (2 * ROC_NUMROWS - row - 1);
};

inline uint16_t CMSPixelBaseConverter::roc_to_mod_col(uint8_t roc, uint16_t col) {
  if (roc < 8)
    return (col + roc * ROC_NUMCOLS);
  else
    return ((16 - roc) * ROC_NUMCOLS - col - 1);
};

std::vector<uint16_t> CMSPixelBaseConverter::TransformRawData(const std::vector<unsigned char> &block) {

  // Transform data of form char* to vector<int16_t>
  std::vector<uint16_t> rawData;

  int size = block.size();
  if (size < 2) {
    return rawData;
  }

  int i = 0;
  while (i < size - 1) {
    uint16_t temp = ((uint16_t)block.data()[i + 1] << 8) | block.data()[i];
    rawData.push_back(temp);
    i += 2;
  }
  return rawData;
};

void CMSPixelBaseConverter::GetSinglePlane(eudaq::StandardEventSP d2, unsigned plane_id, pxar::Event *evt) const {

  // Create a new StandardPlane object:
  StandardPlane plane(plane_id, event_type(), m_detector);

  // More than 16 ROCs on one module?! Impossible!
  if (m_nplanes > 16)
  return;

  // Initialize the plane size (zero suppressed), set the number of pixels
  // Check which carrier PCB has been used and book planes accordingly:
  if (m_rotated_pcb) {
    plane.SetSizeZS(2 * ROC_NUMROWS, 8 * ROC_NUMCOLS, 0);
  } else {
    plane.SetSizeZS(8 * ROC_NUMCOLS, 2 * ROC_NUMROWS, 0);
  }

  // Iterate over all pixels and place them in the module plane:
  for (std::vector<pxar::pixel>::iterator it = evt->pixels.begin();
  it != evt->pixels.end(); ++it) {

    if (m_rotated_pcb) {
      plane.PushPixel(roc_to_mod_row(it->roc(), it->row()),
      roc_to_mod_col(it->roc(), it->column()), it->value());
    } else {
      plane.PushPixel(roc_to_mod_col(it->roc(), it->column()),
      roc_to_mod_row(it->roc(), it->row()), it->value());
    }
  }

  // Add plane to the output event:
  d2->AddPlane(plane);
}

void CMSPixelBaseConverter::GetMultiPlanes(eudaq::StandardEventSP d2, unsigned plane_id, pxar::Event *evt) const {
  // Iterate over all planes and check for pixel hits:
  for (size_t roc = 0; roc < m_nplanes; roc++) {

    // Create a new StandardPlane object:
    StandardPlane plane(plane_id + roc, event_type(), m_detector);

    // Initialize the plane size (zero suppressed), set the number of pixels
    // Check which carrier PCB has been used and book planes accordingly:
    if (m_rotated_pcb) {
      plane.SetSizeZS(ROC_NUMROWS, ROC_NUMCOLS, 0);
    } else {
      plane.SetSizeZS(ROC_NUMCOLS, ROC_NUMROWS, 0);
    }

    // Store all decoded pixels belonging to this plane:
    for (std::vector<pxar::pixel>::iterator it = evt->pixels.begin();
    it != evt->pixels.end(); ++it) {
      // Check if current pixel belongs on this plane:
      if (it->roc() == roc) {
        if (m_rotated_pcb) {
          plane.PushPixel(it->row(), it->column(), it->value());
        } else {
          plane.PushPixel(it->column(), it->row(), it->value());
        }
      }
    }

    // Add plane to the output event:
    d2->AddPlane(plane);
  }
}
