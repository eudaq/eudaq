#include "GraphWindow.hh"
#include <iostream>

using namespace std;

GraphWindow::GraphWindow(const TGWindow *p, UInt_t w, UInt_t h) : TGMainFrame(p,w,h), TimeSliderMin(-1), TimeSliderMax(1), GraphSliderMin(-1), GraphSliderMax(1) {
	cout << "Creating new GraphWindow" << endl;
	
	
	Hfrm_Graph = new TGHorizontalFrame(this);
		GraphSlider = new TGDoubleVSlider(Hfrm_Graph,1,1,-1, kVerticalFrame,GetDefaultFrameBackground(),kTRUE, kFALSE);
		GraphSlider->Connect("PositionChanged()","GraphWindow",this,"DoSlider()");
		GraphSlider->SetRange(-1,1);
		GraphSlider->SetPosition(-1,1);
		Hfrm_Graph->AddFrame(GraphSlider, new TGLayoutHints(kLHintsLeft | kLHintsExpandY,2,2,2,2));
		
		ECvs_Graph = new TRootEmbeddedCanvas("Values", Hfrm_Graph);
		(TCanvas*)ECvs_Graph->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)","GraphWindow",this,"Selected(Int_t,Int_t,Int_t,TObject*)");
		Hfrm_Graph->AddFrame(ECvs_Graph, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,2,5,5,2));
		
	
	Hfrm_Slider = new TGHorizontalFrame(this);
		TimeSlider = new TGDoubleHSlider(Hfrm_Slider);
		TimeSlider->SetRange(-1,1);
		TimeSlider->SetPosition(-1,1);
		TimeSlider->Connect("PositionChanged()", "GraphWindow",this,"DoSlider()");
		Hfrm_Slider->AddFrame(TimeSlider, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY,5,5,2,2));
	
	Int_t parts[] = {33,33,34};
	fStatusBar = new TGStatusBar(this,50,10,kHorizontalFrame);
	fStatusBar->SetParts(parts,3);
	
	AddFrame(Hfrm_Graph, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5,5,5,2));
	AddFrame(Hfrm_Slider, new TGLayoutHints(kLHintsExpandX,25,5,5,5));
	AddFrame(fStatusBar, new TGLayoutHints(kLHintsBottom | kLHintsExpandX,0,0,2,0));
	
	leg = new TLegend(0.1,0.7,0.3,0.9);
	SetWindowName("Environment Graphs");
	MapSubwindows();
	Resize(500,300);
	MapWindow();
	Move(100,100);
	MapWindow();
	
	TCanvas *canv = ECvs_Graph->GetCanvas();
	canv->cd();
	canv->Update();
	
	//Update(true);
	

}

GraphWindow::~GraphWindow() {
	delete Hfrm_Graph;
	delete Hfrm_Slider;
	delete GraphSlider;
	delete ECvs_Graph;
	delete TimeSlider;
	delete fStatusBar;
	
}

void GraphWindow::Update(bool firsttime) {
	double min, max;
	float oldminTime, oldmaxTime, oldminGraph, oldmaxGraph;
	set->GetXLimits(min,max);
	TimeSlider->SetRange(min, max);
	TimeSlider->GetPosition(oldminTime, oldmaxTime);
	//although the range itself changes the distance between the lower position and the left range should be fix. right side analogue
	
	
	TimeSlider->SetPosition(oldminTime-TimeSliderMin+min,oldmaxTime-TimeSliderMax+max);
	TimeSliderMin = min;
	TimeSliderMax  = max;
	
	set->GetYLimits(min, max);
	GraphSlider->GetPosition(oldminGraph, oldmaxGraph);
	GraphSlider->SetRange(min,max);
	
	//here it is the same //somethig is strange in this line.... hmn
	GraphSlider->SetPosition(oldminGraph-GraphSliderMin+min,oldmaxGraph - GraphSliderMax+max);
	std::cout << "Pos: " << oldminGraph << " / " << GraphSliderMin << " / " << min << std::endl;
	//std::cout << "Setting to: " << oldminGraph-GraphSliderMin+min << " / " << oldmaxGraph - GraphSliderMax+max << std::endl;
	GraphSliderMin = min;
	GraphSliderMax = max;
	TimeSlider->GetPosition(oldminTime, oldmaxTime);
	GraphSlider->GetPosition(oldminGraph, oldmaxGraph);
	std::cout << "GraphSlider: " << oldminGraph << " / " << oldmaxGraph << std::endl;
	
	set->SetZoom(oldminTime, oldmaxTime,oldminGraph, oldmaxGraph);
	//set->SetLimitsFromSlider(oldminTime,oldmaxTime,oldminGraph,oldmaxGraph);

	TCanvas *fCanvas = ECvs_Graph->GetCanvas();
	fCanvas->cd();
	//graph->Draw("AC*");
	fCanvas->Update();
}

void GraphWindow::DoSlider() {
cout << "Doing TimeSlider" << endl;
	float x1,x2,y1,y2;
	TimeSlider->GetPosition(x1,x2);
	GraphSlider->GetPosition(y1,y2);

	set->SetLimitsFromSlider(x1,x2,y1,y2);
	set->Redraw(ECvs_Graph->GetCanvas());
}

void GraphWindow::Selected(Int_t event, Int_t x, Int_t y, TObject* selected) {
	cout << "Selected something" << endl;
}



TCanvas *GraphWindow::GetCanvas() {
	cout << "Getting Canvas at " << ECvs_Graph->GetCanvas() << endl;
	return ECvs_Graph->GetCanvas();
}
