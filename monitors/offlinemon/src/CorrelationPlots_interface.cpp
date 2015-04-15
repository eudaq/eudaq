#include "CorrelationPlots_interface.h"
#include "Planes.h"
#include "TH2.h"
#include <string>
#include <iostream>
#include "XMLextractorHelpers.h"
using namespace std;


CorrelationPlots_interface::CorrelationPlots_interface(rapidxml::xml_node<> *Correlation) :event_nr(0), m_expected_events(0)
{
  //auto Correlation=node->first_node("Correlation");
  m_planeID0 = std::stoi(Correlation->first_node("CorrX")->first_attribute("planeId")->value());
  m_planeID1 = std::stoi(Correlation->first_node("CorrY")->first_attribute("planeId")->value());
  auto axis0 = Correlation->first_node("CorrX")->first_attribute("plane_axis")->value();
  if (!strcmp(axis0, "x"))
  {
    m_axis0 = x_axis;
  }
  else if (!strcmp(axis0, "y"))
  {
    m_axis0 = y_axis;
  }
  else if (!strcmp(axis0, "z"))
  {
    m_axis0 = z_axis;
  }

  auto axis1 = Correlation->first_node("CorrY")->first_attribute("plane_axis")->value();
  if (!strcmp(axis1, "x"))
  {
    m_axis1 = x_axis;
  }
  else if (!strcmp(axis1, "y"))
  {
    m_axis1 = y_axis;
  }
  else if (!strcmp(axis1, "z"))
  {
    m_axis1 = z_axis;
  }

  extractCutOffCondition(Correlation->first_node("cutOffCondition"));

}

bool CorrelationPlots_interface::registerPlanes(std::vector<plane>& planes)
{
  bool register0(0), register1(0);
  for (auto& p : planes)
  {
    if (p.m_plane_id == m_planeID0)
    {
      m_plane0 = &p;
      register0 = true;
    }
    if (p.m_plane_id == m_planeID1)
    {
      m_plane1 = &p;
      register1 = true;
    }
  }
  return register0&&register1;
}




void CorrelationPlots_interface::processEvent()
{
  ++event_nr;
  if (!m_plane0->empty() && !m_plane1->empty())
  {


    m_plane0->firstEntry();
    m_plane1->firstEntry();
    do
    {
      do
      {
        processEntry();


      } while (m_plane1->nextEntry());
      m_plane1->firstEntry();
    } while (m_plane0->nextEntry());
  }
}

bool CorrelationPlots_interface::cutOffCondition(const double& x, const  double& y) const
{
  if (m_con.empty())
  {
    return true;
  }
  for (auto& p : m_con)
  {
    if (!p.isTrue(x, y))
    {
      return false;
    }
  }
  return true;
}

void CorrelationPlots_interface::extractCutOffCondition(rapidxml::xml_node<> *cutOffCon)
{

  m_con = XMLhelper::extractCutOffCondition1<condition>(cutOffCon);
  if (cutOffCon)
  {
    double A, B, C, D;
    auto node = cutOffCon->first_node("condition");


    while (node)
    {
      A = std::stod(node->first_attribute("A")->value());
      B = std::stod(node->first_attribute("B")->value());
      C = std::stod(node->first_attribute("C")->value());
      D = std::stod(node->first_attribute("D")->value());
      m_con.emplace_back(A, B, C, D);
      node = node->next_sibling("condition");
    }
  }
}

CorrelationPlots_interface::~CorrelationPlots_interface()
{

}

double CorrelationPlots_interface::get0()
{

  if (m_axis0 == x_axis)
  {
    return m_plane0->getX();
  }
  else if (m_axis0 == y_axis)
  {
    return m_plane0->getY();
  }
  return -1;
}

double CorrelationPlots_interface::get1()
{
  if (m_axis1 == x_axis)
  {
    return m_plane1->getX();
  }
  else if (m_axis1 == y_axis)
  {
    return m_plane1->getY();
  }
  return -1;
}

void CorrelationPlots_interface::setAxisProberties()
{

  if (m_axis0 == x_axis)
  {
    m_x_axis = m_plane0->m_x_axis;

  }
  else if (m_axis0 == y_axis)
  {
    m_x_axis = m_plane0->m_y_axis;

  }

  if (m_axis1 == x_axis)
  {
    m_y_axis = m_plane1->m_x_axis;

  }
  else if (m_axis1 == y_axis)
  {
    m_y_axis = m_plane1->m_y_axis;

  }
}



