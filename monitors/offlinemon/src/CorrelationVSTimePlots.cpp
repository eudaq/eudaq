#include "CorrelationVSTimePlots.h"
#include "CorrelationPlots_interface.h"
#include "TH2D.h"
#include <string>
#include <iostream>
#include "XMLextractorHelpers.h"
#include <algorithm>
#include "RootFactories.h"

using namespace std;



void CorrelationVSTimePlots::createHistogram()
{
  setAxisProberties();
  std::string name = "correlation_" + to_string(m_planeID0) + "_" + m_x_axis.axis_title + "_" + to_string(m_planeID1) + "_" + m_y_axis.axis_title + "_time";
  std::string title = "correlation " + to_string(m_planeID0) + " axis " + m_x_axis.axis_title + " - " + to_string(m_planeID1) + " axis " + m_y_axis.axis_title + " VS Time";

  setTimeAxis();
  setCorrelationAxis();
  m_corr = rootFacories::createTH2DfromAxis(name.c_str(), title.c_str(),
    m_time_axis,
    m_axis0_minus_axis1);

}

void CorrelationVSTimePlots::setTimeAxis()
{
  m_time_axis.axis_title = "event no";
  m_time_axis.bins = m_expected_events / 100;
  m_time_axis.low = 0;
  m_time_axis.high = m_expected_events;
}

void CorrelationVSTimePlots::setCorrelationAxis()
{
  m_axis0_minus_axis1.axis_title = "Correlation";
  m_axis0_minus_axis1.bins = max(m_y_axis.bins, m_x_axis.bins);

  // to take all combinations into account without writing to many if statements they get calculated.
  vector<double> combinations;
  combinations.push_back(m_x_axis.low*CorrectionFactorX - m_y_axis.low*CorrectionFactorY - ConstantTerm);
  combinations.push_back(m_x_axis.low*CorrectionFactorX - m_y_axis.high*CorrectionFactorY - ConstantTerm);
  combinations.push_back(m_x_axis.high*CorrectionFactorX - m_y_axis.high*CorrectionFactorY - ConstantTerm);
  combinations.push_back(m_x_axis.high*CorrectionFactorX - m_y_axis.low*CorrectionFactorY - ConstantTerm);
  m_axis0_minus_axis1.low = *min_element(combinations.begin(), combinations.end());
  m_axis0_minus_axis1.high = *max_element(combinations.begin(), combinations.end());


}

void CorrelationVSTimePlots::processEntry()
{
  double x = get0(), y = get1();

  if (cutOffCondition(x, y))
  {
    m_corr->Fill(event_nr, CorrectionFactorX*x - CorrectionFactorY*y - ConstantTerm);
  }

}

CorrelationVSTimePlots::CorrelationVSTimePlots(rapidxml::xml_node<> *node) :CorrelationPlots_interface(node), m_corr(nullptr), CorrectionFactorX(1), CorrectionFactorY(1), ConstantTerm(0)
{
  if (node->first_node("CorrectionFactors"))
  {
    CorrectionFactorX = XMLhelper::getValue(node->first_node("CorrectionFactors")->first_attribute("x"), (double)1);

    CorrectionFactorY = XMLhelper::getValue(node->first_node("CorrectionFactors")->first_attribute("y"), (double)1);
    ConstantTerm = XMLhelper::getValue(node->first_node("CorrectionFactors")->first_attribute("const"), (double)0);
  }

}

CorrelationVSTimePlots::~CorrelationVSTimePlots()
{

}

void CorrelationVSTimePlots::Draw(const char* DrawOptions)
{
  m_corr->Draw(DrawOptions);
}
