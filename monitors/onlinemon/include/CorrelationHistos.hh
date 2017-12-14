/*
 * CorrelationHistos.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef CORRELATIONHISTOS_HH_
#define CORRELATIONHISTOS_HH_

#include <mutex>

#include <TH2I.h>
#include <TFile.h>

#include "SimpleStandardEvent.hh"

using namespace std;

class CorrelationHistos {
protected:
  std::string _sensor1;
  std::string _sensor2;
  int _id1;
  int _id2;
  int _maxX1;
  int _maxX2;
  int _maxY1;
  int _maxY2;
  int _fills;
  TH2I *_2dcorrX;
  TH2I *_2dcorrY;
  TH2I *_2dcorrXY;
  TH2I *_2dcorrYX;

  TH2I *_2dcorrTimeX;
  TH2I *_2dcorrTimeY;

  double m_pitchX1;
  double m_pitchY1;
  double m_pitchX2;
  double m_pitchY2;
  
  std::mutex m_mu;
  
public:
  CorrelationHistos(SimpleStandardPlane p1, SimpleStandardPlane p2);

  void Fill(const SimpleStandardCluster &cluster1,
            const SimpleStandardCluster &cluster2);
  void FillCorrVsTime(const SimpleStandardCluster &cluster1,
		      const SimpleStandardCluster &cluster2,
		      const SimpleStandardEvent &simpev);

  
  void Reset();

  TH2I *getCorrXHisto();
  TH2I *getCorrYHisto();
  TH2I *getCorrXYHisto();
  TH2I *getCorrYXHisto();
  TH2I *getCorrTimeXHisto(){return _2dcorrTimeX;};
  TH2I *getCorrTimeYHisto(){return _2dcorrTimeY;};
  std::mutex* getMutex();
  
  int getFills() const;
  void resetFills();
  void Write();
};
#ifdef __CINT__
#pragma link C++ class CorrelationHistos - ;
#endif

#endif /* CORRELATIONHISTOS_HH_ */
