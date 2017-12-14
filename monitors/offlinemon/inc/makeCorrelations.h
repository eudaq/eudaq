#ifndef makeCorrelations_h__
#define makeCorrelations_h__
#include <vector>
#include "Rtypes.h"
#include "eudaq/DetectorEvent.hh"
#include "rapidxml_utils.hpp"
#include "CorrelationPlots_interface.h"

class TFile;
class TTree;
class plane;
class TH2D;
class TH1D;
class CorrelationPlot;
class CorrelationVSTimePlots;

class mCorrelations {
public:
  mCorrelations();
  ~mCorrelations();

  void open_confFile(const char *InFileName);
  void register_planes(rapidxml::xml_node<> *planesNode);
  void register_Correlations(rapidxml::xml_node<> *correlationsNode);
  void register_CorrelationsVsTime(rapidxml::xml_node<> *correlationsTimeNode);
  void register_planes2Correlation();
  void createHistograms();
  void open_outFile(const char *outFileName);
  void open_outFile();
  void SetFilePattern(const std::string &p) { m_filepattern = p; }
  void setRunNumber(unsigned int RunNum) { m_runNumber = RunNum; }

  void clear();
  bool ProcessDetectorEvent(const eudaq::DetectorEvent &);
  void ProcessCurrentEntry();

  void AddCurrentEntryToCalibration();
  void CalibrateIgnore();
  void AddCurrentEntryToHistograms();
  void fillCorrelations();

  void savePlotsAsPicture();

private:
  TFile *OutPutFile;
  std::string m_filepattern;
  unsigned int m_runNumber;

  std::vector<plane> m_planes;
  std::vector<CorrelationPlots_interface *> m_corr;

  Long64_t currentEvent;
  Double_t m_hit_x, m_hit_y;
  Int_t m_plane_id, m_event_id, m_CalibrationEvents, NumberOfEvents;
};

#endif // makeCorrelations_h__
