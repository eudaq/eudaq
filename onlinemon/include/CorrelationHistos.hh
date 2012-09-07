/*
 * CorrelationHistos.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef CORRELATIONHISTOS_HH_
#define CORRELATIONHISTOS_HH_

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
	TH2I * _2dcorrX;
	TH2I * _2dcorrY;
public:
	CorrelationHistos(SimpleStandardPlane p1, SimpleStandardPlane p2);

	void Fill(const SimpleStandardCluster &cluster1, const SimpleStandardCluster &cluster2);

	void Reset();

	TH2I * getCorrXHisto();
	TH2I * getCorrYHisto();
	int getFills() const ;
	void resetFills();
	void Write();

};
#ifdef __CINT__
#pragma link C++ class CorrelationHistos-;
#endif

#endif /* CORRELATIONHISTOS_HH_ */
