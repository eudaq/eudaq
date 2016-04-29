/*
 * ParaMonitorHistos.cc
 *
 *  Created on: Jul 5, 2011
 *      Author: stanitz
 */

#include "ParaMonitorHistos.hh"

#include <iostream>

ParaMonitorHistos::ParaMonitorHistos() {
  m_TempHisto =
      new TH1I("Temperature", "Temperature", 400, 0, 0.01);
  if ((m_TempHisto == NULL) ) {
    std::cout << "ParaMonitorHistos:: Error allocating Histograms"
              << std::endl;
    exit(-1);
  }
}

ParaMonitorHistos::~ParaMonitorHistos() {
  // TODO Auto-generated destructor stub
}

void ParaMonitorHistos::Write() {
  m_TempHisto->Write();  
}

void ParaMonitorHistos::Fill(SimpleStandardEvent ev) {
  double value = 0;
  std::string name("TEMPERATURE");
  if(ev.getSlow_para(name, value)){
    m_TempHisto->Fill(ev.getEvent_timestamp(),value);
  }
}

void ParaMonitorHistos::Reset() {
  m_TempHisto->Reset();
}
