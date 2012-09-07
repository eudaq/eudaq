#ifndef EUDAQ_INCLUDE_TGRAPHSET
#define EUDAQ_INCLUDE_TGRAPHSET

#include <TGraph.h>
#include <string>
#include <vector>
#include <TCanvas.h>
#include <map>

class TGraphSet {
protected:
	TGraph* _sizegraph;
	std::vector <TGraph*> _graphs;
	std::vector <std::string> _names;
	std::vector <std::string> _units;
	std::vector <TGraph*> _selection;
	std::vector <int> _numbers;
	std::map <std::string, int> _map;
	int _max;
	double _xMax;
	double _xMin;
	double _yMax;
	double _yMin;

	double _xLimitMax;
	double _xLimitMin;
	double _yLimitMax;
	double _yLimitMin;
	

	float _starttime;
	bool _nodata;
	TGraph *GetTGraph(std::string name);
	std::string GetUnit(std::string name);
	int GetNumberOfPoints(std::string name);
	void IncreaseNumberOfPoints(std::string name);
public:
	TGraphSet();
	virtual ~TGraphSet();

	void AddGraph(const std::string name, const std::string unit);
	void Update(TCanvas *canvas);
	void Redraw(TCanvas *canvas);
	void AddPoint(const std::string name, const double time, const double value);
	void SelectGraphs(const std::vector<std::string> select);
	void SetLimits(const int x1, const int x2, const int y1, const int y2);
	void SetLimitsFromSlider(const int x1, const int x2, const int y1, const int y2);
	void GetXLimits(double &min, double &max);
	void GetYLimits(double &min, double &max);
	void SetZoom(const int x1,const int x2,const int y1,const int y2) { _xLimitMin = x1+_starttime; _xLimitMax = x2+_starttime; _yLimitMin = y1; _yLimitMax = y2; }
	TGraph * GetGraph(const std::string name) { return _graphs.at(_map[name]); }
	
};

#ifdef __CINT__
#pragma link C++ class TGraphSet-;
#endif


#endif