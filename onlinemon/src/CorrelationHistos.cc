/*
 * CorrelationHistos.cc
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#include "CorrelationHistos.hh"

  CorrelationHistos::CorrelationHistos(SimpleStandardPlane p1, SimpleStandardPlane p2)
: _sensor1(p1.getName()), _sensor2(p2.getName()), _id1(p1.getID()), _id2(p2.getID()),
  _maxX1(p1.getMaxX()), _maxX2(p2.getMaxX()), _maxY1(p1.getMaxY()), _maxY2(p2.getMaxY()), _fills(0),
  _2dcorrX(NULL),_2dcorrY(NULL)
{
  char out[1024], out2[1024], out_x[1024],out_y[1024];

  if (_maxX1 != -1 && _maxX2 != -1) {
    sprintf(out,"X Correlation of %s %i and %s %i",_sensor1.c_str(),_id1, _sensor2.c_str(), _id2);
    sprintf(out2,"h_corr_X_%s_%i_vs_%s_%i",_sensor1.c_str(),_id1, _sensor2.c_str(), _id2);
    sprintf(out_x,"%s %i X",_sensor1.c_str(),_id1);
    sprintf(out_y,"%s %i X",_sensor2.c_str(), _id2);
    _2dcorrX = new TH2I(out2, out, _maxX1,0,_maxX1, _maxX2,0,_maxX2);
    if (_2dcorrX!=NULL)
    {
      _2dcorrX->GetXaxis()->SetTitle(out_x );
      _2dcorrX->GetXaxis()->SetLabelSize(0.03);
      _2dcorrX->GetXaxis()->SetTitleSize(0.03);
      _2dcorrX->GetXaxis()->SetTitleOffset(1);
      _2dcorrX->GetYaxis()->SetTitle(out_y );
      _2dcorrX->GetYaxis()->SetTitleSize(0.03);
      _2dcorrX->GetXaxis()->SetLabelSize(0.03);
      _2dcorrX->GetXaxis()->SetTitleOffset(1);
    }
  }

  if (_maxY1 != -1 && _maxY2 != -1) {
    sprintf(out,"Y Correlation of %s %i and %s %i",_sensor1.c_str(),_id1, _sensor2.c_str(), _id2);
    sprintf(out2,"h_corr_Y_%s_%i_vs_%s_%i",_sensor1.c_str(),_id1, _sensor2.c_str(), _id2);
    sprintf(out_x,"%s %i Y",_sensor1.c_str(),_id1);
    sprintf(out_y,"%s %i Y",_sensor2.c_str(), _id2);
    _2dcorrY = new TH2I(out2, out, _maxY1,0,_maxY1, _maxY2,0,_maxY2);
    if (_2dcorrY!=NULL)
    {
      _2dcorrY->GetXaxis()->SetTitle(out_x );
      _2dcorrY->GetXaxis()->SetLabelSize(0.03);
      _2dcorrY->GetXaxis()->SetTitleSize(0.03);
      _2dcorrY->GetXaxis()->SetTitleOffset(1);
      _2dcorrY->GetYaxis()->SetTitle(out_y );
      _2dcorrY->GetYaxis()->SetTitleSize(0.03);
      _2dcorrY->GetXaxis()->SetLabelSize(0.03);
      _2dcorrY->GetXaxis()->SetTitleOffset(1);		}

  }
}

void CorrelationHistos::Fill(const SimpleStandardCluster &cluster1, const SimpleStandardCluster &cluster2)
{
  //std::cout << "Filling Histogram: " << _2dcorrX->GetName() << " (" << cluster1.getX() << ", " << cluster2.getX() << ")" << std::endl;
  if (_2dcorrX !=NULL) _2dcorrX->Fill(cluster1.getX(),cluster2.getX());
  if (_2dcorrY !=NULL) _2dcorrY->Fill(cluster1.getY(),cluster2.getY());
  _fills++;
}

void CorrelationHistos::Reset()
{
  _2dcorrX->Reset();
  _2dcorrY->Reset();
}

TH2I * CorrelationHistos::getCorrXHisto() { return _2dcorrX; }
TH2I * CorrelationHistos::getCorrYHisto() { return _2dcorrY; }
int CorrelationHistos::getFills() const { return _fills; }
void CorrelationHistos::resetFills() { _fills = 0; }

void CorrelationHistos::Write()
{
  _2dcorrX->Write();
  _2dcorrY->Write();
}

