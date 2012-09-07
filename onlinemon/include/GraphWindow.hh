#ifndef EUDAQ_INCLUDE_GRAPHWINDOW
#define EUDAQ_INCLUDE_GRAPHWINDOW

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGWindow.h>
#include <TCanvas.h>
#include <TRootEmbeddedCanvas.h>
#include <TGStatusBar.h>
#include <TGDoubleSlider.h>
#include <TGraph.h>
#include "TGraphSet.hh"
#include <TLegend.h>



class TGWindow;
class TGMainFrame;
class TRootEmbeddedCanvas;


class GraphWindow : public TGMainFrame {
	protected:
		TGHorizontalFrame *Hfrm_Graph;
		TGHorizontalFrame *Hfrm_Slider;
	
		TRootEmbeddedCanvas *ECvs_Graph;
	
		TGDoubleHSlider *TimeSlider;
		TGDoubleVSlider *GraphSlider;
	
		TGraph *graph;
	
		TGStatusBar *fStatusBar;
		TGraphSet* set;
		TLegend *leg;
	
		float TimeSliderMin, TimeSliderMax;
		float GraphSliderMin, GraphSliderMax;
	
	public:
		GraphWindow(const TGWindow *p, UInt_t w, UInt_t h);
		virtual ~GraphWindow();
		void Update(bool firsttime=false);
		void DoSlider();
		void Selected(Int_t event, Int_t x, Int_t y, TObject* selected);
		TLegend * getLegend() { return leg; }
		
		TCanvas * GetCanvas();
		void SetTGraphSet(TGraphSet *s) { set = s; } 
	
};

#ifndef __CINT__
#pragma link C++ class GraphWindow-;
#endif



#endif