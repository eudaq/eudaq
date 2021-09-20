#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <vector>
#include <cmath>

class DPTSRawEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
private:
  std::vector<float> GetEdges(const std::vector<uint8_t> &d,size_t i0,size_t i1,float thr) const;
  void Dump();
};

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<DPTSRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(DPTS)

bool DPTSRawEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  std::vector<uint8_t> data=rawev->GetBlock(0); // FIXME: copy?
  size_t n=data.size();
  for (int i=0;i<2;++i) {
    char name[100];
    sprintf(name,"DPTS-ch%c",'D'+i);
    eudaq::StandardPlane plane(rawev->GetDeviceN()+i,"ITS3DAQ","DPTS");
    plane.SetSizeZS(32,32,0,1); // 0 hits so far + 1 frame
    std::vector<float> e=GetEdges(data,data.size()/2*i,data.size()/2*(i+1),30.);
    if (e.size()>=4) {
      float pidf=e[2]-e[0];
      float gidf=e[3]-e[2];
      printf("PID: %5.2f, GID: %5.2f\n",pidf,gidf);
      int pid=std::round((pidf-5)/(11-5)*32);
      int gid=std::round((gidf-1)/( 6-1)*32);
      if(0<=pid&&pid<=31&&0<=gid&&gid<=31) {
        int x=gid<16?2*gid:31-2*(gid-16);
        x=gid;
        int y=x%4<2?31-pid:pid;
        printf("(%d,%d) -> (%d,%d)\n",pid,gid,x,y);
        plane.PushPixel(y,x,1,(uint64_t)0); // row, column, charge, time
      }
      else {
        plane.PushPixel(16,16,1,(uint64_t)0); // row, column, charge, time
      }
    }
    out->AddPlane(plane);
  }
  return true;
}

std::vector<float> DPTSRawEvent2StdEventConverter::GetEdges(const std::vector<uint8_t> &d,size_t i0,size_t i1,float thr) const {
  std::vector<float> edges;
  float a=static_cast<int8_t>(i0);
  for(size_t i=i0+1;i<i1;++i) {
    float b=static_cast<int8_t>(d[i]);
    if((a<thr && b>thr) || (a>thr && b<thr))
      edges.push_back(((i-i0-1)+(thr-a)/(b-a))*0.2); // in ns
    a=b; 
  }
  return edges;
}
