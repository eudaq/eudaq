/*
 * CorrelationCollection.hh
 *
 *  Created on: Jun 17, 2011
 *      Author: stanitz
 */

#ifndef CORRELATIONCOLLECTION_HH_
#define CORRELATIONCOLLECTION_HH_
//ROOT Includes
#include <RQ_OBJECT.h>
#include <TH2I.h>
#include <TFile.h>

#include <map>
#include <set>
#include <vector>
#include <string>
#include <utility>

#include "SimpleStandardPlaneDouble.hh"
#include "CorrelationHistos.hh"
#include "BaseCollection.hh"


using namespace std;
class RootMonitor;


// currently we do not use SimplePlaneDobule anymore due to the overhead ,,,,
inline bool operator==(SimpleStandardPlaneDouble const &a, SimpleStandardPlaneDouble const &b) {
  if ( a.getPlane1() ==  b.getPlane1() && a.getPlane2() ==  b.getPlane2()) {
    return true;
  } else {
    if ( a.getPlane1() ==  b.getPlane2() && a.getPlane2() ==  b.getPlane1()) {
      return true;
    } else { return false;}
  }
}

inline bool operator<(SimpleStandardPlaneDouble const &a, SimpleStandardPlaneDouble const &b) { // Needed to use struct in a map
  return a.getPlane1()<b.getPlane1() || ( a.getPlane1()==b.getPlane1() && a.getPlane2() < b.getPlane2());
}

struct SortClustersByXY
{
  bool operator()(SimpleStandardCluster const& L, SimpleStandardCluster const& R) const
  {
    if (L.getX() < R.getX())
      return true;
    else if (L.getX() > R.getX())
      return false;
    else
      if (L.getY() < R.getY())
        return true;
      else
        return false;
  }
};

//!Correlation Collection Class
/*!
  This inherits from the BaseCollection class. It is used as part of the online monitor system to show the correlation plots.
 */
class CorrelationCollection : public BaseCollection {
  protected:
    map< SimpleStandardPlaneDouble, CorrelationHistos* > _mapOld;
    map< pair< SimpleStandardPlane, SimpleStandardPlane >, CorrelationHistos* > _map;
    vector< SimpleStandardPlane > _planes;
    bool isPlaneRegistered(SimpleStandardPlane p);
    bool checkCorrelations(const SimpleStandardCluster &cluster1, const SimpleStandardCluster &cluster2, const bool all_mimosa);
    void fillHistograms(vector< vector< pair< int, SimpleStandardCluster > > > tracks, const SimpleStandardEvent & simpEv);
    void fillHistograms(const SimpleStandardPlaneDouble &simpPlaneDouble);
    void fillHistograms(const SimpleStandardPlane& p1, const SimpleStandardPlane& p2);
  public:
    CorrelationCollection();
    void Fill(const SimpleStandardEvent &simpev);
    unsigned int FillWithTracks(const SimpleStandardEvent &simpev);
    virtual void Reset();
    void setRootMonitor(RootMonitor *mon);
    CorrelationHistos * getCorrelationHistos(SimpleStandardPlaneDouble pd);
    CorrelationHistos * getCorrelationHistos(const SimpleStandardPlane &p1, const SimpleStandardPlane &p2);

    void registerPlaneCorrelations(const SimpleStandardPlane &p1, const SimpleStandardPlane &p2);

    virtual void Write(TFile *file);
    bool getCorrelateAllPlanes() const;
    vector<int> getSelected_planes_to_skip() const;
    void setCorrelateAllPlanes(bool correlateAllPlanes);
    void setSelected_planes_to_skip(vector<int> selected_planes_to_skip);
    virtual void Calculate(const unsigned int currentEventNumber)
    {

    }
    void setPlanesNumberForCorrelation(unsigned param)  { planesNumberForCorrelation = param; }
    void setWindowWidthForCorrelation(unsigned param)   { windowWidthForCorrelation = param; }
    unsigned getPlanesNumberForCorrelation()  { return planesNumberForCorrelation; }
    unsigned getWindowWidthForCorrelation()   { return windowWidthForCorrelation; }
  private:
    vector< bool > skip_this_plane; // a array of booleans, initialized with values of selected_planes_to_skip on the first call
    bool correlateAllPlanes;
    vector< int > selected_planes_to_skip;
    unsigned planesNumberForCorrelation;
    unsigned windowWidthForCorrelation;
};

#ifdef __CINT__
#pragma link C++ class CorrelationCollection-;
#endif

#endif /* CORRELATIONCOLLECTION_HH_ */
