/*!
  \file                  CMSITConverterPlugin.cc
  \brief                 Implementaion of CMSIT Converter Plugin for EUDAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  23/08/21
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include <cmath>

#include "eudaq/CMSITEventData.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "UTIL/CellIDEncoder.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
// Eudaq
#include "EUTelRD53ADetector.h"
#include "EUTelRunHeaderImpl.h"
// Eutelescope
#include "EUTELESCOPE.h"
#include "EUTelEventImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelSetupDescription.h"
#include "EUTelTrackerDataInterfacerImpl.h"
using eutelescope::EUTELESCOPE;
#endif

// ROOT
#include "TCanvas.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TH2D.h"
#include "TKey.h"

// BOOST
#include "boost/archive/binary_iarchive.hpp"
#include "boost/serialization/vector.hpp"

namespace eudaq
{
// ######################
// # Internal constants #
// ######################
static const char*       EVENT_TYPE = "CMSIT";
static const std::string SENSORTYPE = "RD53A";
static const int         NROWS = 192, NCOLS = 400;
static const int         MAXFRAMES = 32;
static const int         MAXHYBRID = 9, MAXCHIPID = 7;
static const double      PITCHX = 0.050, PITCHY = 0.050; // [mm]
static const int         CMSITplaneIdOffset = 100;
static const int         MAXTRIGIDCNT       = 32767;
// #####################################################################
// # Config file name: CFG_FILE_NAME                                   #
// # Layout example:                                                   #
// #                                                                   #
// # [sensor.geometry]                                                 #
// # pitch_hybridId0_chipId0 = “25x100origR0C0”                        #
// # pitch_hybridId1_chipId0 = “25x100origR1C0”                        #
// # pitch_hybridId2_chipId0 = “50x50”                                 #
// #                                                                   #
// # [sensor.calibration]                                              #
// # fileName_hybridId0_chipId0                = “Run000000_Gain.root" #
// # slopeVCal2Electrons_hybridId0_chipId0     = 11.67                 #
// # interceptVCal2Electrons_hybridId0_chipId0 = 64                    #
// #                                                                   #
// # [sensor.selection]                                                #
// # charge_hybridId0_chipId1        = 1500                            #
// # triggerIdLow_hybridId0_chipId1  = -1                              #
// # triggerIdHigh_hybridId0_chipId1 = -1                              #
// #                                                                   #
// # [converter.settings]                                              #
// # exitIfOutOfSync = true                                            #
// #####################################################################
// # To be placed in the Corryvreckan output directory or in the EUDAQ #
// # bin directory                                                     #
// #####################################################################
static const char* CFG_FILE_NAME = "CMSIT.cfg";

// #####################################
// # Row, Column, and Charge converter #
// #####################################
class TheConverter
{
  public:
    struct calibrationParameters
    {
        TFile* calibrationFile;
        TH2D*  hSlope;
        TH2D*  hIntercept;
        float  slopeVCal2Charge;
        float  interceptVCal2Charge;

        calibrationParameters()
        {
            calibrationFile = nullptr;
            hSlope          = nullptr;
            hIntercept      = nullptr;
        }
    };
    void ConverterFor25x100origR1C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
    {
        charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
        row    = (2 * row + 1) - (col % 2);
        col    = std::floor(col / 2.);
        col += (!isSingleChip) * nCols * lane;
    }
    void ConverterFor25x100origR0C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
    {
        charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
        row    = 2 * row + (col % 2);
        col    = std::floor(col / 2.);
        col += (!isSingleChip) * nCols * lane;
    }
    void ConverterFor50x50(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
    {
        charge = ChargeConverter(row, col, charge, calibPar, chargeCut);
        row    = row;
        col += (!isSingleChip) * nCols * lane;
    }
    void operator()(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut)
    {
        (this->*whichConverter)(row, col, charge, lane, calibPar, chargeCut);
    }
    int  nCols;
    bool isSingleChip;
    void (TheConverter::*whichConverter)(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);

  private:
    int ChargeConverter(const int row, const int col, const int ToT, const calibrationParameters& calibPar, const int& chargeCut)
    {
        if((calibPar.hIntercept != nullptr) && (calibPar.hSlope != nullptr) && (calibPar.hSlope->GetBinContent(col + 1, row + 1) != 0))
        {
            auto value = (ToT - calibPar.hIntercept->GetBinContent(col + 1, row + 1)) / calibPar.hSlope->GetBinContent(col + 1, row + 1) * calibPar.slopeVCal2Charge + calibPar.interceptVCal2Charge;
            return (value > chargeCut ? value : -1);
        }

        return (ToT > chargeCut ? ToT : -1);
    }
};

auto GetChipGeometry(const std::string& cfgFromData, const std::string& cfgFromFile, int& nRows, int& nCols, double& pitchX, double& pitchY)
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

TH2D* FindHistogram(const std::string& nameInHisto)
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

class CMSITConverterPlugin : public DataConverterPlugin
{
  public:
    virtual ~CMSITConverterPlugin()
    {
        delete theConfigFromFile;
        delete theTLUtriggerId_previous;
    }

    virtual void Initialize(const Event& bore, const Configuration& cnf)
    {
        theTLUtriggerId_previous = new int(0);
        theConfigFromFile        = nullptr;
        std::ifstream cfgFile(CFG_FILE_NAME);

        if(cfgFile.good() == true)
        {
            std::cout << "[EUDAQ::CMSITConverterPlugin::Initialize] --> Found cfg file: " << CFG_FILE_NAME << std::endl;
            theConfigFromFile = new eudaq::Configuration(cfgFile);

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
                            calibMap[calibration]                 = TheConverter::calibrationParameters();
                            calibMap[calibration].calibrationFile = TFile::Open(calibFileName.c_str());
                            if((calibMap[calibration].calibrationFile != nullptr) && (calibMap[calibration].calibrationFile->IsOpen() == true))
                            {
                                std::cout << "[EUDAQ::CMSITConverterPlugin::Initialize] --> Opening calibration file: " << calibFileName << std::endl;

                                TDirectory* rootDir          = gDirectory;
                                calibMap[calibration].hSlope = FindHistogram("Slope2D");

                                rootDir->cd();
                                calibMap[calibration].hIntercept = FindHistogram("Intercept2D");

                                const std::string slope("slopeVCal2Electrons_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
                                calibMap[calibration].slopeVCal2Charge = theConfigFromFile->Get(slope, 0.);

                                const std::string intercept("interceptVCal2Electrons_hybridId" + std::to_string(hybridId) + "_chipId" + std::to_string(chipId));
                                calibMap[calibration].interceptVCal2Charge = theConfigFromFile->Get(intercept, 0.);
                            }
                            else
                                std::cout << "[EUDAQ::CMSITConverterPlugin::Initialize] --> I couldn't open the calibration file: " << calibFileName << std::endl;
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
    }

    virtual unsigned GetTriggerID(const Event& ev) const
    {
        CMSITEventData::EventData theEvent;
        if(CMSITConverterPlugin::Deserialize(ev, theEvent) == true) return theEvent.tluTriggerId;

        return -1;
    }

    virtual bool GetStandardSubEvent(StandardEvent& sev, const Event& ev) const
    {
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
                const auto planeId = CMSITConverterPlugin::ComputePlaneId(theChip.hybridId, theChip.chipId, chipTypeFromFile, theCalibPar, chargeCut, triggerIdLow, triggerIdHigh);

                if(std::find(planeIDs.begin(), planeIDs.end(), planeId) == planeIDs.end())
                {
                    StandardPlane plane(planeId, EVENT_TYPE, SENSORTYPE);

                    planeIDs.push_back(planeId);

                    int    nRows, nCols;
                    double pitchX, pitchY;
                    auto   theConverter = GetChipGeometry(theChip.chipType, chipTypeFromFile, nRows, nCols, pitchX, pitchY);

                    plane.SetSizeZS(nCols, nRows, 0, MAXFRAMES, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
                    plane.SetTLUEvent(theEvent.tluTriggerId);

                    // #######################################
                    // # Check possible synchronization loss #
                    // #######################################
                    if(theEvent.tluTriggerId != *theTLUtriggerId_previous + 1)
                    {
                        if(exitIfOutOfSync == true)
                        {
                            std::cout << "[EUDAQ::CMSITConverterPlugin::GetStandardSubEvent] WARNING: possible loss of synchronization, current trigger ID " << theEvent.tluTriggerId
                                      << ", previus trigger ID " << *theTLUtriggerId_previous << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    *const_cast<int*>(theTLUtriggerId_previous) = (theEvent.tluTriggerId == MAXTRIGIDCNT ? -1 : theEvent.tluTriggerId);

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
                                    plane.PushPixel(col, row, charge, false, triggerId);
                                }

                            triggerId++;
                        }

                    sev.AddPlane(plane);
                }
                else
                    break;
            }

            return true;
        }

        return false;
    }

#if USE_LCIO && USE_EUTELESCOPE
    virtual lcio::LCEvent* GetLCIOEvent(const Event*) const { return 0; }

    virtual bool GetLCIOSubEvent(lcio::LCEvent& lcioEvent, const Event& ev) const
    {
        if(ev.IsBORE() == true)
            return true;
        else if(ev.IsEORE() == true)
            return true;

        // Set type of the resulting lcio event
        lcioEvent.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE);
        // Pointer to collection which will store data
        LCCollectionVec* zsDataCollection = nullptr;
        // It can be already in event or has to be created
        bool zsDataCollectionExists = false;

        try
        {
            zsDataCollection       = static_cast<LCCollectionVec*>(lcioEvent.getCollection("zsdata_rd53a"));
            zsDataCollectionExists = true;
        }
        catch(lcio::DataNotAvailableException& e)
        {
            zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
        }

        // Create cell encoders to set sensorID and pixel type
        CellIDEncoder<TrackerDataImpl>                   zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
        std::vector<eutelescope::EUTelSetupDescription*> setupDescription;

        CMSITEventData::EventData theEvent;
        if(CMSITConverterPlugin::Deserialize(ev, theEvent) == false) return false;

        std::vector<int> planeIDs;
        for(auto i = 0; i < theEvent.chipData.size(); i++)
        {
            int                   chargeCut;
            int                   triggerIdLow;
            int                   triggerIdHigh;
            calibrationParameters theCalibPar;
            std::string           chipTypeFromFile;
            const auto&           theChip = theEvent.chipData[i];
            const auto            planeId = CMSITConverterPlugin::ComputePlaneId(theChip.hybridId, theChip.chipId, chipTypeFromFile, theCalibPar, chargeCut, triggerIdLow, triggerIdHigh);

            if(std::find(planeIDs.begin(), planeIDs.end(), planeId) == planeIDs.end())
            {
                planeIDs.push_back(planeId);

                int    nRows, nCols;
                double pitchX, pitchY;
                auto   theConverter = GetChipGeometry(theChip.chipType, chipTypeFromFile, nRows, nCols, pitchX, pitchY);

                if(lcioEvent.getEventNumber() == 0)
                {
                    eutelescope::EUTelPixelDetector* currentDetector = new eutelescope::EUTelRD53ADetector(pitchX, pitchY);
                    currentDetector->setMode("ZS");
                    setupDescription.push_back(new eutelescope::EUTelSetupDescription(currentDetector));
                }
                zsDataEncoder["sensorID"]        = planeId;
                zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

                // Prepare a new TrackerData object for the ZS data
                // it contains all the hits for a particular sensor in one event
                std::unique_ptr<lcio::TrackerDataImpl> zsFrame(new lcio::TrackerDataImpl);
                // Set some values of "Cells" for this object
                zsDataEncoder.setCellID(zsFrame.get());

                // This is the structure that will host the sparse pixel
                // it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
                // a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
                std::unique_ptr<eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>> sparseFrame(
                    new eutelescope::EUTelTrackerDataInterfacerImpl<eutelescope::EUTelGenericSparsePixel>(zsFrame.get()));

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
                                sparseFrame->emplace_back(col, row, charge, triggerId);
                            }

                        triggerId++;
                    }

                zsDataCollection->push_back(zsFrame.release());
            }
            else
                break;
        }

        // Add this collection to lcio event
        if((!zsDataCollectionExists) && (zsDataCollection->size() != 0)) lcioEvent.addCollection(zsDataCollection, "zsdata_rd53a");

        if(lcioEvent.getEventNumber() == 0)
        {
            // Do this only in the first event
            LCCollectionVec* apixSetupCollection = nullptr;
            bool             apixSetupExists     = false;
            try
            {
                apixSetupCollection = static_cast<LCCollectionVec*>(lcioEvent.getCollection("rd53a_setup"));
                apixSetupExists     = true;
            }
            catch(...)
            {
                apixSetupCollection = new LCCollectionVec(lcio::LCIO::LCGENERICOBJECT);
            }

            for(auto const& plane: setupDescription) apixSetupCollection->push_back(plane);

            if(!apixSetupExists) lcioEvent.addCollection(apixSetupCollection, "rd53a_setup");
        }

        return true;
    }
#endif

  private:
    CMSITConverterPlugin() : DataConverterPlugin(EVENT_TYPE) {}

    bool Deserialize(const Event& ev, CMSITEventData::EventData& theEvent) const
    {
        // Make sure the event is of class RawDataEvent
        if(const RawDataEvent* rev = dynamic_cast<const RawDataEvent*>(&ev))
        {
            if((rev->NumBlocks() > 0) && (rev->GetBlock(0).size() >= 0))
            {
                std::istringstream              theSerialized(std::string((char*)rev->GetBlock(0).data(), rev->GetBlock(0).size()));
                boost::archive::binary_iarchive theArchive(theSerialized);
                theArchive >> theEvent;

                return true;
            }
        }

        return false;
    }

    int ComputePlaneId(const uint32_t                       hybridId,
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

    bool                                                       exitIfOutOfSync;
    int*                                                       theTLUtriggerId_previous;
    eudaq::Configuration*                                      theConfigFromFile;
    std::map<std::string, TheConverter::calibrationParameters> calibMap;
    static CMSITConverterPlugin                                m_instance;
};

CMSITConverterPlugin CMSITConverterPlugin::m_instance;
} // namespace eudaq
