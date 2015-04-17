#include "CorrelationPlots.h"
#include "Planes.h"
#include "TH2.h"
#include <string>
#include <iostream>
#include "XMLextractorHelpers.h"
#include "RootFactories.h"
using namespace std;



CorrelationPlot::CorrelationPlot(rapidxml::xml_node<> *Correlation) :CorrelationPlots_interface(Correlation), m_corr(nullptr)
{


}




void CorrelationPlot::createHistogram()
{
  setAxisProberties();
  std::string name = "correlation_" + to_string(m_planeID0) + "_" + m_x_axis.axis_title + "_" + to_string(m_planeID1) + "_" + m_y_axis.axis_title;
  std::string title = "correlation " + to_string(m_planeID0) + " axis " + m_x_axis.axis_title + " VS " + to_string(m_planeID1) + " axis " + m_y_axis.axis_title;
  m_corr = rootFacories::createTH2DfromAxis(name.c_str(), title.c_str(), m_x_axis, m_y_axis);


}

void CorrelationPlot::processEntry()
{
  double x = get0(), y = get1();

  if (cutOffCondition(x, y))
  {
    m_corr->Fill(x, y);
  }

}

CorrelationPlot::~CorrelationPlot()
{


}

void CorrelationPlot::Draw(const char* DrawOptions)
{
  m_corr->Draw("colz");
}







