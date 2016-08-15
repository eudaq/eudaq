#ifndef Processors_h__
#define Processors_h__
#include <vector>
#include <memory>
#include "Platform.hh"
#include "EventSynchronisationBase.hh"
#include "eudaq_types.hh"
namespace eudaq {
class ProcessorBase;
class Processor_Inspector;
class fileConfig;
namespace Processors {
using processor_up = std::unique_ptr<eudaq::ProcessorBase>;
using processor_i_up = std::unique_ptr<eudaq::Processor_Inspector>;
DLLEXPORT processor_i_up eventSelector(const std::vector<unsigned>& eventsOfIntresst, bool doBore = true, bool doEORE = true);
DLLEXPORT processor_i_up ShowEventNR(size_t repetition);
DLLEXPORT processor_up fileReader(const eudaq::fileConfig &);
DLLEXPORT processor_i_up fileWriter();
DLLEXPORT processor_i_up fileWriter(const std::string& name, const std::string& param_ = "");


DLLEXPORT processor_up merger(const SyncBase::MainType& type_, SyncBase::Parameter_ref param_ = SyncBase::Parameter_t());
DLLEXPORT processor_up dataReciver(const std::string& listAdrrs, eudaq_types::outPutString outPut_connectionName);
DLLEXPORT processor_up dataReciver(const std::string& listAdrrs);



DLLEXPORT processor_i_up dataSender(const std::string& serverAdress, const std::string& type_ = "processor", const std::string& name_ = "default");

DLLEXPORT processor_up waitForEORE(int timeIn_ms=200);



}

}

#endif // Processors_h__
