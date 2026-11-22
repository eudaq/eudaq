#include "eudaq/RawEvent.hh"
//#include "eudaq/TTreeEventConverter.hh"
#include "TTreeEventConverter.hh"
#include "CAENDigitizerType.h" 
#include "CAENDigitizer.h"
#include "FERS_Registers_5202.h"
#include "FERSlib.h"

#include "DRS_EUDAQ.h"
#include "FERS_EUDAQ.h"
#include <set>

class FERSProducerEvent2TTreeEventConverter : public eudaq::TTreeEventConverter {
public:
    bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
    static const uint32_t m_id_factory;


};

const uint32_t FERSProducerEvent2TTreeEventConverter::m_id_factory = eudaq::cstr2hash("FERSProducer");

namespace {
    auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
        Register<FERSProducerEvent2TTreeEventConverter>(FERSProducerEvent2TTreeEventConverter::m_id_factory);
}

bool FERSProducerEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
    if (!ev) {
        EUDAQ_THROW("Failed to cast event to RawEvent");
    }

    static int PID[18];
    static float SipmHV[18];
    static float SipmI[18];
    static float TempDET[18];
    static float TempFPGA[18];
    static SpectEvent_t spect_event[18];
    static std::set<int> initialized_boards;

    uint64_t trigTime[2];
    trigTime[1] = ev->GetTimestampBegin() / 1000;

    auto block_n_list = ev->GetBlockNumList();
    for (auto &block_n : block_n_list) {
        std::vector<uint8_t> block = ev->GetBlock(block_n);
        int brd, iPID;
	float sipmHV,sipmI,tempDET,tempFPGA;
        int index = read_headerFERS(&block, &brd, &iPID,&sipmHV,&sipmI,&tempDET,&tempFPGA);

        // Board index safety check
        if (brd < 0 || brd >= 18) {
            std::cerr << "Warning: Invalid board index: " << brd << std::endl;
            continue;
        }

        PID[brd] = iPID;
	SipmHV[brd]=sipmHV;
	SipmI[brd]=sipmI;
	TempDET[brd]=tempDET;
	TempFPGA[brd]=tempFPGA;

        std::vector<uint8_t> data(block.begin() + index, block.end());
        if (data.empty()) continue;

        spect_event[brd] = FERSunpack_spectevent(&data);

        // Only initialize branches once per board
        if (initialized_boards.find(brd) == initialized_boards.end()) {
            TString base = TString::Format("FERS_Board%d_", brd);

            d2->Branch(base + "PID", &PID[brd], "PID/I");
            d2->Branch(base + "SipmHV", &SipmHV[brd], "SipmHV/F");
            d2->Branch(base + "SipmI", &SipmI[brd], "SipmI/F");
            d2->Branch(base + "TempDET", &TempDET[brd], "TempDET/F");
            d2->Branch(base + "TempFPGA", &TempFPGA[brd], "TempFPGA/F");
            d2->Branch(base + "tstamp_us", &spect_event[brd].tstamp_us, "tstamp_us/D");
            d2->Branch(base + "rel_tstamp_us", &spect_event[brd].rel_tstamp_us, "rel_tstamp_us/D");
            d2->Branch(base + "trigger_id", &spect_event[brd].trigger_id, "trigger_id/g");
            d2->Branch(base + "chmask", &spect_event[brd].chmask, "chmask/g");
            d2->Branch(base + "qdmask", &spect_event[brd].qdmask, "qdmask/g");
            d2->Branch(base + "energyHG", spect_event[brd].energyHG, "energyHG[64]/s");
            d2->Branch(base + "energyLG", spect_event[brd].energyLG, "energyLG[64]/s");

            initialized_boards.insert(brd);
        }

    }

    return true;
}

