#ifndef SIMPLE_STANDARD_EVENT_H
#define SIMPLE_STANDARD_EVENT_H
#include <string>
#include <vector>
#include <iostream>

#include "include/SimpleStandardPlane.hh"

inline bool operator==(SimpleStandardPlane const &a, SimpleStandardPlane const &b) {
  return (a.getName()==b.getName() && a.getID() == b.getID());
}



inline bool operator<(SimpleStandardPlane const &a, SimpleStandardPlane const &b) { // Needed to use struct in a map
  return a.getName()<b.getName() || ( a.getName()==b.getName() && a.getID() < b.getID()); 
}

class SimpleStandardEvent {
  protected:
    //int _nr;
    std::vector<SimpleStandardPlane> _planes;

  public:
    SimpleStandardEvent();

    void addPlane(SimpleStandardPlane &plane);
    SimpleStandardPlane getPlane (const int i) const {return _planes.at(i);}
    int getNPlanes() const {return _planes.size(); }
    void doClustering();
    double getMonitor_eventanalysistime() const;
    double getMonitor_eventfilltime() const;
    double getMonitor_clusteringtime() const;
    double getMonitor_correlationtime() const;
    void setMonitor_eventanalysistime(double monitor_eventanalysistime);
    void setMonitor_eventfilltime(double monitor_eventfilltime);
    void setMonitor_eventclusteringtime(double monitor_eventclusteringtime);
    void setMonitor_eventcorrelationtime(double monitor_eventcorrelationtime);

    unsigned int getEvent_number() const;
    void setEvent_number(unsigned int event_number);
    unsigned long long int getEvent_timestamp() const;
    void setEvent_timestamp(unsigned long long int event_timestamp);
  private:
    double monitor_eventfilltime; //stores the time to fill the histogram
    double monitor_eventanalysistime;
    double monitor_clusteringtime; //stores the time to fill the histogram
    double monitor_correlationtime;
    unsigned int event_number;
    unsigned long long int event_timestamp;
};






#ifdef __CINT__

#pragma link C++ class SimpleStandardPlane-;

#endif
#endif
