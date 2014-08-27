
#ifndef CMSPIXELEVTMONITOR_HH
#define CMSPIXELEVTMONITOR_HH

#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>
#include <TCanvas.h>
#include "TApplication.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TVirtualX.h"
#include "TString.h"
#include "TGWindow.h"
#include "TGFrame.h"
#include "TGraph.h"
#include <TRootEmbeddedCanvas.h>

#include "api.h"

using namespace pxar; 

//#include <istream>
#include <fstream>
#include <iostream>
#include <sstream> 
#include <string> 
#include <vector>


class MyMainFrame;

class CMSPixelEvtMonitor {
	public:
		static CMSPixelEvtMonitor* Instance()
		{
			if(!m_instance)
				m_instance = new CMSPixelEvtMonitor();
			return m_instance;
		}

		void DrawFromFile(std::string filename);

		void DrawMap(std::vector<pxar::pixel> pixels, char* name);

    void TrackROTiming(unsigned int n_ev, double t);

    void DrawROTiming();

	private:

		TApplication* m_theApp;
		TProfile2D* m_h2_map;
		TCanvas* m_canv;
    TCanvas* m_c_timing;
		MyMainFrame* m_MF;
    TGraph* m_g_ROTiming;
		static CMSPixelEvtMonitor* m_instance;

		CMSPixelEvtMonitor();
		CMSPixelEvtMonitor( const CMSPixelEvtMonitor&);
		~CMSPixelEvtMonitor(){}
};



class MyMainFrame:public TGMainFrame {

	private:
		TGMainFrame * fMain;
		TRootEmbeddedCanvas *fEcanvas;
	public:
		MyMainFrame( const TGWindow * p, UInt_t w, UInt_t h );
		virtual ~ MyMainFrame(  );
		TCanvas *GetCanvas(  );
		void HandleEmbeddedCanvas(Int_t event, Int_t x, Int_t y, TObject *sel);
};

#endif
