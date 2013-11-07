/*
 * SimpleStandardPlane.hh
 *
 *  Created on: Jun 9, 2011
 *      Author: stanitz
 */

#ifndef SIMPLESTANDARDPLANE_HH_
#define SIMPLESTANDARDPLANE_HH_

#include <string>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>

#include <stdio.h>
#include <string.h>

#include "include/SimpleStandardHit.hh"
#include "include/SimpleStandardCluster.hh"
#include "include/OnlineMonConfiguration.hh"

//!Simple Standard Plane Class
/*!

 */
class SimpleStandardPlane {
  protected:
    std::string _name;
    int _id;
    int _maxX;
    int _maxY;
    int _binsX;
    int _binsY;
    std::vector<SimpleStandardHit> _hits;
    std::vector<SimpleStandardHit> _badhits; //stores hits which appear to be corrupted
    std::vector<SimpleStandardHit> _rawhits; //stores hits without a threshold in case of analog pixels
    std::vector<SimpleStandardCluster> _clusters;
    std::vector<SimpleStandardHit> _section_hits[4];//FIXME hard-coded for Mimosa
    std::vector<SimpleStandardCluster> _section_clusters[4];//FIXME hard-coded for Mimosa
    int _tlu_event;
    int _pivot_pixel;
  public:
    SimpleStandardPlane(const std::string & name, const int id, const int maxX, const int maxY, const int tlu_event, const int pivot_pixel, OnlineMonConfiguration* mymon);
    SimpleStandardPlane(const std::string & name, const int id);
    void addHit(SimpleStandardHit oneHit);
    void addRawHit(SimpleStandardHit oneHit);
    void doClustering();
    std::vector<SimpleStandardHit> getHits() const { return _hits; }
    std::vector<SimpleStandardHit> getRawHits() const { return _rawhits; }
    std::vector<SimpleStandardCluster> getClusters() const { return _clusters; }
    int getNHits() const { return _hits.size(); }
    int getNBadHits() const { return _badhits.size(); }
    int getNSectionHits(unsigned int section) const { return _section_hits[section].size(); }
    int getNClusters() const { return _clusters.size(); }
    int getNSectionClusters(unsigned int section) const { return _section_clusters[section].size(); }
    SimpleStandardCluster getCluster(const int i ) const { return _clusters.at(i); }
    SimpleStandardHit getHit(const int i) const { return _hits.at(i); }
    SimpleStandardHit getRawHit(const int i) const { return _rawhits.at(i); }
    std::string getName() const {return _name; }
    int getID() const {return _id;}
    int getMaxX() { return _maxX; }
    int getMaxY() { return _maxY; }
    int getBinsX() {return _binsX;}
    int getBinsY() {return _binsY;}
    int getPivotPixel() const { return _pivot_pixel;}
    int getTLUEvent()const { return _tlu_event;}
    void addSuffix( const std::string & suf ) { _name = _name + suf; }
    void reducePixels(const int reduceX, const int reduceY);
    void setMonitorConfiguration(OnlineMonConfiguration *mymon)
    {
      mon=mymon;
    }
    void setAnalogPixelType(bool type) {AnalogPixelType=type;}
    bool getAnalogPixelType() {return AnalogPixelType;}
    void setIsRotated(bool type) {isRotated=type;}
    bool getIsRotated() {return isRotated;}
    void setPixelType(string name);
    bool is_MIMOSA26;
    bool is_DEPFET;
    bool is_APIX;
    bool is_USBPIX;
    bool is_USBPIXI4;
    bool is_FORTIS;
    bool is_EXPLORER;
    bool is_UNKNOWN;

  private:
    OnlineMonConfiguration* mon;
    bool AnalogPixelType; // dealing with pixels, that have analog information
    bool isRotated; //dealing with planes rotated by 90 degrees

};

struct SortHitsByXY
{
  bool operator()(SimpleStandardHit const& L, SimpleStandardHit const& R) const
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


#endif //ifndef SIMPLESTANDARDPLANE_HH_
