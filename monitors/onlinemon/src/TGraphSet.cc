#include "TGraphSet.hh"
#include <iostream>
#include <TAxis.h>

TGraphSet::TGraphSet() : _max(0), _xMax(1),_xMin(-1), _yMax(1), _yMin(-1), _xLimitMin(-1), _xLimitMax(1), _yLimitMin(-1), _yLimitMax(1), _nodata(true) , _starttime(0){
  _sizegraph = new TGraph();
  _sizegraph->SetPoint(0,0,0);
  _sizegraph->SetPoint(1,1,1);
}

TGraphSet::~TGraphSet() {
  delete _sizegraph;
  for (int i = 0; i < _graphs.size(); ++i) {
    delete _graphs.at(i);
  }
}

void TGraphSet::AddGraph(const std::string name, const std::string unit) {
  std::map <std::string, int>::iterator it;
  it = _map.find(name);
  if (it == _map.end()) { // Check, that names are unique
    std::cout << "Adding Graph" << std::endl;
    TGraph *graph = new TGraph();
    _graphs.push_back(graph);
    _names.push_back(name);
    _units.push_back(unit);
    _numbers.push_back(0);
    _map[name] = _max;
    graph->SetLineColor(_max%10+1);
    _max++;


  } else {
    //std::cerr << "Names in EnvMon must be unique!" << std::endl;
  }

}

void TGraphSet::AddPoint(const std::string name, const double time, const double value) {
  TGraph *graph = GetTGraph(name);
  graph->SetPoint(GetNumberOfPoints(name), time, value);
  IncreaseNumberOfPoints(name);
  std::cout << "Adding Point " << time << " " << value << std::endl;
  if (_nodata) {
    _yMin = value-0.1; 
    _yMax = value+0.1;
    _xMin = time-0.1;
    _xMax = time+0.1;
    SetLimits(time-0.1, time+0.1, value-0.1, value+0.1);
    _starttime = time;
    _nodata = false;
  } else {
    std::cout << "Min&Max: " << _yMin << " / " << _yMax << std::endl;
    if (value < _yMin) _yMin = value;
    if (value > _yMax) _yMax = value;
    if (time < _xMin) _xMin = time;
    if (time > _xMax) _xMax = time;

  }

}

void TGraphSet::SelectGraphs(const std::vector<std::string> select) {
  _selection.clear();
  for (int i = 0; i < select.size(); ++i) {
    _selection.push_back(GetTGraph(select.at(i)));
  }

}

void TGraphSet::Redraw(TCanvas *canvas) {
  canvas->cd();

  std::string option = "AP";
  /*_sizegraph->SetPoint(0,_xMin,_yMin);
    _sizegraph->SetPoint(1,_xMax,_yMax);
   */
  _sizegraph->SetPoint(0,_xLimitMin,_yLimitMin);
  _sizegraph->SetPoint(1,_xLimitMax,_yLimitMax);

  _sizegraph->GetXaxis()->SetTimeDisplay(1);
  _sizegraph->GetXaxis()->SetTimeFormat("%H %M %S");
  _sizegraph->GetXaxis()->SetTimeOffset(0,"local");
  //_sizegraph->GetXaxis()->SetRangeUser(




  _sizegraph->Draw("AP");
  option = "SAME";
  if (_selection.size() != 0) {
    for (int i = 0; i < _selection.size(); ++i) {
      _selection.at(i)->Draw(option.c_str());

      std::cout << "HERE2" << std::endl;
    }
  } else {
    for (int i = 0; i < _graphs.size(); ++i) {
      _graphs.at(i)->Draw(option.c_str());

      std::cout << "HERE" << std::endl;
    }
  }
  canvas->Update();

}

void TGraphSet::Update(TCanvas *canvas) {
  canvas->cd();
  _sizegraph->Draw("AP");
  canvas->Update();
}

void TGraphSet::SetLimits(const int x1, const int x2, const int y1, const int y2) {
  _sizegraph->SetPoint(0,x1,y1);
  _sizegraph->SetPoint(1,x2,y2);
  _sizegraph->SetMarkerSize(0);
  //_sizegraph->SetLineStyle(0);
}

void TGraphSet::SetLimitsFromSlider(const int x1, const int x2, const int y1, const int y2) {
  //_sizegraph->GetXaxis()->SetRangeUser(x1+_starttime, x2+_starttime);
  //SetLimits(x1+_starttime,x2+_starttime,y1,y2);
  _xMin = x1 + _starttime;
  _xMax = x2 + _starttime;
  _yMin = y1;
  _yMax = y2;

}

void TGraphSet::GetXLimits(double &min, double &max) {
  min = _xMin - _starttime; // otherwise the numbers are too big!
  max = _xMax - _starttime;
}

void TGraphSet::GetYLimits(double &min, double &max) {
  min = _yMin;
  max = _yMax;
}

TGraph *TGraphSet::GetTGraph(std::string name) {
  return _graphs.at(_map[name]);
}

std::string TGraphSet::GetUnit(std::string name) {
  return _units.at(_map[name]);
}

int TGraphSet::GetNumberOfPoints(std::string name) {
  return _numbers.at(_map[name]);
}

void TGraphSet::IncreaseNumberOfPoints(std::string name) {
  _numbers.at(_map[name])++;
}

