#include "eudaq/RawEvent.hh"
//#include "eudaq/TTreeEventConverter.hh"
#include "TTreeEventConverter.hh"
#include "CAENDigitizerType.h" 
#include "CAENDigitizer.h"
#include "FERS_Registers_5202.h"
#include "FERSlib.h"

#include "DRS_EUDAQ.h"
#include "FERS_EUDAQ.h"
#include "QTP_EUDAQ.h"


class QTPDProducerEvent2TTreeEventConverter : public eudaq::TTreeEventConverter {
public:
    bool Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const override;
    static const uint32_t m_id_factory;


};

const uint32_t QTPDProducerEvent2TTreeEventConverter::m_id_factory = eudaq::cstr2hash("QTPDProducer");

namespace {
    auto dummy0 = eudaq::Factory<eudaq::TTreeEventConverter>::
        Register<QTPDProducerEvent2TTreeEventConverter>(QTPDProducerEvent2TTreeEventConverter::m_id_factory);
}

bool QTPDProducerEvent2TTreeEventConverter::Converting(eudaq::EventSPC d1, eudaq::TTreeEventSP d2, eudaq::ConfigSPC conf) const {
    auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
    if (!ev) {
        EUDAQ_THROW("Failed to cast event to RawEvent");
    }else{
	//std::cout<<" ------- 8888 ------"<<std::endl;
    }



    int brd;
    int PID[16];
    int iPID;

    uint64_t trigTime[2];
    uint16_t ADCdata[32];

    //SpectEvent_t spect_event[16];
    trigTime[1]=ev->GetTimestampBegin()/1000;
    auto block_n_list = ev->GetBlockNumList();
    for(auto &block_n: block_n_list){
	std::vector<uint8_t> block = ev->GetBlock(block_n);
        int index = read_header(&block, &brd, &iPID);
	PID[brd]=iPID;

        std::vector<uint8_t> data(block.begin()+index, block.end());
	if(data.size()>0) {
		QTPunpack_event(&data, ADCdata);


                TString boardPIDBranchName = TString::Format("QDC_Board_%d_PID", brd);

                if (d2->GetListOfBranches()->FindObject(boardPIDBranchName)) {
                        d2->SetBranchAddress(boardPIDBranchName, &PID[brd]);
                } else {
                        d2->Branch(boardPIDBranchName, &PID[brd], "PID/I");
                }

		TString branch_name = TString::Format("QDC_Board%d_ADC", brd);
		if (d2->GetListOfBranches()->FindObject(branch_name)) d2->SetBranchAddress(branch_name, ADCdata);
		else d2->Branch(branch_name, ADCdata, "ADC[32]/s");



	}
    } //end block






    return true;
}
