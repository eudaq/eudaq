#ifndef Planes_h__
#define Planes_h__

#include "rapidxml.hpp"
#include <string>
#include <vector>
#include "Rtypes.h"
class TH2D;

struct hit {
  hit(int x_pos, double y_pos) : x(x_pos), y(y_pos) {}

  double x, y;
};
inline bool operator==(hit a, hit b) { return (a.x == b.x) && (a.y == b.y); }
hit createHitFromString(const std::string &commaSeparatedString);

struct axisProberties {
  double low, high;
  int bins;
  std::string axis_title;
};

class plane {
public:
  static void extraxtHotPixel(std::vector<hit> &h1, std::vector<hit> &ignor);
  plane(int plane_id);
  plane(rapidxml::xml_node<> *node);
  void setHistogramSize(double XLowerBin, double XUpperBin, int XnumberOfBins,
                        double YLowerBin, double YUpperBin, int YnumberOfBins);
  ~plane();
  bool nextEntry();
  void firstEntry();
  void addCurrentEntry(const int &plane_id, const int &hit_x, const int &hit_y);
  void addCurrentEntry2CalibrationData(const int &plane_id, const int &hit_x,
                                       const int &hit_y);
  void HotPixelsuppression();
  void add2X_IgnorRegin(double beginIgnor, double endIgnor);
  void add2Y_IgnorRegin(double beginIgnor, double endIgnor);
  void setIgnorePercentage(int Percentage);
  void createHistograms();
  void Draw(const char *DrawOptions = "");
  bool ignor(hit x);

  double getX() const;
  double getY() const;
  bool empty();
  void clear();
  axisProberties m_x_axis, m_y_axis;

  double m_Ignore_percentage;
  std::vector<double> ignorX_begin, ignorX_end;
  std::vector<double> ignorY_begin, ignorY_end;
  std::vector<hit> pos;
  std::vector<hit>::iterator current_pos;
  std::vector<hit>::iterator end_pos;
  int m_plane_id, m_hit_x, m_hit_y, m_current_plane_id;

  TH2D *Hitmap;
  std::vector<hit> ev_, ignor_;
  std::vector<Double_t> ignor_x, ignor_y;
};

bool ignorRegin(double h, const std::vector<double> &beginVec,
                const std::vector<double> &endVec);
#endif // Planes_h__
