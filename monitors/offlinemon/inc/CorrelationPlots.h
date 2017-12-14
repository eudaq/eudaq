#ifndef CorrelationPlots_h__
#define CorrelationPlots_h__
#include "rapidxml.hpp"
#include <string>
#include <vector>
#include "Planes.h"
#include "Axis.h"
#include "CorrelationPlots_interface.h"
class plane;
class TH2D;

class CorrelationPlot : public CorrelationPlots_interface {
public:
  CorrelationPlot(rapidxml::xml_node<> *node);
  ~CorrelationPlot();
  virtual void Draw(const char *DrawOptions = "");
  virtual void createHistogram();
  virtual void processEntry();

private:
  TH2D *m_corr;
};

#endif // CorrelationPlots_h__
