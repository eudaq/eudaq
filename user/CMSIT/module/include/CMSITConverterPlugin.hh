/*!
  \file                  CMSITConverterPlugin.hh
  \brief                 Header file of CMSIT Converter Plugin for EUDAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  23/08/21
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include "CMSITEventData.hh"

#include "eudaq/RawEvent.hh"
#include "eudaq/StdEventConverter.hh"

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
static constexpr char    EVENT_TYPE[] = "CMSIT";
static const std::string SENSORTYPE("RD53A");
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

    void ConverterFor25x100origR1C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);
    void ConverterFor25x100origR0C0(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);
    void ConverterFor50x50(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);
    void operator()(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);

    int  nCols;
    bool isSingleChip;
    void (TheConverter::*whichConverter)(int& row, int& col, int& charge, const int& lane, const calibrationParameters& calibPar, const int& chargeCut);

  private:
    int ChargeConverter(const int row, const int col, const int ToT, const calibrationParameters& calibPar, const int& chargeCut);
};

class CMSITConverterPlugin : public StdEventConverter
{
  public:
    bool                  Converting(EventSPC ev, StandardEventSP sev, ConfigurationSPC conf) const override;
    static const uint32_t m_id_factory = cstr2hash(EVENT_TYPE);

  private:
    void         Initialize();
    TheConverter GetChipGeometry(const std::string& cfgFromData, const std::string& cfgFromFile, int& nRows, int& nCols, double& pitchX, double& pitchY) const;
    TH2D*        FindHistogram(const std::string& nameInHisto);
    bool         Deserialize(const EventSPC ev, CMSITEventData::EventData& theEvent) const;
    int          ComputePlaneId(const uint32_t                       hybridId,
                                const uint32_t                       chipId,
                                std::string&                         chipTypeFromFile,
                                TheConverter::calibrationParameters& calibPar,
                                int&                                 chargeCut,
                                int&                                 triggerIdLow,
                                int&                                 triggerIdHigh) const;

    static bool                                                       exitIfOutOfSync;
    static int                                                        theTLUtriggerId_previous;
    static std::shared_ptr<Configuration>                             theConfigFromFile;
    static std::map<std::string, TheConverter::calibrationParameters> calibMap;
};

} // namespace eudaq
