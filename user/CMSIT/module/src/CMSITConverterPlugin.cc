/*!
  \file                  CMSITConverterPlugin.cc
  \brief                 Implementaion of CMSIT Converter Plugin for EUDAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  23/08/21
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "CMSITConverterPlugin.hh"

using namespace eudaq;

// #####################################
// # Row, Column, and Charge converter #
// #####################################
void TheConverter::ConverterForQuad(int& row, int& col, const int& chipIdMod4)
{
    // ###########################################
    // # @TMP@: mapping quad chips is hard-coded #
    // ###########################################
    if(chipIdMod4 == 15)
    {
        row = nRows + row;
        col = nCols + col;
    }
    else if(chipIdMod4 == 14)
    {
        row = nRows + row;
        col = col;
    }
    else if(chipIdMod4 == 13)
    {
        row = nRows - 1 - row;
        col = nCols - 1 - col;
    }
    else if(chipIdMod4 == 12)
    {
        row = nRows - 1 - row;
        col = nCols + nCols - 1 - col;
    }
}

void TheConverter::ConverterForDual(int& row, int& col, const int& chipIdMod4)
{
    row = row;
    col = col;
    if(chipIdMod4 == 1) col = nCols + col;
}

void TheConverter::ConverterFor25x100origR1C0(int& row, int& col, int& charge, const int& chipIdMod4, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);

    if(theSensor == TheConverter::SensorType::QuadChip)
        TheConverter::ConverterForQuad(row, col, chipIdMod4);
    else if(theSensor == TheConverter::SensorType::DualChip)
        TheConverter::ConverterForDual(row, col, chipIdMod4);

    row = (2 * row + 1) - (col % 2);
    col = floor(col / 2.);
}

void TheConverter::ConverterFor25x100origR0C0(int& row, int& col, int& charge, const int& chipIdMod4, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);

    if(theSensor == TheConverter::SensorType::QuadChip)
        TheConverter::ConverterForQuad(row, col, chipIdMod4);
    else if(theSensor == TheConverter::SensorType::DualChip)
        TheConverter::ConverterForDual(row, col, chipIdMod4);

    row = 2 * row + (col % 2);
    col = floor(col / 2.);
}

void TheConverter::ConverterFor50x50(int& row, int& col, int& charge, const int& chipIdMod4, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);

    if(theSensor == SensorType::QuadChip)
        TheConverter::ConverterForQuad(row, col, chipIdMod4);
    else if(theSensor == SensorType::DualChip)
        TheConverter::ConverterForDual(row, col, chipIdMod4);
}

void TheConverter::operator()(int& row, int& col, int& charge, const int& chipIdMod4, const calibrationParameters& calibPar, const int& chargeCut)
{
    (this->*whichConverter)(row, col, charge, chipIdMod4, calibPar, chargeCut);
}

int TheConverter::ChargeConverter(const int row, const int col, const int ToT, const calibrationParameters& calibPar, const int& chargeCut)
{
#ifdef ROOTSYS
    if((calibPar.hIntercept != nullptr) && (calibPar.hSlope != nullptr) && (calibPar.hSlope->GetBinContent(col + 1, row + 1) != 0))
    {
        double value = -1;

        if((isnan(calibPar.hIntercept->GetBinContent(col + 1, row + 1)) == false) && (isnan(calibPar.hSlope->GetBinContent(col + 1, row + 1)) == false))
            value = (ToT - calibPar.hIntercept->GetBinContent(col + 1, row + 1)) / calibPar.hSlope->GetBinContent(col + 1, row + 1) * calibPar.slopeVCal2Charge + calibPar.interceptVCal2Charge;

        return (value > chargeCut ? value : -1);
    }
#endif
    return (ToT > chargeCut ? ToT : -1);
}

bool CMSITConverterPlugin::Converting(EventSPC ev, StandardEventSP sev, ConfigurationSPC conf) const
{
    if(ev->IsEORE() == true) return true;

    // #########################################################
    // # Check if dataformat and encoder versions are the same #
    // #########################################################
    if(stoi(ev->GetTag("Dataformat version")) != CMSITEventData::DataFormatVersion)
        throw std::runtime_error("[EUDAQ::CMSITConverterPlugin::Converting] ERROR: version mismatch between dataformat " + ev->GetTag("Dataformat version") + " and encoder " +
                                 std::to_string(CMSITEventData::DataFormatVersion));

    CMSITEventData::EventData theEvent;
    if(CMSITConverterPlugin::Deserialize(ev, theEvent) == true)
    {
        std::vector<int> deviceIDs;
        for(auto i = 0; i < theEvent.chipData.size(); i++)
        {
            int                                 chargeCut;
            int                                 triggerIdLow;
            int                                 triggerIdHigh;
            TheConverter::calibrationParameters theCalibPar;
            std::string                         chipTypeFromFile;
            const auto&                         theChip = theEvent.chipData[i];
            const auto deviceId = CMSITConverterPlugin::ReadConfigurationAndComputeDeviceId(theChip.hybridId, theChip.chipId, chipTypeFromFile, theCalibPar, chargeCut, triggerIdLow, triggerIdHigh);

            if(std::find(deviceIDs.begin(), deviceIDs.end(), deviceId) == deviceIDs.end())
            {
                int         nRows, nCols, planeId;
                std::string ChipType;
                auto        theConverter = CMSITConverterPlugin::GetChipGeometry(theChip.chipType, chipTypeFromFile, nRows, nCols, ChipType, deviceId, planeId);

                StandardPlane plane;
                try
                {
                    plane = sev->GetPlane(planeId);
                }
                catch(...)
                {
                    plane = StandardPlane(planeId, EVENT_TYPE, ChipType);
                    plane.SetSizeZS(nCols, nRows, 0, MAXFRAMES, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
                }
                deviceIDs.push_back(deviceId);

                // #######################################
                // # Check possible synchronization loss #
                // #######################################
                if(theEvent.tluTriggerId != theTLUtriggerId_previous + 1)
                {
                    if(exitIfOutOfSync == true)
                    {
                        std::stringstream myString;
                        myString.clear();
                        myString.str("");
                        myString << "[EUDAQ::CMSITConverterPlugin::GetStandardSubEvent] WARNING: possible loss of synchronization, current trigger ID " << theEvent.tluTriggerId
                                 << ", previus trigger ID " << theTLUtriggerId_previous;
                        EUDAQ_ERROR(myString.str().c_str());
                        exit(EXIT_FAILURE);
                    }
                }
                theTLUtriggerId_previous = (theEvent.tluTriggerId == MAXTRIGIDCNT ? -1 : theEvent.tluTriggerId);

                // #####################
                // # Search for frames #
                // #####################
                int triggerId = 0;
                for(auto j = i; j < theEvent.chipData.size(); j++)
                    if((theEvent.chipData[j].hybridId == theChip.hybridId) && (theEvent.chipData[j].chipId == theChip.chipId))
                    {
                        if((triggerId >= triggerIdLow) && ((triggerIdHigh >= 0) && (triggerId <= triggerIdHigh)) || (triggerIdHigh < 0))
                            for(const auto& hit: theEvent.chipData[j].hits)
                            {
                                int row    = hit.row;
                                int col    = hit.col;
                                int charge = hit.tot;
                                theConverter(row, col, charge, theEvent.chipData[j].chipIdMod4, theCalibPar, chargeCut);
                                if((chargeCut >= 0) && (charge < 0)) continue;
                                plane.PushPixel((uint32_t)col, (uint32_t)row, charge, (uint32_t)triggerId);
                            }

                        triggerId++;
                    }

                try
                {
                    sev->GetPlane(planeId);
                }
                catch(...)
                {
                    sev->AddPlane(plane);
                }
            }
            else
                break;
        }

        return true;
    }

    return false;
}

void CMSITConverterPlugin::Initialize()
{
    theTLUtriggerId_previous = 0;
    theConfigFromFile        = nullptr;
    std::ifstream cfgFile(CFG_FILE_NAME);

    if(cfgFile.good() == true)
    {
        std::stringstream myString;
        myString.clear();
        myString.str("");
        myString << "[EUDAQ::CMSITConverterPlugin::Initialize] --> Found cfg file: " << CFG_FILE_NAME;
        EUDAQ_INFO(myString.str().c_str());
        theConfigFromFile = std::make_shared<Configuration>(Configuration(cfgFile));

        if(theConfigFromFile != nullptr)
        {
            // ########################
            // # Fill calibration map #
            // ########################
            theConfigFromFile->SetSection("sensor.calibration");

            for(auto chipId = 0; chipId < MAXCHIPID; chipId++)
                for(auto hybridId = 0; hybridId < MAXHYBRID; hybridId++)
                {
                    const std::string calibration("fileName_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
                    const std::string calibFileName(theConfigFromFile->Get(calibration, ""));

                    if(calibFileName != "")
                    {
                        calibMap[calibration] = TheConverter::calibrationParameters();
#ifdef ROOTSYS
                        calibMap[calibration].calibrationFile = TFile::Open(calibFileName.c_str());
                        if((calibMap[calibration].calibrationFile != nullptr) && (calibMap[calibration].calibrationFile->IsOpen() == true))
                        {
                            myString.clear();
                            myString.str("");
                            myString << "[EUDAQ::CMSITConverterPlugin::Initialize] --> Opening calibration file: " << calibFileName;
                            EUDAQ_INFO(myString.str().c_str());

                            TDirectory* rootDir          = gDirectory;
                            calibMap[calibration].hSlope = CMSITConverterPlugin::FindHistogram("Slope2D", hybridId, chipId);

                            rootDir->cd();
                            calibMap[calibration].hIntercept = CMSITConverterPlugin::FindHistogram("Intercept2D", hybridId, chipId);

                            const std::string slope("slopeVCal2Electrons_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
                            calibMap[calibration].slopeVCal2Charge = theConfigFromFile->Get(slope, 0.);

                            const std::string intercept("interceptVCal2Electrons_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
                            calibMap[calibration].interceptVCal2Charge = theConfigFromFile->Get(intercept, 0.);
                        }
                        else
                        {
                            myString.clear();
                            myString.str("");
                            myString << "[EUDAQ::CMSITConverterPlugin::Initialize] --> I couldn't open the calibration file: " << calibFileName;
                            EUDAQ_INFO(myString.str().c_str());
                        }
#else
                        myString.clear();
                        myString.str("");
                        myString << "[EUDAQ::CMSITConverterPlugin::Initialize] --> ROOTSYS not defined";
                        EUDAQ_INFO(myString.str().c_str());
#endif
                    }
                }

            // ########################
            // # Fill calibration map #
            // ########################
            theConfigFromFile->SetSection("converter.settings");
            exitIfOutOfSync = theConfigFromFile->Get("exitIfOutOfSync", false);
        }

        cfgFile.close();
    }
    else
    {
        std::stringstream myString;
        myString.clear();
        myString.str("");
        myString << "[EUDAQ::CMSITConverterPlugin::Initialize] --> I couldn't find cfg file: " << CFG_FILE_NAME << " (it's not mandatory)";
        EUDAQ_INFO(myString.str().c_str());
    }
}

TheConverter CMSITConverterPlugin::GetChipGeometry(const std::string& cfgFromData, const std::string& cfgFromFile, int& nRows, int& nCols, std::string& ChipType, int deviceId, int& planeId) const
{
    TheConverter theConverter;
    std::string  cfg = (cfgFromFile != "" ? cfgFromFile : cfgFromData);
    nRows            = (cfg.find("RD53A") != std::string::npos ? NROWS_RD53A : NROWS_RD53B);
    nCols            = (cfg.find("RD53A") != std::string::npos ? NCOLS_RD53A : NCOLS_RD53B);
    ChipType         = (cfg.find("RD53A") != std::string::npos ? CHIPTYPE_RD53A : CHIPTYPE_RD53B);

    theConverter.whichConverter = &TheConverter::ConverterFor50x50;
    theConverter.theSensor      = TheConverter::SensorType::SingleChip;

    if((cfg.find("25x100") != std::string::npos) || (cfg.find("100x25") != std::string::npos))
    {
        nCols /= 2;
        nRows *= 2;
    }

    if(cfg.find("dual") != std::string::npos)
    {
        theConverter.theSensor = TheConverter::SensorType::DualChip;
        nCols *= 2;
    }
    else if(cfg.find("quad") != std::string::npos)
    {
        theConverter.theSensor = TheConverter::SensorType::QuadChip;
        nCols *= 2;
        nRows *= 2;
    }

    theConverter.nCols = nCols;
    theConverter.nRows = nRows;

    if((cfg.find("25x100origR0C0") != std::string::npos) || (cfg.find("100x25origR0C0") != std::string::npos))
        theConverter.whichConverter = &TheConverter::ConverterFor25x100origR0C0;
    else if((cfg.find("25x100origR1C0") != std::string::npos) || (cfg.find("100x25origR1C0") != std::string::npos))
        theConverter.whichConverter = &TheConverter::ConverterFor25x100origR1C0;

    // #############################################
    // # @TMP@: compute plane ID from device ID    #
    // # Quad modules are coded as chipId = QUADID #
    // # Dual modules are coded as chipId = DUALID #
    // #############################################
    planeId = deviceId;
    if(theConverter.theSensor == TheConverter::SensorType::QuadChip)
        planeId = round(deviceId / CMSITplaneIdOffset) * CMSITplaneIdOffset + QUADID;
    else if(theConverter.theSensor == TheConverter::SensorType::QuadChip)
        planeId = round(deviceId / CMSITplaneIdOffset) * CMSITplaneIdOffset + DUALID;

    return theConverter;
}

#ifdef ROOTSYS
TH2D* CMSITConverterPlugin::FindHistogram(const std::string& nameInHisto, uint16_t hybridId, uint16_t chipId)
{
    TDirectory* dir = gDirectory;
    TKey*       key = nullptr;

    // ########################
    // # Find sub-directories #
    // ########################
    while(true)
    {
        TIter keyList(dir->GetListOfKeys());
        if(((key = (TKey*)keyList.Next()) != nullptr) && (key->IsFolder() == true) && (std::string(key->GetName()).find(std::string("Hybrid")) == std::string::npos))
        {
            dir->cd(key->GetName());
            dir = gDirectory;
        }
        else
            break;
    }

    // #####################
    // # Search for Hybrid #
    // #####################
    TIter keyListHybrid(dir->GetListOfKeys());
    while((key != nullptr) && (key->IsFolder() == true) && (std::string(key->GetName()).find(std::string("Hybrid_") + std::to_string(hybridId)) == std::string::npos))
        key = (TKey*)keyListHybrid.Next();

    // ###################
    // # Search for Chip #
    // ###################
    if(key != nullptr)
    {
        dir->cd(key->GetName());
        dir = gDirectory;
    }
    TIter keyListChip(dir->GetListOfKeys());
    while((key != nullptr) && (key->IsFolder() == true) && (std::string(key->GetName()).find(std::string("Chip_") + std::to_string(chipId)) == std::string::npos)) key = (TKey*)keyListChip.Next();

    // ###########################
    // # Enter in Chip directory #
    // ###########################
    if(key != nullptr) dir->cd(key->GetName());
    dir = gDirectory;
    TIter keyListHisto(dir->GetListOfKeys());

    // ######################
    // # Find the histogram #
    // ######################
    while(((key = (TKey*)keyListHisto.Next()) != nullptr) && (std::string(key->GetName()).find(nameInHisto) == std::string::npos)) {};
    if((dir != nullptr) && (dir->Get(key->GetName()) != nullptr)) return (TH2D*)((TCanvas*)dir->Get(key->GetName()))->GetPrimitive(key->GetName());

    return nullptr;
}
#endif

bool CMSITConverterPlugin::Deserialize(const EventSPC ev, CMSITEventData::EventData& theEvent) const
{
    // Make sure the event is of class RawDataEvent
    if(auto rev = static_cast<const RawDataEvent*>(ev.get()))
    {
        if((rev->NumBlocks() > 0) && (rev->GetBlock(0).size() > 0))
        {
            std::istringstream              theSerialized(std::string((char*)rev->GetBlock(0).data(), rev->GetBlock(0).size()));
            boost::archive::binary_iarchive theArchive(theSerialized);
            theArchive >> theEvent;

            return true;
        }
    }

    return false;
}

int CMSITConverterPlugin::ReadConfigurationAndComputeDeviceId(const uint32_t                       hybridId,
                                                              const uint32_t                       chipId,
                                                              std::string&                         chipTypeFromFile,
                                                              TheConverter::calibrationParameters& calibPar,
                                                              int&                                 chargeCut,
                                                              int&                                 triggerIdLow,
                                                              int&                                 triggerIdHigh) const
{
    // #################
    // # Set chip type #
    // #################
    const std::string pitch("pitch_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
    if(theConfigFromFile != nullptr)
    {
        theConfigFromFile->SetSection("sensor.geometry");
        chipTypeFromFile = theConfigFromFile->Get(pitch, "");
    }
    else
        chipTypeFromFile = "";

    // ##############################
    // # Set calibration parameters #
    // ##############################
    const std::string calibration("fileName_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
    if(calibMap.find(calibration) != calibMap.end()) calibPar = calibMap.at(calibration);

    // ##################
    // # Set charge cut #
    // ##################
    const std::string charge("charge_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
    if(theConfigFromFile != nullptr)
    {
        theConfigFromFile->SetSection("sensor.selection");
        chargeCut = theConfigFromFile->Get(charge, -1);
    }
    else
        chargeCut = -1;

    // #####################
    // # Set triggerID cut #
    // #####################
    const std::string triggerIdL("triggerIdLow_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
    const std::string triggerIdH("triggerIdHigh_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
    if(theConfigFromFile != nullptr)
    {
        theConfigFromFile->SetSection("sensor.selection");
        triggerIdLow  = theConfigFromFile->Get(triggerIdL, -1);
        triggerIdHigh = theConfigFromFile->Get(triggerIdH, -1);
    }
    else
    {
        triggerIdLow  = -1;
        triggerIdHigh = -1;
    }

    // ########################################################################################################
    // # Generate a unique deviceId: 100 (CMSIT offset) + 100 * hybridId (hybrid index) + chipId (chip index) #
    // # Don't use these ranges (Needs to be unique to each ROC):                                             #
    // # (-) 0-10:  used by NIConverter/MIMOSA                                                                #
    // # (-) 25-30: used by USBPixGen3Converter/FEI-4                                                         #
    // # (-) 30+:   used by BDAQ53Converter/RD53 with same model (30 [BDAQ offset] + 10 * boardId + chipId)   #
    // ########################################################################################################
    return CMSITplaneIdOffset + 100 * hybridId + chipId;
}

bool                                                       CMSITConverterPlugin::exitIfOutOfSync          = false;
int                                                        CMSITConverterPlugin::theTLUtriggerId_previous = 0;
std::map<std::string, TheConverter::calibrationParameters> CMSITConverterPlugin::calibMap                 = {};
std::shared_ptr<Configuration>                             CMSITConverterPlugin::theConfigFromFile        = nullptr;
std::once_flag                                             CMSITConverterPlugin::callOnce;

namespace
{
auto dummy0 = Factory<StdEventConverter>::Register<CMSITConverterPlugin>(CMSITConverterPlugin::m_id_factory);
}
