#ifndef CorrelationPlots_interface_h__
#define CorrelationPlots_interface_h__
#include "rapidxml_utils.hpp"
#include "Axis.h"
#include "Planes.h"

class CorrelationPlots_interface {
public:
  CorrelationPlots_interface(rapidxml::xml_node<> *node);
  virtual ~CorrelationPlots_interface();
  bool registerPlanes(std::vector<plane> &planes);

  virtual void processEntry() = 0;
  void processEvent();
  void setExpectedEventNumber(int ExpectedEvents) {
    m_expected_events = ExpectedEvents;
  }
  virtual void createHistogram() = 0;
  virtual void Draw(const char *DrawOptions = "") = 0;
  bool cutOffCondition(const double &x, const double &y) const;
  void extractCutOffCondition(rapidxml::xml_node<> *cutOffCon);
  int m_planeID0, m_planeID1;
  int event_nr, m_expected_events;
  axis m_axis0, m_axis1;
  axisProberties m_x_axis, m_y_axis;

  plane *m_plane0, *m_plane1;
  double get0();
  double get1();
  void setAxisProberties();

  class condition {
  public:
    condition(double A, double B, double C, double D)
        : m_A(A), m_B(B), m_C(C), m_D(D) {}

    bool isTrue(const double &x, const double &y) const {
      return abs(m_A * x + m_B * y + m_C) < m_D;
    }

  private:
    double m_A, m_B, m_C, m_D;
  };
  std::vector<condition> m_con;
};
#endif // CorrelationPlots_interface_h__
