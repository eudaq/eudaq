#ifndef SIMPLE_STANDARD_EVENT_H
#define SIMPLE_STANDARD_EVENT_H

#ifdef __CINT__
#undef __GNUC__
typedef unsigned long long int uint64_t;
typedef char __signed;
typedef char int8_t;
#endif
#include <map>
#include <string>
#include <vector>
#include <iostream>

#ifndef __CINT__
#ifdef WIN32
#define NOMINMAX
#include <windows.h>
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include<cstdint>
#endif
#endif


#include "include/SimpleStandardPlane.hh"

    inline bool
    operator==(SimpleStandardPlane const &a, SimpleStandardPlane const &b) {
  return (a.getName() == b.getName() && a.getID() == b.getID());
}

inline bool
operator<(SimpleStandardPlane const &a,
          SimpleStandardPlane const &b) { // Needed to use struct in a map
  return a.getName() < b.getName() ||
         (a.getName() == b.getName() && a.getID() < b.getID());
}

class SimpleStandardEvent {
protected:
  // int _nr;
  std::vector<SimpleStandardPlane> _planes;

public:
  SimpleStandardEvent();

  void addPlane(SimpleStandardPlane &plane);
  SimpleStandardPlane getPlane(const int i) const { return _planes.at(i); }
  int getNPlanes() const { return _planes.size(); }
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
  uint64_t getEvent_timestamp() const;
  void setEvent_timestamp(uint64_t event_timestamp);
  void setSlow_para(std::string name, double value);
  bool getSlow_para(std::string name, double &value) const;
  std::vector<std::string> getSlowList() const;

private:
  double monitor_eventfilltime; // stores the time to fill the histogram
  double monitor_eventanalysistime;
  double monitor_clusteringtime; // stores the time to fill the histogram
  double monitor_correlationtime;
  unsigned int event_number;
  uint64_t event_timestamp;
  std::map<std::string, double> slowpara;
};

#endif
