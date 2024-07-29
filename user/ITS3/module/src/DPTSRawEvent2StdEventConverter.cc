#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <limits>

class DPTSRawEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
private:
  struct PulseTrain {
    float gid,pid;
  };
  struct XY {
    int x,y;
  };
  struct Config {
    int ch;
    float thr[2];
    bool create_tags;
    bool include_bad_trains;
    float time_cut_low[2];
    float time_cut_high[2];
    std::ofstream debug_stream;
    std::vector<std::pair<PulseTrain,XY>> mapping_assert  [2]; // 2 channels are supported
    std::vector<std::pair<PulseTrain,XY>> mapping_deassert[2];
  };
  Config& LoadConf(eudaq::ConfigSPC config_) const;
  const XY PulseTrain2XY(int ich,int slope,const PulseTrain& train,eudaq::ConfigSPC conf_) const;
  std::vector<float> GetEdges(const std::vector<uint8_t> &d,int ich,eudaq::ConfigSPC conf_) const;
  static std::map<eudaq::ConfigSPC,Config> confs;
};

std::map<eudaq::ConfigSPC,DPTSRawEvent2StdEventConverter::Config> DPTSRawEvent2StdEventConverter::confs;

#define REGISTER_CONVERTER(name) namespace{auto dummy##name=eudaq::Factory<eudaq::StdEventConverter>::Register<DPTSRawEvent2StdEventConverter>(eudaq::cstr2hash(#name));}
REGISTER_CONVERTER(DPTS)
REGISTER_CONVERTER(DPTS_0)


DPTSRawEvent2StdEventConverter::Config& DPTSRawEvent2StdEventConverter::LoadConf(eudaq::ConfigSPC conf_) const {
  if(confs.find(conf_)!=confs.end()) return confs[conf_];
  //conf_->Print();
  Config conf;
  conf.ch=-1; // decode all (default fall back, also used for online monitor)
  for(auto i=0;i<2;i++) {
    conf.time_cut_low[i] = -std::numeric_limits<float>::infinity();
    conf.time_cut_high[i] = std::numeric_limits<float>::infinity();
  }
  conf.include_bad_trains=true;
  
  EUDAQ_INFO("Load configuration for DPTS");
  // pass configuration via EUDAQ2 online monitor
  bool mon=conf_?conf_->HasSection("DPTS"):false;
  if(mon){
    conf_->SetSection("DPTS");
    EUDAQ_INFO("Set section `DPTS` for online monitor");
  } else { // look for "identifier set by corry"
    // pass configuration via Corryvreckan
    std::string id=conf_?conf_->Get("identifier",""):""; // set by corry
    if(id!="") {
      size_t p=id.find("DPTS_"); // TODO: no idea why the identifier is quoted in the config, may change in future?
      if(p!=std::string::npos) {
        p+=5;
        conf.ch=std::stoi(id.substr(p));
        EUDAQ_INFO("Set section `"+id+"` from Corryvreckan");
      }
      else {
        conf.ch=-2; // other plane is looked for, bail out early to not waste time in full decoding
        confs[conf_]=std::move(conf);
        return confs[conf_]; // also here: early out: do not try opening the dump file
      }
    }
  }
  
  if(conf_) {
    for(auto i=0;i<2;i++) {
      std::string name;
      name="train_time_cut_low_ch"; name+=std::to_string(i);
      conf.time_cut_low[i] = conf_->Get(name,-std::numeric_limits<float>::infinity());
      //conf_->Print();
      EUDAQ_DEBUG(name+" set to "+std::to_string(conf.time_cut_low[i]));
      name="train_time_cut_high_ch"; name+=std::to_string(i);
      conf.time_cut_high[i] = conf_->Get(name,std::numeric_limits<float>::infinity());
      EUDAQ_DEBUG(name+" set to "+std::to_string(conf.time_cut_high[i]));
    }
  }
  conf.create_tags  =conf_?conf_->Get("create_tags",0):false;
  std::string ofname=conf_?conf_->Get("debug_file",""):"";
  if(ofname!="") conf.debug_stream.open(ofname+std::to_string(conf.ch));
  conf.include_bad_trains=conf_?conf_->Get("include_bad_trains",1):true;
  conf.include_bad_trains?EUDAQ_INFO("Including bad trains as pixel (15,15) (for e.g. efficiency analysis)"):EUDAQ_INFO("Excluding bad trains (for e.g. position resolution analysis)");

  std::string cf=conf_?conf_->Get("calibration_file",""):"";
  if (cf!="") {
    EUDAQ_DEBUG("Found calibration file path: "+cf);
    auto cal=eudaq::Configuration::MakeUniqueReadFile(cf);
    //cal->Print();
    char name[100];
    for (int ich=0;ich<2;++ich) {
      sprintf(name,"threshold_ch%d",ich);
      if (cal->Has(name)) {
        conf.thr[ich]=cal->Get(name,0.0);
      } else {
        EUDAQ_WARN("No threshold for ch" + std::to_string(ich) + ", use default 30.0\n");
        conf.thr[ich]=30.0;
      }
      for(int j=0;j<2;++j) {
        char slope=j==0?'a':'d';
        auto& mapping=j==0?conf.mapping_assert[ich]:conf.mapping_deassert[ich];
        for(int i=0;;++i) {
          sprintf(name,"gidpid2xy_ch%d_%c_%d_gid",ich,slope,i);
          float gid=cal->Get(name,-1.0);
          if(gid<0) break;
          sprintf(name,"gidpid2xy_ch%d_%c_%d_pid",ich,slope,i);
          float pid=cal->Get(name,-1.0);
          sprintf(name,"gidpid2xy_ch%d_%c_%d_x",ich,slope,i);
          int x    =cal->Get(name,-1);
          sprintf(name,"gidpid2xy_ch%d_%c_%d_y",ich,slope,i);
          int y    =cal->Get(name,-1);
          mapping.push_back({{gid,pid},{x,y}});
        }
        // printf("len: %d\n",mapping.size());
      }
      // TODO: move these warnings to the event decoding, here we do not know which (gid,pid)->(x,y) is missing
      if (conf.mapping_assert[ich].size()<1024) {
        EUDAQ_WARN("Assert mapping for ch"  + std::to_string(ich) +  " has too few (gid,pid)->(x,y) points, only: " + std::to_string(conf.mapping_assert[ich].size()) + " (<32x32)\n");
      }
      if (conf.mapping_deassert[ich].size()<1024) {
        EUDAQ_WARN("Deassert mapping for ch"  + std::to_string(ich) +  " has too few (gid,pid)->(x,y) points, only: " + std::to_string(conf.mapping_deassert[ich].size()) + " (<32x32)\n");      
      }
    }
  }
  else {
    for (int ich=0;ich<2;++ich) {
      EUDAQ_WARN("No calibration file for DPTS in ch" + std::to_string(ich) + ". Use default threshold 30.0, associate all the hits to pixel (0,0)\n");      
      conf.thr[ich]=30;
      conf.mapping_assert[ich].push_back({{0,0},{0,0}});
      conf.mapping_deassert[ich].push_back({{0,0},{0,0}});
    }
  }
  confs[conf_]=std::move(conf);
  return confs[conf_];
}

const DPTSRawEvent2StdEventConverter::XY DPTSRawEvent2StdEventConverter::PulseTrain2XY(int ich,int slope,const PulseTrain& train,eudaq::ConfigSPC conf_) const {
  const Config &conf=LoadConf(conf_);
  float vmin=-1;
  int   imin=-1;
  auto& mapping=slope==0?conf.mapping_assert[ich]:conf.mapping_deassert[ich];
  for(size_t i=0;i<mapping.size();++i) {
    float dgid=mapping[i].first.gid-train.gid;
    float dpid=mapping[i].first.pid-train.pid;
    float v=dgid*dgid+dpid*dpid;
    if (vmin<0 || v<vmin) {
      imin=i;
      vmin=v;
    }
  }
  return mapping[imin].second;
}

bool DPTSRawEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf_) const{
  Config &conf=LoadConf(conf_);
  if(conf.ch==-2) return false;
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  std::vector<uint8_t> data=rawev->GetBlock(0); // FIXME: copy?
  size_t n=data.size();
  char name[100];
  for (int ich=0;ich<2;++ich) {
    if(!(conf.ch==-1||conf.ch==rawev->GetDeviceN()+ich)) continue;
    eudaq::StandardPlane plane(rawev->GetDeviceN()+ich,"ITS3DAQ","DPTS");
    plane.SetSizeZS(32,32,0,1); // 0 hits so far + 1 frame
    std::vector<float> e=GetEdges(data,ich,conf_);
    if(conf.create_tags)
      for(int i=0;i<e.size();++i) {
        sprintf(name,"ch%d-edge%d",ich,i);
        out->SetTag(name,e[i]-800); // TODO: configurable offset/histogram range...
      }
    if(conf.debug_stream.is_open()) {
      conf.debug_stream<<ich;
      for(auto t:e) conf.debug_stream<<"\t"<<t;
      conf.debug_stream<<std::endl<<std::flush;
    }
    // TODO: add time for later time cut
    // TODO: do ToT
    std::vector<float> edges;
    std::vector<std::array<float,4>> trains;
    bool bad_trains = false;
    float laste = -std::numeric_limits<float>::infinity();
    e.push_back(std::numeric_limits<float>::infinity());
    for(auto t:e) {
      float dt = t-laste;
      laste=t;
      if(dt>20.) { // min_sep = 20 ns
        if(edges.size()%4==0) {
          for(auto i=0;i<edges.size();i+=4) {
            std::array<float,4> train;
            std::copy(edges.begin()+i,edges.begin()+i+4,train.begin());
            if(conf.time_cut_low[ich]<train.at(0) && train.at(0)<conf.time_cut_high[ich]) {
              trains.push_back(train);
            }
          }
        }
        else if(edges.size()>0 && conf.time_cut_low[ich]<edges.at(0) && edges.at(0)<conf.time_cut_high[ich]) {
          bad_trains = true;
        }
        edges.clear();
      }
      edges.push_back(t);
    }
    if(trains.size()>0) {
      for(auto t:trains) {
        float pidf=t[2]-t[0];
        float gidf=t[3]-t[2];
        XY p=PulseTrain2XY(ich,0,{gidf,pidf},conf_);
        plane.PushPixel(p.x,p.y,1,(uint64_t)0); // column, row, charge, time
      }
    }
    else if(bad_trains && conf.include_bad_trains) {
      plane.PushPixel(15,15,1,(uint64_t)0); // column, row, charge, time
    }
    else if(!conf.include_bad_trains) {
      return false;
    }
    out->AddPlane(plane);
  }
  return true;
}

std::vector<float> DPTSRawEvent2StdEventConverter::GetEdges(const std::vector<uint8_t> &d,int ich,eudaq::ConfigSPC conf_) const {
  const Config &conf=LoadConf(conf_);
  std::vector<float> edges;
  float a=static_cast<int8_t>(d[ich*d.size()/2]);
  for(size_t i=ich*d.size()/2+1;i<(ich+1)*d.size()/2;++i) {
    float b=static_cast<int8_t>(d[i]);
    if((a<conf.thr[ich] && b>=conf.thr[ich]) || (a>=conf.thr[ich] && b<conf.thr[ich]))
      edges.push_back(((i-ich*d.size()/2-1)+(conf.thr[ich]-a)/(b-a))*0.2); // in ns
    a=b; 
  }
  return edges;
}
