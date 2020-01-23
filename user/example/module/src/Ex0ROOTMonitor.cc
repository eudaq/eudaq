#include "eudaq/ROOTMonitor.hh"

#include "TH1.h"
#include "TGraph2D.h"
#include "TProfile.h"

// let there be a user-defined Ex0EventDataFormat, containing e.g. three
// double-precision attributes get'ters:
//   double GetQuantityX(), double GetQuantityY(), and double GetQuantityZ()
struct Ex0EventDataFormat {
  Ex0EventDataFormat(const eudaq::Event&) {}
  double GetQuantityX() const { return (double)rand()/RAND_MAX; }
  double GetQuantityY() const { return (double)rand()/RAND_MAX; }
  double GetQuantityZ() const { return (double)rand()/RAND_MAX; }
};

class Ex0ROOTMonitor : public eudaq::ROOTMonitor {
public:
  Ex0ROOTMonitor(const std::string& name, const std::string& runcontrol):
    eudaq::ROOTMonitor(name, "Ex0 ROOT monitor", runcontrol){}

  void AtConfiguration() override;
  void AtEventReception(eudaq::EventSP ev) override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("Ex0ROOTMonitor");

private:
  TH1D* m_my_hist;
  TGraph2D* m_my_graph;
  TProfile* m_my_prof;
};

namespace{
  auto mon_rootmon = eudaq::Factory<eudaq::Monitor>::
    Register<Ex0ROOTMonitor, const std::string&, const std::string&>(Ex0ROOTMonitor::m_id_factory);
}

void Ex0ROOTMonitor::AtConfiguration(){
  m_my_hist = m_monitor->Book<TH1D>("Channel 0/my_hist", "Example histogram",
    "h_example", "A histogram;x-axis title;y-axis title", 100, 0., 1.);
  m_my_graph = m_monitor->Book<TGraph2D>("Channel 0/my_graph", "Example graph");
  m_my_graph->SetTitle("A graph;x-axis title;y-axis title;z-axis title");
  m_monitor->SetDrawOptions(m_my_graph, "colz");
  m_my_prof = m_monitor->Book<TProfile>("Channel 0/my_profile", "Example profile",
    "p_example", "A profile histogram;x-axis title;y-axis title", 100, 0., 1.);
}

void Ex0ROOTMonitor::AtEventReception(eudaq::EventSP ev){
  auto event = std::make_shared<Ex0EventDataFormat>(*ev);
  m_my_hist->Fill(event->GetQuantityX());
  m_my_graph->SetPoint(m_my_graph->GetN(),
    event->GetQuantityX(), event->GetQuantityY(), event->GetQuantityZ());
  m_my_prof->Fill(event->GetQuantityX(), event->GetQuantityY());
}

