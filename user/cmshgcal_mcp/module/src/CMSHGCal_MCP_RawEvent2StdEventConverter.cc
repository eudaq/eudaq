#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "CAEN_v17XX_Unpacker.h"

const std::string EVENT_TYPE = "CMSHGCal_MCP_RawEvent";

//must be adjusted to each setup of the digitizer
//todo: move hard coded values into the configuration
#define Nchannels 9
#define Ngroups 4
#define Nsamples 1024

class CMSHGCAL_MCP_RawEvent2StdEventConverter: public eudaq::StdEventConverter {
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("CMSHGCal_MCP_RawEvent");
  CMSHGCAL_MCP_RawEvent2StdEventConverter();
  ~CMSHGCAL_MCP_RawEvent2StdEventConverter();
private:
  CAEN_V1742_Unpacker* unpacker;
};

namespace {
auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
              Register<CMSHGCAL_MCP_RawEvent2StdEventConverter>(CMSHGCAL_MCP_RawEvent2StdEventConverter::m_id_factory);
}

CMSHGCAL_MCP_RawEvent2StdEventConverter::CMSHGCAL_MCP_RawEvent2StdEventConverter() : eudaq::StdEventConverter() {
  unpacker = new CAEN_V1742_Unpacker;
}

CMSHGCAL_MCP_RawEvent2StdEventConverter::~CMSHGCAL_MCP_RawEvent2StdEventConverter() { delete unpacker;}

bool CMSHGCAL_MCP_RawEvent2StdEventConverter::Converting(eudaq::EventSPC rev, eudaq::StdEventSP sev, eudaq::ConfigSPC conf) const {
  // If the event type is used for different sensors
  // they can be differentiated here
  const std::string sensortype = "DIGITIZER";

  sev->SetTag("cpuTime_mcpProducer_mus", rev->GetTimestampBegin());

  const unsigned nBlocks = rev->NumBlocks();
  if (nBlocks > 1) {
    std::cout << "More than one blocks in the MCPConverter, stop!!!" << std::endl;
    return false;
  }

  std::vector<digiData> converted_samples;
  unsigned int digitizer_triggerTimeTag;
  for (unsigned digititzer_index = 0; digititzer_index < nBlocks; digititzer_index++) {
    std::vector<uint8_t> bl = rev->GetBlock(digititzer_index);

    //conversion block
    std::vector<uint32_t> Words;
    Words.resize(bl.size() / sizeof(uint32_t));
    std::memcpy(&Words[0], &bl[0], bl.size());

    unpacker->Unpack(Words, converted_samples, digitizer_triggerTimeTag);

  }
  sev->SetTag("digitizer_triggertimetag", digitizer_triggerTimeTag);

  eudaq::StandardPlane digitizer_full_plane(0, EVENT_TYPE, sensortype);
  digitizer_full_plane.SetSizeRaw(Ngroups, Nchannels, Nsamples + 1);
  for (int gr = 0; gr < Ngroups; gr++) for (int ch = 0; ch < Nchannels; ch++) digitizer_full_plane.SetPixel(gr * Nchannels + ch, gr, ch, Nsamples, false, 0);

  //plane for offline reconstruction
  int gr, ch, sample_index;
  for (int i = 0; i < converted_samples.size(); i++) {
    gr = converted_samples[i].group;
    ch = converted_samples[i].channel;
    sample_index = converted_samples[i].sampleIndex;
    digitizer_full_plane.SetPixel(gr * Nchannels + ch, gr, ch, converted_samples[i].sampleValue, false, 1 + sample_index);
  }
  sev->AddPlane(digitizer_full_plane);


  // Indicate that data was successfully converted
  return true;

}
