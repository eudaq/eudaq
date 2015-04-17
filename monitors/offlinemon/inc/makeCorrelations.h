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


class mCorrelations{
public:
	mCorrelations();
	~mCorrelations();

void open_confFile(const char * InFileName);
void register_planes(rapidxml::xml_node<> *planesNode);
void register_Correlations(rapidxml::xml_node<> *correlationsNode);
void register_CorrelationsVsTime(rapidxml::xml_node<> *correlationsTimeNode);
void register_planes2Correlation();
void createHistograms();
void open_outFile(const char * outFileName);
void open_outFile();
 void SetFilePattern(const std::string & p) { m_filepattern = p; }
 void setRunNumber(unsigned int RunNum){m_runNumber=RunNum;}

void clear();
bool ProcessDetectorEvent(const eudaq::DetectorEvent &);
void ProcessCurrentEntry();

void AddCurrentEntryToCalibration();
void CalibrateIgnore();
void AddCurrentEntryToHistograms();
void fillCorrelations();


void savePlotsAsPicture();

private:


TFile* OutPutFile;
std::string m_filepattern;
unsigned int m_runNumber;

std::vector<plane> m_planes;
std::vector<CorrelationPlots_interface*> m_corr;

Long64_t currentEvent;
Double_t m_hit_x,m_hit_y;
Int_t m_plane_id,m_event_id,m_CalibrationEvents,NumberOfEvents;
};


inline void helper_ProcessEvent(mCorrelations& writer, const eudaq::Event &ev){ 
  writer.ProcessDetectorEvent(dynamic_cast<const eudaq::DetectorEvent&>(ev)); 
}
inline void helper_StartRun(mCorrelations& writer, unsigned runnumber){
  writer.SetFilePattern("test$6R$X");
  writer.setRunNumber(runnumber);
  writer.open_outFile();
  writer.createHistograms();
}
inline void helper_EndRun(mCorrelations& writer){ 
  writer.savePlotsAsPicture(); 

};
inline void helper_setParameter(mCorrelations &m_processor, const std::string &TagName, const std::string & TagValue){ m_processor.open_confFile(TagValue.c_str()); }
#endif // makeCorrelations_h__
