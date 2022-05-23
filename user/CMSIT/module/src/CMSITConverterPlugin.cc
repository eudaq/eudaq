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
void TheConverter::ConverterFor25x100origR1C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
    row    = (2 * row + 1) - (col % 2);
    col    = floor(col / 2.);
    col += (!isSingleChip) * nCols * lane;
}

void TheConverter::ConverterFor25x100origR0C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
    row    = 2 * row + (col % 2);
    col    = floor(col / 2.);
    col += (!isSingleChip) * nCols * lane;
}

void TheConverter::ConverterFor50x50(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
{
    charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
    row    = row;
    col += (!isSingleChip) * nCols * lane;
}

void TheConverter::operator()(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
{
    (this->*whichConverter)(row, col, charge, lane, calibPar, chargeCut);
}

int TheConverter::ChargeConverter(const int row, const int col, const int ToT, const calibrationParameters& calibPar, const int& chargeCut)
{
#ifdef ROOTSYS
    if((calibPar.hIntercept != nullptr) && (calibPar.hSlope != nullptr) && (calibPar.hSlope->GetBinContent(col + 1, row + 1) != 0))
    {
        auto value = (ToT - calibPar.hIntercept->GetBinContent(col + 1, row + 1)) / calibPar.hSlope->GetBinContent(col + 1, row + 1) * calibPar.slopeVCal2Charge + calibPar.interceptVCal2Charge;
        return (value > chargeCut ? value : -1);
    }
#endif
    return (ToT > chargeCut ? ToT : -1);
}

bool CMSITConverterPlugin::Converting(EventSPC ev, StandardEventSP sev, ConfigurationSPC conf) const
{
    if(ev->IsEORE() == true) return true;

    CMSITEventData::EventData theEvent;
    if(CMSITConverterPlugin::Deserialize(ev, theEvent) == true)
    {
        std::vector<int> planeIDs;
        for(auto i = 0; i < theEvent.chipData.size(); i++)
        {
            int                                 chargeCut;
            int                                 triggerIdLow;
            int                                 triggerIdHigh;
            TheConverter::calibrationParameters theCalibPar;
            std::string                         chipTypeFromFile;
            const auto&                         theChip = theEvent.chipData[i];
            const auto                          planeId = CMSITConverterPlugin::ComputePlaneId(theChip.hybridId, theChip.chipId, chipTypeFromFile, theCalibPar, chargeCut, triggerIdLow, triggerIdHigh);

            if(std::find(planeIDs.begin(), planeIDs.end(), planeId) == planeIDs.end())
            {
                StandardPlane plane(planeId, EVENT_TYPE, SENSORTYPE);

                planeIDs.push_back(planeId);

                int    nRows, nCols;
                double pitchX, pitchY;
                auto   theConverter = CMSITConverterPlugin::GetChipGeometry(theChip.chipType, chipTypeFromFile, nRows, nCols, pitchX, pitchY);

                plane.SetSizeZS(nCols, nRows, 0, MAXFRAMES, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);

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
                        std::cout << myString << std::endl;
                        EUDAQ_ERROR(myString.str());
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
                                theConverter(row, col, charge, theEvent.chipData[j].chipLane, theCalibPar, chargeCut);
                                if((chargeCut >= 0) && (charge < 0)) continue;
                                plane.PushPixel((uint32_t)col, (uint32_t)row, charge, (uint32_t)triggerId);
                            }

                        triggerId++;
                    }

                sev->AddPlane(plane);
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
        std::cout << myString << std::endl;
        EUDAQ_INFO(myString.str());
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
                            std::cout << myString << std::endl;
                            EUDAQ_INFO(myString.str());

                            TDirectory* rootDir          = gDirectory;
                            calibMap[calibration].hSlope = CMSITConverterPlugin::FindHistogram("Slope2D");

                            rootDir->cd();
                            calibMap[calibration].hIntercept = CMSITConverterPlugin::FindHistogram("Intercept2D");

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
                            std::cout << myString << std::endl;
                            EUDAQ_INFO(myString.str());
                        }
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
        std::cout << myString << std::endl;
        EUDAQ_INFO(myString.str());
    }
}

TheConverter CMSITConverterPlugin::GetChipGeometry(const std::string& cfgFromData, const std::string& cfgFromFile, int& nRows, int& nCols, double& pitchX, double& pitchY) const
{
    TheConverter theConverter;
    std::string  cfg = (cfgFromFile != "" ? cfgFromFile : cfgFromData);
    nRows            = NROWS;
    nCols            = NCOLS;
    pitchX           = PITCHX; // [mm]
    pitchY           = PITCHY; // [mm]

    theConverter.whichConverter = &TheConverter::ConverterFor50x50;
    theConverter.isSingleChip   = true;

    if(cfg.find("25x100") != std::string::npos)
    {
        nCols /= 2;
        nRows *= 2;
        pitchX = 0.100;
        pitchY = 0.025;
    }

    if(cfg.find("dual") != std::string::npos)
    {
        nCols *= 2;
        theConverter.isSingleChip = false;
    }
    else if(cfg.find("quad") != std::string::npos)
    {
        nCols *= 4;
        theConverter.isSingleChip = false;
    }

    theConverter.nCols = nCols;

    if(cfg.find("25x100origR0C0") != std::string::npos)
        theConverter.whichConverter = &TheConverter::ConverterFor25x100origR0C0;
    else if(cfg.find("25x100origR1C0") != std::string::npos)
        theConverter.whichConverter = &TheConverter::ConverterFor25x100origR1C0;

    return theConverter;
}

#ifdef ROOTSYS
TH2D* CMSITConverterPlugin::FindHistogram(const std::string& nameInHisto)
{
    TDirectory* dir = gDirectory;
    TKey*       key;
    while(true)
    {
        TIter keyList(dir->GetListOfKeys());
        if(((key = (TKey*)keyList.Next()) != nullptr) && (key->IsFolder() == true) && (strcmp(key->GetName(), "Channel") != 0))
        {
            dir->cd(key->GetName());
            dir = gDirectory;
        }
        else
            break;
    }
    TIter keyList(dir->GetListOfKeys());
    while(((key = (TKey*)keyList.Next()) != nullptr) && (std::string(key->GetName()).find(nameInHisto) == std::string::npos)) {};

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

int CMSITConverterPlugin::ComputePlaneId(const uint32_t                       hybridId,
                                         const uint32_t                       chipId,
                                         std::string&                         chipTypeFromFile,
                                         TheConverter::calibrationParameters& calibPar,
                                         int&                                 chargeCut,
                                         int&                                 triggerIdLow,
                                         int&                                 triggerIdHigh) const
{
    // #######################################################################################################
    // # Generate a unique planeId: 100 (CMSIT offset) + 10 * hybridId (hybrid index) + chipId (chip index)  #
    // # Don't use these ranges (Needs to be unique to each ROC):                                            #
    // # (-) 0-10:  used by NIConverter/MIMOSA                                                               #
    // # (-) 25-30: used by USBPixGen3Converter/FEI-4                                                        #
    // # (-) 30+:   used by BDAQ53Converter/RD53A with same model (30 [BDAQ offset] + 10 * boardId + chipId) #
    // #######################################################################################################
    int planeId = CMSITplaneIdOffset + hybridId * 10 + chipId;

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

    return planeId;
}

bool                                                       CMSITConverterPlugin::exitIfOutOfSync          = false;
int                                                        CMSITConverterPlugin::theTLUtriggerId_previous = 0;
std::map<std::string, TheConverter::calibrationParameters> CMSITConverterPlugin::calibMap                 = {};
std::shared_ptr<Configuration>                             CMSITConverterPlugin::theConfigFromFile        = nullptr;

namespace
{
auto dummy0 = Factory<StdEventConverter>::Register<CMSITConverterPlugin>(CMSITConverterPlugin::m_id_factory);
}
