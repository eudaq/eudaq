/*
 * HitmapHistos.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef HITMAPHISTOS_HH_
#define HITMAPHISTOS_HH_

#include <TH2I.h>
#include <TFile.h>

#include <map>

#include "SimpleStandardEvent.hh"



using namespace std;


class RootMonitor;

class HitmapHistos {
  protected:
    string _sensor;
    int _id;
    int _maxX;
    int _maxY;
    bool _wait;
    TH2I * _hitmap;
    TH2I * _clusterMap;
    TH2D * _HotPixelMap;
    TH1I * _lvl1Distr;
    TH1I * _lvl1Width;
    TH1I * _lvl1Cluster;
    TH1I * _totSingle;
    TH1I * _totCluster;
    TH1F * _hitOcc;
    TH1I * _clusterSize;
    TH1I * _nClusters;
    TH1I * _nHits;
    TH1I * _clusterXWidth;
    TH1I * _clusterYWidth;
    TH1I * _nbadHits;
    TH1I * _nHotPixels;
    TH1I * _nPivotPixel;
    TH1I * _hitmapSections;
    TH1I ** _nHits_section;
    TH1I ** _nClusters_section;
    TH1I ** _nClustersize_section;
    TH1I ** _nHotPixels_section;


  public:
    HitmapHistos(SimpleStandardPlane p, RootMonitor * mon);


    void Fill(const SimpleStandardHit & hit);
    void Fill(const SimpleStandardPlane & plane);
    void Fill(const SimpleStandardCluster & cluster);
    void Reset();

    void Calculate(const int currentEventNum);
    void Write();


    TH2I * getHitmapHisto() { return _hitmap; }
    TH1I * getHitmapSectionsHisto() { return _hitmapSections; }
    TH2I * getClusterMapHisto() { return _clusterMap; }
    TH2D * getHotPixelMapHisto() { return _HotPixelMap; }
    TH1I * getLVL1Histo() { return _lvl1Distr; }
    TH1I * getLVL1WidthHisto() { return _lvl1Width; }
    TH1I * getLVL1ClusterHisto() { return _lvl1Cluster; }
    TH1I * getTOTSingleHisto() { return _totSingle; }
    TH1I * getTOTClusterHisto() { return _totCluster; }
    TH1F * getHitOccHisto() { if (_wait) return NULL; else return _hitOcc; }
    TH1I * getClusterSizeHisto() { return _clusterSize; }
    TH1I * getNHitsHisto() { return _nHits; }
    TH1I * getNClustersHisto() { return _nClusters; }
    TH1I * getClusterWidthXHisto() { return _clusterXWidth; }
    TH1I * getClusterWidthYHisto() { return _clusterYWidth; }
    TH1I * getNbadHitsHisto() { return _nbadHits; }
    TH1I * getSectionsNHitsHisto(unsigned int section) { return _nHits_section[section]; }
    TH1I * getSectionsNClusterHisto(unsigned int section) { return _nClusters_section[section]; }
    TH1I * getSectionsNClusterSizeHisto(unsigned int section) { return _nClustersize_section[section]; }
    TH1I * getSectionsNHotPixelsHisto(unsigned int section) { return _nHotPixels_section[section]; }
    TH1I * getNHotPixelsHisto() { return _nHotPixels; }
    TH1I * getNPivotPixelHisto(){ return _nPivotPixel;}
    void setRootMonitor(RootMonitor *mon)  {_mon = mon; }

  private:
    int ** plane_map_array; //store an array representing the map
    int zero_plane_array(); // fill array with zeros;
    int SetHistoAxisLabelx(TH1* histo,string xlabel);
    int SetHistoAxisLabely(TH1* histo,string ylabel);
    int SetHistoAxisLabels(TH1* histo,string xlabel, string ylabel);
    int filling_counter; //we don't need occupancy to be refreshed for every single event

    RootMonitor * _mon;
    unsigned int mimosa26_max_section;
    //check what kind sensor we're dealing with
    // for the filling this eliminates a string comparison
    bool is_MIMOSA26;
    bool is_APIX;
    bool is_USBPIX;
    bool is_USBPIXI4;
    bool is_DEPFET;
};

#ifdef __CINT__
#pragma link C++ class HitmapHistos-;
#endif

#endif /* HITMAPHISTOS_HH_ */

