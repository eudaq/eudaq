
#include "eudaq/Monitor.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include "TROOT.h"
#include "TNamed.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TGMenu.h"
#include "TGTab.h"
#include "TGButton.h"
#include "TGComboBox.h"
#include "TGLabel.h"
#include "TGTextEntry.h"
#include "TGNumberEntry.h"
#include "TGComboBox.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TRootEmbeddedCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TPaletteAxis.h"
#include "TThread.h"
#include "TFile.h"
#include "TColor.h"
#include "TString.h"
#include "TF1.h"
//#include "TSystem.h" // for TProcessEventTimer
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <TSystem.h>
#include <TInterpreter.h>
#include <TQObject.h>
#include <RQ_OBJECT.h>
//

static const unsigned MAX_BOARDS = 6;
static const unsigned MAX_SEEDS = 1000;
static const double DEFAULT_NOISE = 4;

//displayed board colors
static const float COL_R[] = { .8,  0,  0, .9, .8,  0 };
static const float COL_G[] = {  0, .7,  0,  0, .8, .8 };
static const float COL_B[] = {  0,  0, .9, .9,  0, .8 };
static const int COL_BASE = 10000;


class histopad // this class represents a pad of histograms.
{
public:
  histopad(TGCheckButton *check) // assign a checkbutton to this pad
    {
      SetVars();
      checkboxbutton = check;
      Disable();
    }
  ~histopad(){ }
  
  void AddHisto(TH1 *histo, TString option) //add histograms and the draw options to this pad
    {
      h.push_back(histo);
      drawoptions.push_back(option);
    }
  void Enable() //enable the pad. this updates also the assigned checkbox
    {
      enabled = kTRUE;
      UpdateStatus();
    }
  void Disable() //disable the pad
    {
      enabled = kFALSE;
      UpdateStatus();
    }
  Bool_t GetStatus() const { return enabled; } //return whether the pad is enabled or disabled
  void SetVars() //initialize variables
    {
      enabled = kTRUE;
    }
  void SetStatus() //synchronize the histopad object and the assigned checkbox
    {
      enabled = checkboxbutton->IsOn();
    }
  TGCheckButton *checkboxbutton; //check box associated to this array of histograms
  std::vector<TH1*> h; // array of histogram pointer
  std::vector<TString> drawoptions; //draw option for each histogram
private:
  void UpdateStatus() // update the checkbox
    {
      if(enabled)
        checkboxbutton->SetOn();
      else
        checkboxbutton->SetOn(kFALSE);
    }
  
  Bool_t enabled; //boolean whether this pad is drawn or not
  
};

class TH2DNew : public TH2D //inherited TH2D class to add a "modified" flag. for this reason some methods are overwritten
{
public:
  TH2DNew(const char *name,const char *title,Int_t nbinsx,Double_t xlow,Double_t xup,Int_t nbinsy,Double_t ylow,Double_t yup) : TH2D(name,title,nbinsx,xlow,xup,nbinsy,ylow,yup), modified(kFALSE)
    {
    
    }
  virtual void Reset(Option_t *option)
    {
      TH2D::Reset(option);
      modified = kTRUE;
    }
  virtual void FillN(Int_t ntimes, const Double_t* x, const Double_t* y, const Double_t* w, Int_t stride = 1)
    {
      TH2D::FillN(ntimes,x,y,w,stride);
      modified = kTRUE;
    }
  virtual Int_t Fill(Double_t x,Double_t y, Double_t w)
    {
      modified = kTRUE;
      return TH2D::Fill(x,y,w);
    }
  virtual Int_t Fill(Double_t x,Double_t y)
    {
      modified = kTRUE;
      return Fill(x,y,1.0);
    }
  Bool_t modified; //flag whether this histogram was modified.
};

class TH1DNew : public TH1D //inherited TH1D class to add a "modified" flag. for this reason some methods are overwritten
{
public:
  TH1DNew(const char *name,const char *title,Int_t nbinsx,Double_t xlow,Double_t xup) : TH1D(name,title,nbinsx,xlow,xup), modified(kFALSE)
    {
    
    }
  virtual void Reset(Option_t *option)
    {
      TH1D::Reset(option);
      modified = kTRUE;
    }
  virtual void FillN(Int_t ntimes, const Double_t* x, const Double_t* w, Int_t stride = 1)
    {
      TH1D::FillN(ntimes,x,w,stride);
      modified = kTRUE;
    }
  virtual Int_t Fill(Double_t x, Double_t w)
    {
      modified = kTRUE;
      return TH1D::Fill(x,w);
    }
  virtual Int_t Fill(Double_t x)
    {
      modified = kTRUE;
      return Fill(x,1.0);
    }
  Bool_t modified; //flag whether this histogram was modified.
};

class ConfigurationClass : public TQObject { //a class holding some configuration informations
public:
  ConfigurationClass () : UPDATE_EVERY_N_EVENTS(40), HITCORR_NUM_BINS(20), CLUSTER_POSITION(1), CLUSTER_TYPE(3), SEED_THRESHOLD(5.0), SEED_NEIGHBOUR_THRESHOLD(2.0), CLUSTER_THRESHOLD(7.0) //some default values for the configuration
    {
    }

  ~ConfigurationClass(){}
  unsigned UPDATE_EVERY_N_EVENTS;
  unsigned HITCORR_NUM_BINS;

  unsigned CLUSTER_POSITION; // 0=seed position, 1=lin cog
  unsigned CLUSTER_TYPE; // 3=3x3, 5=5x5
  double SEED_THRESHOLD;
  double SEED_NEIGHBOUR_THRESHOLD;
  double CLUSTER_THRESHOLD;
};

struct Seed {
  int x, y;
  double c;
  Seed(int x, int y, double c) : x(x), y(y), c(c) {}
  static bool compare(const Seed & lhs, const Seed & rhs) { return lhs.c > rhs.c; }
};

template <typename T>
T square(T val) {
  return val * val;
}

class RootLocker {
public:
  RootLocker() {
    m_locked = !TThread::Lock();
    if (!m_locked) std::cerr << "############\n#### Warning: Root not locked!\n############" << std::endl;
  }
  void Unlock() {
    if (m_locked) TThread::UnLock();
    m_locked = false;
  }
  ~RootLocker() { Unlock(); }
private:
  bool m_locked;
};

static std::string make_name(const std::string & base, int n) {
  return base + eudaq::to_string(n);
}

class RootMonitor : private eudaq::Holder<int>,
                    public TApplication,
                    public TGMainFrame,
                    public eudaq::Monitor {
public:
  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t) //this method implements the communication between gui elements and the underlaying functions
    {
      switch (GET_MSG(msg))
        {
        case  kC_COMMAND:
          switch(GET_SUBMSG(msg))
            {
            case kCM_BUTTON:
              if(parm1 == 150) // the apply button
                {
                  UpdateConf();
                  m_conf_apply->SetEnabled(kFALSE); //the configuration was changed; enable the apply button, so it is again possible to push it
                }
              if(parm1 == 160) //the reset button
                {
                  std::cout << "*** resetting all histograms ***" << std:: endl;
                  for (size_t i = 0; i < m_board.size(); ++i) {
                    m_board[i].Reset();
                  }
                }
              break;
            case kCM_COMBOBOX:
              m_conf_apply->SetEnabled(kTRUE);//the configuration was changed; enable the apply button, so it is again possible to push it
       
              break;
            case kCM_CHECKBUTTON:
              m_conf_apply->SetEnabled(kTRUE);//the configuration was changed; enable the apply button, so it is again possible to push it
              break;
            }
          break;
        case kC_TEXTENTRY:
          switch(GET_SUBMSG(msg))
            {
            case kTE_TEXTCHANGED:
              m_conf_apply->SetEnabled(kTRUE);//the configuration was changed; enable the apply button, so it is again possible to push it
              break;
            }
        }
    
      return kTRUE;
    }
  RootMonitor(const std::string & runcontrol, const std::string & datafile, int x, int y, int w, int h,
              int argc, const char ** argv)
    : eudaq::Holder<int>(argc),
      TApplication("RootMonitor", &m_val, const_cast<char**>(argv)),
      TGMainFrame(gClient->GetRoot(), w, h),
      eudaq::Monitor("Root", runcontrol, datafile),
      m_lastupdate(eudaq::Time::Current()),
      m_modified(true),
      m_runended(false),
      m_prevt((unsigned long long)-1),
      m_histoevents(0),
      
      m_hinttop(new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX, 2, 2, 2, 2)),
      m_hintleft(new TGLayoutHints(kLHintsCenterY | kLHintsLeft, 2, 10, 1, 1)),
      m_hintbig(new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandX | kLHintsExpandY, 4, 4, 4, 4)),
      m_hint_l(new TGLayoutHints(kLHintsLeft | kLHintsTop,20,2,2,2)),
      m_hint_test(new TGLayoutHints(kLHintsLeft | kLHintsTop ,310,0,2,2)),
      
      m_toolbar(new TGHorizontalFrame(this, 800, 20, kFixedWidth)),
      m_tb_filename(new TGLabel(m_toolbar.get(), "                              ")),
      m_tb_runnum(new TGLabel(m_toolbar.get(), "0     ")),
      m_tb_evtnum(new TGLabel(m_toolbar.get(), "0          ")),
      m_tb_reduce(new TGNumberEntry(m_toolbar.get(), 1.0, 3)),
      m_tb_update(new TGNumberEntry(m_toolbar.get(), 10.0, 4)),
      m_tabs(new TGTab(this, 700, 500)),
      m_cluster_size(8)
      //m_timer(new TTimer(this, 500, kFALSE)),
      //m_processtimer(new TProcessEventTimer(100)),
    {
      
      totalnumevents=0;
      
      //m_tb_filename->Set3DStyle(kSunkenFrame);
      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "File name:"), m_hintleft.get());
      m_toolbar->AddFrame(m_tb_filename.get(), m_hintleft.get());
      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "Run #:"), m_hintleft.get());
      m_toolbar->AddFrame(m_tb_runnum.get(), m_hintleft.get());
      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "Event #:"), m_hintleft.get());
      m_toolbar->AddFrame(m_tb_evtnum.get(), m_hintleft.get());
      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "Reduce by:"), m_hintleft.get());
      m_tb_reduce->SetFormat(TGNumberFormat::kNESInteger, TGNumberFormat::kNEAPositive);
      m_toolbar->AddFrame(m_tb_reduce.get(), m_hintleft.get());

      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "Update every:"), m_hintleft.get());
      m_tb_update->SetFormat(TGNumberFormat::kNESRealOne, TGNumberFormat::kNEAPositive);
      m_toolbar->AddFrame(m_tb_update.get(), m_hintleft.get());

      m_toolbar->AddFrame(new TGLabel(m_toolbar.get(), "Colours:"), m_hintleft.get());
      static const int NUM_COL = sizeof COL_R / sizeof *COL_R;
      for (unsigned b = 0; b < MAX_BOARDS; ++b) {
        int i = b % NUM_COL;
        m_colours.push_back(new TColor(COL_BASE+b, COL_R[i], COL_G[i], COL_B[i]));
        TGLabel * label = new TGLabel(m_toolbar.get(), ("Board " + eudaq::to_string(b)).c_str());
        label->SetTextColor(m_colours[b]);
        m_toolbar->AddFrame(label, m_hintleft.get());
      }
    
   
      // For some reason the palette seems to get reset - set it again here:
      gStyle->SetPalette(1);

      AddFrame(m_toolbar.get(), m_hinttop.get());

      m_board = std::vector<BoardDisplay>(MAX_BOARDS); // Maximum number of boards displayed

      //here the code for the configuration tab starts
      //configuration tab
      m_conf_tab = m_tabs->AddTab("Conf");
      m_conf_tab->SetLayoutManager(new TGHorizontalLayout(m_conf_tab.get()));
      m_conf_group_frame = new TGGroupFrame(m_conf_tab.get(),"Settings");
      
      TGFont *ufont;         // will reflect user font changes
      ufont = gClient->GetFont("-*-helvetica-medium-r-*-*-12-*-*-*-*-*-iso8859-1");
      
      TGGC   *uGC;           // will reflect user GC changes
      // graphics context changes
      GCValues_t valEntry730;
      valEntry730.fMask = kGCForeground | kGCBackground | kGCFillStyle | kGCFont | kGCGraphicsExposures;
      gClient->GetColorByName("#000000",valEntry730.fForeground);
      gClient->GetColorByName("#c6c2c6",valEntry730.fBackground);
      valEntry730.fFillStyle = kFillSolid;
      valEntry730.fFont = ufont->GetFontHandle();
      valEntry730.fGraphicsExposures = kFALSE;
      uGC = gClient->GetGC(&valEntry730, kTRUE);
      
      //from here elements to the configuration tab were added
      seedthresholdlabel = new TGLabel(m_conf_group_frame.get(),"Seed Threshold:");
      m_conf_group_frame->AddFrame(seedthresholdlabel.get(), m_hinttop.get());
     
      m_conf_seedthreshold= new TGNumberEntry(m_conf_group_frame.get(), conf.SEED_THRESHOLD, 3);
      m_conf_seedthreshold->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_seedthreshold.get(), m_hinttop.get());

      seedneighbourthresholdlabel = new TGLabel(m_conf_group_frame.get(),"Seed Neighbour Threshold:");
      m_conf_group_frame->AddFrame(seedneighbourthresholdlabel.get(), m_hinttop.get());
     
      m_conf_seedneighbourthreshold= new TGNumberEntry(m_conf_group_frame.get(), conf.SEED_NEIGHBOUR_THRESHOLD, 4);
      m_conf_seedneighbourthreshold->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_seedneighbourthreshold.get(), m_hinttop.get());

      clusterthresholdlabel = new TGLabel(m_conf_group_frame.get(),"Cluster Threshold:");
      m_conf_group_frame->AddFrame(clusterthresholdlabel.get(), m_hinttop.get());
     
      m_conf_clusterthreshold= new TGNumberEntry(m_conf_group_frame.get(), conf.CLUSTER_THRESHOLD, 3);
      m_conf_clusterthreshold->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_clusterthreshold.get(), m_hinttop.get());


      updatecdslabel = new TGLabel(m_conf_group_frame.get(),"Update cds plots after N events:");
      m_conf_group_frame->AddFrame(updatecdslabel.get(), m_hinttop.get());
     
      m_conf_cds_lego_update = new TGNumberEntry(m_conf_group_frame.get(), conf.UPDATE_EVERY_N_EVENTS, 5);
      m_conf_cds_lego_update->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_cds_lego_update.get(), m_hinttop.get());

      numbinshitcorr = new TGLabel(m_conf_group_frame.get(),"Hit Correlation: number of Bins");
      m_conf_group_frame->AddFrame(numbinshitcorr.get(), m_hinttop.get());
      m_conf_numbinshitcorr = new TGNumberEntry(m_conf_group_frame.get(), conf.HITCORR_NUM_BINS, 20);
      m_conf_numbinshitcorr->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_numbinshitcorr.get(), m_hinttop.get());
      
      clustertypelabel = new TGLabel(m_conf_group_frame.get(),"Cluster Type:");
      m_conf_group_frame->AddFrame(clustertypelabel.get(), m_hinttop.get());
      
      clustertypeComboBox = new TGComboBox(m_conf_group_frame.get(),1,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
      clustertypeComboBox->AddEntry("3x3",0);
      clustertypeComboBox->AddEntry("5x5",1);
      clustertypeComboBox->Resize(102,23);
      if(conf.CLUSTER_TYPE == 3)
        clustertypeComboBox->Select(0);
      else if (conf.CLUSTER_TYPE == 5)
        clustertypeComboBox->Select(1);
      else
        clustertypeComboBox->Select(0);
      clustertypeComboBox->Associate(this);
      m_conf_group_frame->AddFrame(clustertypeComboBox.get(), m_hinttop.get());

      clusterpositionlabel = new TGLabel(m_conf_group_frame.get(),"Cluster Position:");
      m_conf_group_frame->AddFrame(clusterpositionlabel.get(), m_hinttop.get());
     
      clusterpositionComboBox = new TGComboBox(m_conf_group_frame.get(),2,kHorizontalFrame | kSunkenFrame | kDoubleBorder | kOwnBackground);
      clusterpositionComboBox->AddEntry("seed position",0);
      clusterpositionComboBox->AddEntry("linear center of gravity",1);
      clusterpositionComboBox->Resize(102,23);
      if(conf.CLUSTER_POSITION == 0)
        clusterpositionComboBox->Select(0);
      else if(conf.CLUSTER_POSITION == 1)
        clusterpositionComboBox->Select(1);
      clusterpositionComboBox->Associate(this);
      m_conf_group_frame->AddFrame(clusterpositionComboBox.get(), m_hinttop.get());

      m_conf_apply = new TGTextButton(m_conf_group_frame.get(),"&Apply",150);
      m_conf_apply->SetEnabled(kFALSE);
      m_conf_apply->Associate(this);
      m_conf_group_frame->AddFrame(m_conf_apply.get(), m_hinttop.get());
      
      m_reset_histos = new TGTextButton(m_conf_group_frame.get(),"&Reset Histograms",160);
      m_reset_histos->Associate(this);
      m_conf_group_frame->AddFrame(m_reset_histos.get(), m_hinttop.get());

      m_conf_group_frame->SetLayoutManager(new TGVerticalLayout(m_conf_group_frame.get()));
      m_conf_group_frame->Resize(m_conf_group_frame->GetDefaultSize());
      m_conf_group_frame->MapWindow();

      m_conf_tab->AddFrame(m_conf_group_frame.get(), m_hint_l.get());       

      for (size_t i = 0; i < m_board.size(); ++i) {
        BookBoard(i, m_board[i]);
      }
      m_histonumtracks = new TH1DNew("NumTracks", "Num Tracks", 100, 0, 100);


      //histograms for hit correlation between neighbor boards
      for (size_t i = 0; i < m_board.size()-1; ++i) {
        TString title;
        char tmpstring[50];
        sprintf(tmpstring, "Hit Correlation Board %1.0f : Board %1.0f", (float)i,(float)(i+1) );
        title = tmpstring;
        m_hitcorrelation.push_back(
				   new TH2DNew(make_name("hitcorrelation",    i).c_str(), title, conf.HITCORR_NUM_BINS, 0, conf.HITCORR_NUM_BINS, conf.HITCORR_NUM_BINS, 0, conf.HITCORR_NUM_BINS)
				   );
        (m_hitcorrelation.back())->SetContour(99);
      }

      //cluster correlations between neigbhor boards
      for (size_t i = 0; i < m_board.size()-1; ++i) {
        TString title;
        char tmpstring[50];
        sprintf(tmpstring, "X Cluster Correlation Board %1.0f : Board %1.0f", (float)i,(float)(i+1) );
        title = tmpstring;
        m_clustercorrelation.push_back(
				       new TH2DNew(make_name("clustercorrelation",    i).c_str(), title,  264, 0, 264, 264, 0, 264)
          );
        (m_clustercorrelation.back())->SetContour(99);
      }
      {
	TString title;
        char tmpstring[50];
        sprintf(tmpstring, "X Cluster Correlation Board 0 : Board %1.0f", (float)(m_board.size()) );
        title = tmpstring;
        m_clustercorrelation.push_back(
				       new TH2DNew(make_name("clustercorrelation",    (m_board.size()-1)).c_str(), title,  264, 0, 264, 264, 0, 264)
				       );
        (m_clustercorrelation.back())->SetContour(99);
      }
      
     //y cluster correlations between neigbhor boards
      for (size_t i = 0; i < m_board.size()-1; ++i) {
        TString title;
        char tmpstring[50];
        sprintf(tmpstring, "Y Cluster Correlation Board %1.0f : Board %1.0f", (float)i,(float)(i+1) );
        title = tmpstring;
        m_clustercorrelationy.push_back(
				       new TH2DNew(make_name("clustercorrelationy",    i).c_str(), title,  264, 0, 264, 264, 0, 264)
          );
        (m_clustercorrelationy.back())->SetContour(99);
      }
      {
	TString title;
        char tmpstring[50];
        sprintf(tmpstring, "Y Cluster Correlation Board 0 : Board %1.0f", (float)(m_board.size()) );
        title = tmpstring;
        m_clustercorrelationy.push_back(
				       new TH2DNew(make_name("clustercorrelationy",    (m_board.size()-1)).c_str(), title,  264, 0, 264, 264, 0, 264)
				       );
        (m_clustercorrelationy.back())->SetContour(99);
      }
      

      //main histogram checkboxes
      m_conf_group_frame_main = new TGGroupFrame(m_conf_tab.get(),"Main Histograms");


      //the checkboxes for the histograms. this needs some improvement and simplification. another class which holds the checkbox and the preferences for this is needed.
      //number of clusters
      m_conf_checkbox_main_numclusters =  new TGCheckButton(m_conf_group_frame_main.get(),"Number of Clusters");
      m_conf_checkbox_main_numclusters->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_numclusters.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_numclusters.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histonumclusters).get(), drawoption);
      }
      //end of number of clusters
      
      //hit correlations
      m_conf_checkbox_main_hitcorr =  new TGCheckButton(m_conf_group_frame_main.get(),"Hit Correlations");
      m_conf_checkbox_main_hitcorr->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_hitcorr.get(), m_hinttop.get());
      for (size_t i = 0; i < m_hitcorrelation.size(); ++i) {
        main_pads.push_back(histopad(m_conf_checkbox_main_hitcorr.get()));
        TString drawoption;
        drawoption =  "col2z";
        (main_pads.back()).AddHisto(m_hitcorrelation[i], drawoption);
      }
      //end of hit correlations


      //cluster correlations
      m_conf_checkbox_main_clustercorr =  new TGCheckButton(m_conf_group_frame_main.get(),"X Cluster Correlations");
      m_conf_checkbox_main_clustercorr->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_clustercorr.get(), m_hinttop.get());
      for (size_t i = 0; i < m_clustercorrelation.size(); ++i) {
        main_pads.push_back(histopad(m_conf_checkbox_main_clustercorr.get()));
        TString drawoption;
        drawoption =  "col2z";
        (main_pads.back()).AddHisto(m_clustercorrelation[i], drawoption);
      }
      //end of cluster correlations

      //y cluster correlations
      m_conf_checkbox_main_clustercorry =  new TGCheckButton(m_conf_group_frame_main.get(),"Y Cluster Correlations");
      m_conf_checkbox_main_clustercorry->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_clustercorry.get(), m_hinttop.get());
      for (size_t i = 0; i < m_clustercorrelationy.size(); ++i) {
        main_pads.push_back(histopad(m_conf_checkbox_main_clustercorry.get()));
        TString drawoption;
        drawoption =  "col2z";
        (main_pads.back()).AddHisto(m_clustercorrelationy[i], drawoption);
      }
      //y end of cluster correlations


      //raw value
      m_conf_checkbox_main_rawval =  new TGCheckButton(m_conf_group_frame_main.get(),"Raw Value");
      m_conf_checkbox_main_rawval->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_rawval.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_rawval.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_historawval).get(), drawoption);
      }
      //end of raw value
      //cluster 2d
      m_conf_checkbox_main_cluster2d =  new TGCheckButton(m_conf_group_frame_main.get(),"Cluster 2d");
      m_conf_checkbox_main_cluster2d->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_cluster2d.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_cluster2d.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "box" : "same box");
        (main_pads.back()).AddHisto((m_board[i].m_histocluster2d).get(), drawoption);
      }
      //end of cluster 2d
      //delta x
      m_conf_checkbox_main_deltax =  new TGCheckButton(m_conf_group_frame_main.get(),"Delta x");
      m_conf_checkbox_main_deltax->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_deltax.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_deltax.get()));
      for (size_t i = 1; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 1 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histodeltax).get(), drawoption);
      }
      //end of delta x
      //delta y
      m_conf_checkbox_main_deltay =  new TGCheckButton(m_conf_group_frame_main.get(),"Delta y");
      m_conf_checkbox_main_deltay->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_deltay.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_deltay.get()));
      for (size_t i = 1; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 1 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histodeltay).get(), drawoption);
      }
      //end of delta y
      //number of seeds
      m_conf_checkbox_main_numhits =  new TGCheckButton(m_conf_group_frame_main.get(),"Number of Seeds");
      m_conf_checkbox_main_numhits->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_numhits.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_numhits.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histonumhits).get(), drawoption);
      }
      //end of number of seeds
      //cds value
      m_conf_checkbox_main_cdsval =  new TGCheckButton(m_conf_group_frame_main.get(),"CDS Value");
      m_conf_checkbox_main_cdsval->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_cdsval.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_cdsval.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histocdsval).get(), drawoption);
      }
      //end of cds value
      
      //noise
      m_conf_checkbox_main_noise =  new TGCheckButton(m_conf_group_frame_main.get(),"Noise");
      m_conf_checkbox_main_noise->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_noise.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_noise.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "" : "same");
        (main_pads.back()).AddHisto((m_board[i].m_histonoise).get(), drawoption);
      }
      //end of noise
      
      //noise as a function of event nr
      m_conf_checkbox_main_noiseeventnr =  new TGCheckButton(m_conf_group_frame_main.get(),"NoiseEventNr");
      m_conf_checkbox_main_noiseeventnr->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_noiseeventnr.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_noiseeventnr.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "hist" : "hist same");
        (main_pads.back()).AddHisto((m_board[i].m_histonoiseeventnr).get(), drawoption);
      }
      //end of noise as a function of event nr
      

      //track 2d
      m_conf_checkbox_main_track2d =  new TGCheckButton(m_conf_group_frame_main.get(),"Tracks 2D");
      m_conf_checkbox_main_track2d->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_track2d.get(),m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_track2d.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "box" : "same box");
        (main_pads.back()).AddHisto((m_board[i].m_histotrack2d).get(), drawoption);
      }
      //end of track 2d
      //cluster charge
      m_conf_checkbox_main_clusterval =  new TGCheckButton(m_conf_group_frame_main.get(),"Cluster Charge");
      m_conf_checkbox_main_clusterval->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_clusterval.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_clusterval.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        TString drawoption;
        drawoption = (i == 0 ? "box" : "same box");
        (main_pads.back()).AddHisto((m_board[i].m_histoclusterval).get(), drawoption);
      }
      //end of cluster charge
      //number of tracks
      m_conf_checkbox_main_numtracks =  new TGCheckButton(m_conf_group_frame_main.get(),"Number of Tracks");
      m_conf_checkbox_main_numtracks->Associate(this);
      m_conf_group_frame_main->AddFrame(m_conf_checkbox_main_numtracks.get(), m_hinttop.get());
      main_pads.push_back(histopad(m_conf_checkbox_main_numtracks.get()));
      (main_pads.back()).AddHisto(m_histonumtracks.get(), "");
      //end of number of tracks

      m_conf_group_frame_main->SetLayoutManager(new TGVerticalLayout(m_conf_group_frame_main.get()));
      m_conf_group_frame_main->Resize(m_conf_group_frame_main->GetDefaultSize());
      m_conf_group_frame_main->MoveResize(280,16,192,152);
      m_conf_group_frame_main->MapWindow();
      //end of main histogram checkboxes
      m_conf_tab->AddFrame(m_conf_group_frame_main.get(), m_hint_l.get());
      
      m_conf_group_frame_cdslego = new TGGroupFrame(m_conf_tab.get(),"CDS Lego Plots");
      m_conf_tab->AddFrame(m_conf_group_frame_cdslego.get(), m_hint_l.get());

      //cds lego plot
      m_conf_checkbox_cdslego =  new TGCheckButton(m_conf_group_frame_cdslego.get(),"CDS Lego");
      m_conf_checkbox_cdslego->Associate(this);
      m_conf_group_frame_cdslego->AddFrame(m_conf_checkbox_cdslego.get(), m_hinttop.get());
      cdslego_pads.push_back(histopad(m_conf_checkbox_cdslego.get()));
      for (size_t i = 0; i < m_board.size(); ++i) {
        (cdslego_pads.back()).AddHisto((m_board[i].m_testhisto).get(), "SURF2ZFBBB"); //
        //(cdslego_pads.back()).AddHisto((m_board[i].m_testhisto).get(), "scat");
      }
      //end of cds lego plot

      m_conf_group_frame_board = new TGGroupFrame(m_conf_tab.get(),"Board Displays");

      //clusterx
      m_conf_checkbox_clusterx =  new TGCheckButton(m_conf_group_frame_board.get(),"Cluster X");
      m_conf_checkbox_clusterx->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_clusterx.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_clusterx.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histoclusterx).get(), "");
      }
      //end of clusterx
     
      //clustery
      m_conf_checkbox_clustery =  new TGCheckButton(m_conf_group_frame_board.get(),"Cluster Y");
      m_conf_checkbox_clustery->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_clustery.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_clustery.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histoclustery).get(), "");
      }
      //end of clustery
     
      //raw2d
      m_conf_checkbox_raw2d =  new TGCheckButton(m_conf_group_frame_board.get(),"Raw 2D");
      m_conf_checkbox_raw2d->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_raw2d.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_raw2d.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_historaw2d).get(), "colz");
      }
      //end of raw2d
    
      //cds2d
      m_conf_checkbox_cds2d =  new TGCheckButton(m_conf_group_frame_board.get(),"CDS 2D");
      m_conf_checkbox_cds2d->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_cds2d.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_cds2d.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histocds2d).get(), "colz");
      }
      //end of cds2d

      //cluster 2d
      m_conf_checkbox_cluster2d =  new TGCheckButton(m_conf_group_frame_board.get(),"Cluster 2D");
      m_conf_checkbox_cluster2d->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_cluster2d.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_cluster2d.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histocluster2d).get(), "colz");
      }
      //end of cluster2d
     
      //raw value
      m_conf_checkbox_rawval =  new TGCheckButton(m_conf_group_frame_board.get(),"Raw Value");
      m_conf_checkbox_rawval->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_rawval.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_rawval.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_historawval).get(), "");
      }
      //end of raw value
      //noise 2d
      m_conf_checkbox_noise2d =  new TGCheckButton(m_conf_group_frame_board.get(),"Noise 2D");
      m_conf_checkbox_noise2d->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_noise2d.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_noise2d.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histonoise2d).get(), "colz");
      }
      //end of noise2d
      //raw x
      m_conf_checkbox_rawx =  new TGCheckButton(m_conf_group_frame_board.get(),"Raw X");
      m_conf_checkbox_rawx->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_rawx.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_rawx.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_historawx).get(), "");
      }
      //end of raw x
      //raw y
      m_conf_checkbox_rawy =  new TGCheckButton(m_conf_group_frame_board.get(),"Raw Y");
      m_conf_checkbox_rawy->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_rawy.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_rawy.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_historawy).get(), "");
      }
      //end of raw y
      //cds value
      m_conf_checkbox_cdsval =  new TGCheckButton(m_conf_group_frame_board.get(),"CDS Value");
      m_conf_checkbox_cdsval->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_cdsval.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_cdsval.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histocdsval).get(), "");
      }
      //end of cds value
      //noise
      m_conf_checkbox_noise =  new TGCheckButton(m_conf_group_frame_board.get(),"Noise");
      m_conf_checkbox_noise->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_noise.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_noise.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histonoise).get(), "");
      }
      //end of noise


      //noise as a function of eventnr
      m_conf_checkbox_noiseeventnr =  new TGCheckButton(m_conf_group_frame_board.get(),"NoiseEventnr");
      m_conf_checkbox_noiseeventnr->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_noiseeventnr.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_noiseeventnr.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histonoiseeventnr).get(), "");
      }
      //end of noise as a function of eventnr





      //number of seeds
      m_conf_checkbox_numhits =  new TGCheckButton(m_conf_group_frame_board.get(),"Number of Seeds");
      m_conf_checkbox_numhits->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_numhits.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_numhits.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histonumhits).get(), "");
      }
      //end of number of seeds
      //cluster charge
      m_conf_checkbox_clusterval =  new TGCheckButton(m_conf_group_frame_board.get(),"Cluster Charge");
      m_conf_checkbox_clusterval->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_clusterval.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_clusterval.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histoclusterval).get(), "");
      }
      //end of cluster charge
      //number of clusters
      m_conf_checkbox_numclusters =  new TGCheckButton(m_conf_group_frame_board.get(),"Number of Clusters");
      m_conf_checkbox_numclusters->Associate(this);
      m_conf_group_frame_board->AddFrame(m_conf_checkbox_numclusters.get(), m_hinttop.get());
      board_pads.push_back(std::vector<histopad>(m_board.size(),histopad(m_conf_checkbox_numclusters.get())));
      for (size_t i = 0; i <  m_board.size(); ++i) {
        board_pads.back().at(i).AddHisto((m_board[i].m_histonumclusters).get(), "");
      }
      //end of number of clusters
      
      m_conf_tab->AddFrame(m_conf_group_frame_board.get(), m_hint_l.get());

      m_conf_tab->Resize(m_conf_tab->GetDefaultSize());
      m_conf_tab->MapWindow();
      
    
      UpdateBoardCanvas(); //draw and update the board tabs
     

      //cds lego canvas
      TGCompositeFrame * cdslegoframe = m_tabs->AddTab("CDSLego");
      m_embedcdslego  = new TRootEmbeddedCanvas("CDS Lego", cdslegoframe, w, h);
      cdslegoframe->AddFrame(m_embedcdslego, m_hintbig.get());
      m_cds_lego_canvas = m_embedcdslego->GetCanvas();

      UpdateCDSLegoCanvas(); //draw and update the board tabs

      cdslegoframe->Resize(m_conf_tab->GetDefaultSize());
      cdslegoframe->MapWindow();
      

   
      // Main tab
      TGCompositeFrame * frame = m_tabs->AddTab("Main");
      m_embedmain = new TRootEmbeddedCanvas("MainCanvas", frame, w, h);
      frame->AddFrame(m_embedmain, m_hintbig.get());
      m_canvasmain = m_embedmain->GetCanvas();
   
      UpdateMainCanvas();



      // Add tabs to window
      AddFrame(m_tabs.get(), m_hintbig.get());

      //m_timer->TurnOn();
      SetWindowName("EUDAQ Root Monitor");
      MapSubwindows();
      // The following line is needed! even if we resize again afterwards
      Resize(GetDefaultSize());
      MapWindow();
      MoveResize(x, y, w, h);
    }
  ~RootMonitor() {
    //std::cout << "Destructor" << std::endl;
    for(size_t i =0; i < m_hitcorrelation.size();i++)
      delete m_hitcorrelation[i];
    for(size_t i =0; i < m_clustercorrelation.size();i++)
      delete m_clustercorrelation[i];
    for(size_t i =0; i < m_clustercorrelationy.size();i++)
      delete m_clustercorrelationy[i];
   
    gApplication->Terminate();
  }
  void UpdateCDSLegoCanvas() //this function updates the cds lego canvas
    {
      m_cds_lego_canvas->Clear(); //first clean the old canvas
      if(cdslego_pads.size() > 0)
        {
          if(cdslego_pads[0].GetStatus()) //only update the canvas if the checkbox was enabled
            {
              m_cds_lego_canvas->Divide(3,2); //
              for (size_t j = 0; j < m_board.size()-1; ++j) //add the plots for each board
                {
                  m_cds_lego_canvas->cd(j+1);
                  gStyle->SetPalette(1,0);
                  gPad->SetRightMargin(0.13);
                  //  cdslego_pads[0].h[j]->GetZaxis()->SetTitleColor(10);
//   cdslego_pads[0].h[j]->GetZaxis()->SetLabelColor(10);
//     cdslego_pads[0].h[j]->GetZaxis()->SetAxisColor(10);
//   TPaletteAxis *palette = new TPaletteAxis(0.798918, -0.894338, 0.8911, 0.894338,  cdslego_pads[0].h[j]);
//   palette->SetLabelColor(1);
//   palette->SetLabelFont(62);
//   palette->SetLabelOffset(0.005);
//   palette->SetLabelSize(0.04);
//   palette->SetTitleOffset(1);
//   palette->SetTitleSize(0.04);
//   palette->SetFillColor(100);
//   palette->SetFillStyle(1001);
//   cdslego_pads[0].h[j]->GetListOfFunctions()->Add(palette,"br");

                  cdslego_pads[0].h[j]->DrawCopy(cdslego_pads[0].drawoptions[j]); //access the plot, that were assigned to this pad and draw it
                }
            }
        }
    }
  void UpdateBoardCanvas() //update the board display canvases
    {
      for (size_t i = 0; i <  m_board.size(); ++i) //loop over all boards
        {
          m_board[i].m_canvas->Clear(); //clear the canvas for this board
          int activepads = 0; //counter for the number auf active pads. this is important to divide the canvas in a suitable way

          for(size_t j = 0; j < board_pads.size(); j++)
            {
              if(board_pads[j].at(0).GetStatus()) //count the number of pads with enabled plots
                activepads++;
            }
 
          //now divide the canvas depending on the number of active pads. a more intelligent algorithm should be added
          if(activepads <= 3)
            m_board[i].m_canvas->Divide(activepads, 1);
          else if(activepads > 3 && activepads <= 6)
            m_board[i].m_canvas->Divide(3, 2);
          else if(activepads > 6 && activepads <= 9)
            m_board[i].m_canvas->Divide(3, 3);
          else if(activepads > 9 && activepads <= 12)
            m_board[i].m_canvas->Divide(4, 3);
          else if(activepads > 12 && activepads <= 16)
            m_board[i].m_canvas->Divide(4, 4);
	  else if(activepads > 16 && activepads <= 20)
	    m_board[i].m_canvas->Divide(5, 4);
	  else if(activepads > 20 && activepads <= 22)
	    m_board[i].m_canvas->Divide(5, 5);
	  else if(activepads > 22 && activepads <= 30)
	    m_board[i].m_canvas->Divide(6, 5);
	  else if(activepads > 30 && activepads <= 36)
	    m_board[i].m_canvas->Divide(6, 6);
	  else if(activepads > 36 && activepads <= 42)
	    m_board[i].m_canvas->Divide(7, 6);
	  else if(activepads > 42 && activepads <= 49)
	    m_board[i].m_canvas->Divide(7, 7);
	  
          int canvaspadindex = 1;
          for(size_t t = 0; t < board_pads.size(); t++)
            {
              m_board[i].m_canvas->cd(canvaspadindex);
              if(board_pads[t].at(i).GetStatus()) //if the pad is active, it is drawn
                {
                  board_pads[t].at(i).h.at(0)->DrawCopy( board_pads[t].at(i).drawoptions.at(0)); //index is equal to 0 because in each board display tab only one plot per pad is drawn
                  canvaspadindex++;
                }
            }
        }
    }
  void UpdateMainCanvas() //update the main canvas
    {
      m_canvasmain->Clear(); //first clear the old canvas. this deletes all subpads

      int activepads = 0; //counter for the number auf active pads. this is important to divide the canvas in a suitable way
    
      for(size_t i = 0; i < main_pads.size(); i++)
        {
          if(main_pads[i].GetStatus())//count the number of pads with enabled plots
            activepads++;
        }
      //now divide the canvas depending on the number of active pads.
      if(activepads <= 3)
        m_canvasmain->Divide(activepads, 1);
      else if(activepads > 3 && activepads <= 6)
        m_canvasmain->Divide(3, 2);
      else if(activepads > 6 && activepads <= 9)
        m_canvasmain->Divide(3, 3);
      else if(activepads > 9 && activepads <= 12)
        m_canvasmain->Divide(4, 3);
      else if(activepads > 12 && activepads <= 16)
        m_canvasmain->Divide(4, 4);
      else if(activepads > 16 && activepads <= 20)
        m_canvasmain->Divide(5, 4);
      else if(activepads > 20 && activepads <= 22)
        m_canvasmain->Divide(5, 5);
      else if(activepads > 22 && activepads <= 30)
        m_canvasmain->Divide(6, 5);
      else if(activepads > 30 && activepads <= 36)
        m_canvasmain->Divide(6, 6);
     else if(activepads > 36 && activepads <= 42)
        m_canvasmain->Divide(7, 6);
     else if(activepads > 42 && activepads <= 49)
        m_canvasmain->Divide(7, 7);

  
  
       
      int canvaspadindex = 1;
      for(size_t i = 1; i <= main_pads.size(); i++)
        {
          m_canvasmain->cd(canvaspadindex);
          if(main_pads[i-1].GetStatus())//if the pad is active, it is drawn
            {
              for(size_t j = 0; j < (main_pads[i-1].h).size(); j++) //loop over all plots
                {
                  if(main_pads[i-1].h[j]->InheritsFrom("TH2D")) //if it is a 2d plot, set the correct fillcolor
                    main_pads[i-1].h[j]->SetFillColor(COL_BASE+j);
                  else //otherwise change the linecolor
                    main_pads[i-1].h[j]->SetLineColor(COL_BASE+j); 
                  main_pads[i-1].h[j]->Draw(main_pads[i-1].drawoptions.at(j));
                }
              canvaspadindex++;
            }
        }
   
    }
  void UpdateConf() //after pushing the "apply" button in the configuration tab, this function is called. it updates the configuration
    {
      //which histograms to be displayed?
      for(size_t i = 0; i < main_pads.size(); i++)
        {
          main_pads[i].SetStatus(); //synchronize histopads and checkboxes
        }
    
      for(size_t i = 0; i < board_pads.size(); i++)
        {
          for(size_t t = 0; t < board_pads[i].size(); t++)
            {
              board_pads[i].at(t).SetStatus();//synchronize histopads and checkboxes
            }
        }
      UpdateBoardCanvas(); //update the canvases

      UpdateMainCanvas(); //update the canvas
    
      cdslego_pads[0].SetStatus();
      UpdateCDSLegoCanvas(); //update the canvas
    
      unsigned cdsupdate = (unsigned)m_conf_cds_lego_update->GetNumber();
      if(cdsupdate > 0)
        conf.UPDATE_EVERY_N_EVENTS = cdsupdate;


      unsigned numbinshitcorr = (unsigned)m_conf_numbinshitcorr->GetNumber();
      if(numbinshitcorr != conf.HITCORR_NUM_BINS && numbinshitcorr > 0)
	{
	  for (size_t i = 0; i < m_hitcorrelation.size(); ++i) {
	    delete m_hitcorrelation[i];
	    TString title;
	    char tmpstring[50];
	    sprintf(tmpstring, "Hit Correlation Board %1.0f : Board %1.0f", (float)i,(float)(i+1) );
	    title = tmpstring;
	    m_hitcorrelation[i] = new TH2DNew(make_name("hitcorrelation",    i).c_str(), title,   numbinshitcorr, 0, numbinshitcorr, numbinshitcorr, 0, numbinshitcorr);
	    m_hitcorrelation[i]->SetContour(99);
	    
	    main_pads[i].h[0] = m_hitcorrelation[i];
	  }
	  conf.HITCORR_NUM_BINS = numbinshitcorr;
	  UpdateMainCanvas(); //update the canvas
	}
      


      double seedthresh = (double) m_conf_seedthreshold->GetNumber();
      if(seedthresh > 0)
        conf.SEED_THRESHOLD = seedthresh;

      double seedneighbourthresh = (double) m_conf_seedneighbourthreshold->GetNumber();
      if(seedneighbourthresh > 0)
        conf.SEED_NEIGHBOUR_THRESHOLD = seedneighbourthresh;
    
      double clusterthresh = (double) m_conf_clusterthreshold->GetNumber();
      if(clusterthresh > 0)
        conf.CLUSTER_THRESHOLD = clusterthresh;



      //fast and dirty fix. only reset histograms if cluster position or cluster type was changed
      int tmpclustertype = 0;
      if(conf.CLUSTER_TYPE == 3)
        tmpclustertype = 0;
      if(conf.CLUSTER_TYPE == 5)
        tmpclustertype = 1;
    
      if ( clusterpositionComboBox->GetSelected() != (int)conf.CLUSTER_POSITION || clustertypeComboBox->GetSelected() != tmpclustertype)
        {
          std::cout << "*** resetting all histograms ***" << std:: endl;
          for (size_t i = 0; i < m_board.size(); ++i) {
            m_board[i].Reset();
            m_hitcorrelation[i]->Reset("");
	    m_clustercorrelation[i]->Reset("");
	     m_clustercorrelationy[i]->Reset("");
	  }
 
        }
      //read out the gui elements and change the configuration object
      if(clustertypeComboBox->GetSelected() == 0)
        conf.CLUSTER_TYPE = 3;
      if(clustertypeComboBox->GetSelected() == 1)
        conf.CLUSTER_TYPE = 5;
      if(clusterpositionComboBox->GetSelected() == 0)
        conf.CLUSTER_POSITION = 0;
      if(clusterpositionComboBox->GetSelected() == 1)
        conf.CLUSTER_POSITION = 1;


      Update(); //update all canvases
    }
  virtual void StartIdleing() {
    eudaq::Time now = eudaq::Time::Current();
    bool needupdate = (now - m_lastupdate) > eudaq::Time(0, (int)(1e6*m_tb_update->GetNumber()));
    if (m_modified && (needupdate || m_runended)) {
      Update();
      m_lastupdate = now;
    }
  }
  virtual void OnConfigure(const std::string & param) {
    std::cout << "Configure: " << param << std::endl;
    SetStatus(eudaq::Status::LVL_OK);
  }
  virtual void OnTerminate() {
    std::cout << "Terminating" << std::endl;
    gApplication->Terminate();
  }
  virtual void OnReset() {
    std::cout << "Reset" << std::endl;
    SetStatus(eudaq::Status::LVL_OK);
  }
  virtual void OnStartRun(unsigned param) {
    RootLocker lock;
    Monitor::OnStartRun(param);
    m_tb_filename->SetText(m_datafile.c_str());
    m_tb_runnum->SetText(eudaq::to_string(param).c_str());
    
 
    
    for (size_t i = 0; i < m_board.size(); ++i) {
      //      std::cout << "i=" << i << std::endl;
      m_board[i].Reset();
    }
  }
  virtual void OnEvent(counted_ptr<eudaq::DetectorEvent> ev) {
    RootLocker lock;
   
    //std::cout << *ev << std::endl;
    if (ev->IsBORE()) {
      m_decoder = new eudaq::EUDRBDecoder(*ev);
      eudaq::EUDRBEvent * drbev = 0;
      for (size_t i = 0; i < ev->NumEvents(); ++i) {
        drbev = dynamic_cast<eudaq::EUDRBEvent *>(ev->GetEvent(i));
        if (drbev) {
          break;
        }
      }
      if (!drbev) EUDAQ_THROW("No EUDRB detected");
      m_histoevents = 0;
      // Initialize histograms
    } else if (ev->IsEORE()) {
      std::string filename = m_datafile;
      size_t dot = filename.find_last_of("./\\:");
      if (dot != std::string::npos && filename[dot] == '.') filename.erase(dot);
      filename += ".root";
      TFile rootfile(filename.c_str(), "RECREATE");
      for (size_t i = 0; i < m_board.size(); ++i) {
        m_board[i].m_testhisto->Write();
        m_board[i].m_historaw2d->Write();
        m_board[i].m_histocds2d->Write();
        m_board[i].m_histohit2d->Write();
        m_board[i].m_histocluster2d->Write();
        m_board[i].m_histotrack2d->Write();
        m_board[i].m_histonoise2d->Write();
        m_board[i].m_historawx->Write();
        m_board[i].m_historawy->Write();
        m_board[i].m_histoclusterx->Write();
        m_board[i].m_histoclustery->Write();
        m_board[i].m_historawval->Write();
        m_board[i].m_histocdsval->Write();
	m_board[i].m_histonoise->Write();
	m_board[i].m_histonoiseeventnr->Write();
  	
        m_board[i].m_histoclusterval->Write();
        if (i > 0) {
          m_board[i].m_histodeltax->Write();
          m_board[i].m_histodeltay->Write();
        }
      }
      //m_histo
      m_runended = true;
    } else {
      // Data event
      unsigned reduce = static_cast<unsigned>(m_tb_reduce->GetNumber());
      // Fill histograms
      m_prevt = ev->GetTimestamp();

      unsigned numplanes = 0;

      if ((ev->GetEventNumber() % reduce) == 0) {
        m_tb_evtnum->SetText(eudaq::to_string(ev->GetEventNumber()).c_str());
        for (size_t i = 0; i < ev->NumEvents(); ++i) {
          if (eudaq::EUDRBEvent * drbev = dynamic_cast<eudaq::EUDRBEvent *>(ev->GetEvent(i))) {
            if (drbev->NumBoards() > 0) {
              m_histoevents++;
              try {
                totalnumevents++;
                std::vector<unsigned int> numberofclusters(m_board.size(),0);
                std::vector<std::vector<double> > cpos(m_board.size());
                std::vector<std::vector<double> > cposy(m_board.size());
                
		//		std::cout << "Numboards " << drbev->NumBoards() << ", " << m_board.size() << std::endl;
                for (size_t i = 0; i < drbev->NumBoards() && i < m_board.size(); ++i) {
                  numplanes++;
		  std::vector<double> clusterposition;
		  std::vector<double> clusterpositiony;
		  
                  FillBoard(m_board[i], drbev->GetBoard(i),i,numberofclusters[i], clusterposition, clusterpositiony);
		  //cpos.push_back(clusterposition);
		  cpos.at(i) = clusterposition;
		  cposy.at(i) = clusterpositiony;
		  
		}
		//
               for(size_t i = 0; i < m_board.size()-1; i++)
                  {
                   
		   
		    for(size_t k = 0; k < cposy.at(i).size(); k++)
		      {

			for(size_t l = 0; l < cposy.at(i+1).size(); l++)
			  {
			    m_clustercorrelationy[i]->Fill(cposy.at(i+1).at(l),cposy.at(i).at(k));
			  }
		      }
		  }
		
		for(size_t k = 0; k < cposy.at(0).size(); k++)
		  {
		    
		    for(size_t l = 0; l < cposy.at((int)m_board.size()-1).size(); l++)
		      {
			
		       (m_clustercorrelationy.back())->Fill(cposy.at((int)m_board.size()-1).at(l),cposy.at(0).at(k));
		      }
		  }
		//
                for(size_t i = 0; i < m_board.size()-1; i++)
                  {
                    if(numberofclusters[i] != 0 || numberofclusters[i+1] != 0)
                      m_hitcorrelation[i]->Fill(numberofclusters[i+1],numberofclusters[i]);
		    //cluster correlation
		    for(size_t k = 0; k < cpos.at(i).size(); k++)
		      {

			for(size_t l = 0; l < cpos.at(i+1).size(); l++)
			  {
			    m_clustercorrelation[i]->Fill(cpos.at(i+1).at(l),cpos.at(i).at(k));
			  }
		      }
		  }
		
		for(size_t k = 0; k < cpos.at(0).size(); k++)
		  {
		    
		    for(size_t l = 0; l < cpos.at((int)m_board.size()-2).size(); l++)
		      {
			
		       (m_clustercorrelation.back())->Fill(cpos.at((int)m_board.size()-2).at(l),cpos.at(0).at(k));
		      }
		  }
		
	 //end of cluster correlation


// 		std::cout << "cpos size=" << cpos.size() << std::endl;
// 		for(size_t i = 0; i < m_board.size(); i++)
// 		  {
// 		    std::cout << i;
// 		    std::cout << " size=" << cpos.at(i).size() << std::endl;
// 		  }
              } catch (const eudaq::Exception & e) {
                EUDAQ_ERROR("Bad data size in event " + eudaq::to_string(ev->GetEventNumber()) + ": " + e.what());
              }
            }
            break;
          }
        }
      }
      unsigned planeshit = 0;
      std::cout << "Event " << ev->GetEventNumber() << ", clusters:";
      for (size_t i = 0; i < numplanes; ++i) {
        std::cout << " " << m_board[i].m_clusters.size();
      }
      std::cout << std::endl;
      int numtracks = 0;




#if 1 //

      for (size_t i = 0; i < numplanes; ++i) {
        if (m_board[i].m_clusters.size() >= 1) {
          planeshit++;
        }
      }
      if (planeshit < numplanes) {
        std::cout << "No track candidate" << std::endl;
      } else {
        const double alignx[] = { -10,  -5, -20, 30 };
        const double aligny[] = { -10,   5, -10, 10 };
        const double thresh[] = {  20,  20,  40, 20 };
        //const double thresh2 = square(20);
        for (size_t i = 0; i < m_board[0].m_clusters.size(); ++i) {
          //std::cout << "Trying cluster " << i << std::endl;
          size_t board = 1;
          std::vector<double> trackx(numplanes), tracky(numplanes);
          trackx[0] = m_board[0].m_clusterx[i];
          tracky[0] = m_board[0].m_clustery[i];
          double closest2;
          size_t iclosest = 0;
          //bool havecluster;
          do {
            closest2 = 2.0;
            //havecluster = false;
            for (size_t cluster = 0; cluster < m_board[board].m_clusters.size(); ++cluster) {
              double x = m_board[board].m_clusterx[cluster];
              double y = m_board[board].m_clustery[cluster];
              if (board >= 3) { // ????????????????????
                x = 263 - x;
              }
              double dx = x - trackx[board-1] - alignx[board-1];
              double dy = y - tracky[board-1] - aligny[board-1];
              double dist2 = (square(dx) + square(dy)) / square(thresh[board-1]);
              //std::cout << "dist^2 = " << dist2 << std::endl;
              if (dist2 < closest2) {
                iclosest = cluster;
                closest2 = dist2;
                //havecluster = true;
                trackx[board] = x;
                tracky[board] = y;
              }
            }
            ++board;
            //std::cout << "closest = " << std::sqrt(closest2) << std::endl;
          } while (board < numplanes && closest2 < 1);
          if (closest2 < 1) {
            numtracks++;
            //std::cout << "Found track in event " << ev->GetEventNumber() << ":";
            for (size_t i = 0; i < trackx.size(); ++i) {
              //std::cout << " (" << trackx[i] << ", " << tracky[i] << ")";
              m_board[i].m_histotrack2d->Fill(trackx[i], tracky[i]);
              if (i > 0) {
                m_board[i].m_histodeltax->Fill(trackx[i] - trackx[i-1]);
                m_board[i].m_histodeltay->Fill(tracky[i] - tracky[i-1]);
              }
            }
            //std::cout << std::endl;
            //fittrack(trackx, tracky);
          }
        }
      }

#else

      for (size_t i = 0; i < numplanes; ++i) {
        if (m_board[i].m_clusters.size() == 1) {
          planeshit++;
        }
      }
      if (planeshit < numplanes-2) {
        std::cout << "No track candidate" << std::endl;
      } else {
        numtracks++;
        std::ostringstream s;
        s << "Found track candidate in event " << ev->GetEventNumber() << ":";
        double x = -1, y = -1;
        double clustval = 0;
        for (size_t i = 0; i < numplanes; ++i) {
          if (m_board[i].m_clusters.size() < 1) {
            m_board[i].m_trackx = -1;
            m_board[i].m_tracky = -1;
            s << " ()";
          } else {
            if (x == -1) { // first plane: find highest cluster
              for (size_t c = 0; c < m_board[i].m_clusters.size(); ++c) {
                if (m_board[i].m_clusters[c] > clustval) {
                  clustval = m_board[i].m_clusters[c];
                  x = m_board[i].m_clusterx[c];
                  y = m_board[i].m_clustery[c];
                }
              }
            } else { // find closest cluster
              double d2 = -1;
              for (size_t c = 0; c < m_board[i].m_clusters.size(); ++c) {
                double dd2 = square(m_board[i].m_clusterx[c]) + square(m_board[i].m_clustery[c]);
                if (d2 < 0 || dd2 < d2) {
                  d2 = dd2;
                  clustval = m_board[i].m_clusters[c];
                  x = m_board[i].m_clusterx[c];
                  y = m_board[i].m_clustery[c];
                }
              }
            }
            m_board[i].m_trackx = x;
            m_board[i].m_tracky = y;
            m_board[i].m_histotrack2d->Fill(m_board[i].m_trackx, m_board[i].m_tracky);
            if (i > 0 && m_board[i-1].m_trackx != -1) {
              m_board[i].m_histodeltax->Fill(m_board[i].m_trackx - m_board[i-1].m_trackx);
              m_board[i].m_histodeltay->Fill(m_board[i].m_tracky - m_board[i-1].m_tracky);
            }
            s << " (" << m_board[i].m_trackx
              << ", " << m_board[i].m_tracky
              << ", " << clustval
              << ", " << m_board[i].m_clusters.size()
              << ")";
          }
        }
        EUDAQ_EXTRA(s.str());
      }

#endif
      //m_board[0].m_testhisto->SetMaximum();
      m_histonumtracks->Fill(numtracks);
      m_board[0].m_historawval->SetMaximum();
      m_board[0].m_histocdsval->SetMaximum();
      m_board[0].m_histonoise->SetMaximum();
      m_board[0].m_histonoiseeventnr->SetMaximum();
     
      m_board[0].m_histoclusterval->SetMaximum();
      m_board[0].m_histonumhits->SetMaximum();
      m_board[0].m_histonumclusters->SetMaximum();
      m_board[1].m_histodeltax->SetMaximum();
      m_board[1].m_histodeltay->SetMaximum();
      double maxr = m_board[0].m_historawval->GetMaximum();
      double maxd = m_board[0].m_histocdsval->GetMaximum();
      double max_noise = m_board[0].m_histonoise->GetMaximum();
      double max_noise_eventnr = m_board[0].m_histonoiseeventnr->GetMaximum();
      
      double maxc = m_board[0].m_histoclusterval->GetMaximum();
      double maxh = m_board[0].m_histonumhits->GetMaximum();
      double maxn = m_board[0].m_histonumclusters->GetMaximum();
      double maxx = m_board[1].m_histodeltax->GetMaximum();
      double maxy = m_board[1].m_histodeltay->GetMaximum();
      for (size_t i = 0; i < m_board.size(); ++i) {
 
//  m_board[i].m_testhisto->SetMaximum(40.0);
        //m_board[i].m_testhisto->SetMinimum(conf.SEED_NEIGHBOUR_THRESHOLD*5.0);
        m_board[i].m_testhisto->SetMinimum(0.0); //set the minimum of the cds lego plots
      }
      for (size_t i = 1; i < numplanes; ++i) {
 

        if (m_board[i].m_historawval->GetMaximum() > maxr) maxr = m_board[i].m_historawval->GetMaximum();
        if (m_board[i].m_histocdsval->GetMaximum() > maxd) maxd = m_board[i].m_histocdsval->GetMaximum();
	if (m_board[i].m_histonoise->GetMaximum() > max_noise) max_noise = m_board[i].m_histonoise->GetMaximum();
	if (m_board[i].m_histonoiseeventnr->GetMaximum() > max_noise_eventnr) max_noise_eventnr = m_board[i].m_histonoiseeventnr->GetMaximum();
        if (m_board[i].m_histoclusterval->GetMaximum() > maxc) maxc = m_board[i].m_histoclusterval->GetMaximum();
        if (m_board[i].m_histonumhits->GetMaximum() > maxh) maxh = m_board[i].m_histonumhits->GetMaximum();
        if (m_board[i].m_histonumclusters->GetMaximum() > maxn) maxn = m_board[i].m_histonumclusters->GetMaximum();
        if (m_board[i].m_histodeltax->GetMaximum() > maxx) maxx = m_board[i].m_histodeltax->GetMaximum();
        if (m_board[i].m_histodeltay->GetMaximum() > maxy) maxy = m_board[i].m_histodeltay->GetMaximum();
      }
      m_board[0].m_historawval->SetMaximum(maxr*1.1);
      m_board[0].m_histocdsval->SetMaximum(maxd*1.1);
      m_board[0].m_histonoise->SetMaximum(max_noise*1.1);
      m_board[0].m_histonoiseeventnr->SetMaximum(max_noise_eventnr*1.1);
      
      m_board[0].m_histoclusterval->SetMaximum(maxc*1.1);
      m_board[0].m_histonumhits->SetMaximum(maxh*1.1);
      m_board[0].m_histonumclusters->SetMaximum(maxn*1.1);
      m_board[1].m_histodeltax->SetMaximum(maxx*1.1);
      m_board[1].m_histodeltay->SetMaximum(maxy*1.1);
      m_modified = true;
      if (m_histoevents == 100) {
        //m_canvasmain->cd(1)->SetLogy();
        //m_canvasmain->cd(5)->SetLogy();
        //m_canvasmain->cd(9)->SetLogy();
      }
    }
  }
  virtual void OnBadEvent(counted_ptr<eudaq::Event> ev) {
    EUDAQ_ERROR("Bad event type found in data file");
    std::cout << "Bad Event: " << *ev << std::endl;
  }
  void Update() {
    TSeqCollection * list = gROOT->GetListOfCanvases();
    Int_t NCanvas = list->GetEntries();
    //std::cout << "------------------------------------------------start update" << std::endl;
    for (Int_t i = 0; i < NCanvas; ++i) {
      TCanvas * c = (TCanvas*)list->At(i);
      int j = 0;
      int totnumpads = 0;
      int updatedpads = 0;
      while (TVirtualPad * p = c->GetPad(++j)) {
        TList *l;
        l = p->GetListOfPrimitives();
        // std::cout << "pad entries = " << (l->GetEntries()) <<  std::endl;
        Bool_t mod = kFALSE;
        for (Int_t j = 0; j < l->GetEntries(); j++) { //this loop updates ONLY pads that contain updated and modified plots
          if(l->At(j)->InheritsFrom("TH2D")) //ensure that we have histograms and not another tobject like a title and so on
            {
              TH2DNew *h = (TH2DNew*)l->At(j);
              if(h->modified)
                {
                  mod = kTRUE;
                  h->modified = kFALSE;
                }
            }
          if(l->At(j)->InheritsFrom("TH1D"))
            {
              TH1DNew *h = (TH1DNew*)l->At(j);
              if(h->modified)
                {
                  mod = kTRUE;
                  h->modified = kFALSE;
                }
            }
        }
        if(mod)
          {
            p->Modified();
            updatedpads++;
          }
        totnumpads++;
      }
      c->Update();
      //std::cout << "updated pads / total number of pads = " << updatedpads << " / " << totnumpads << std::endl;
    }
    //std::cout << "-------------------------------------------------end update" << std::endl;
  }
  virtual void OnUnrecognised(const std::string & cmd, const std::string & param) {
    std::cout << "Unrecognised: (" << cmd.length() << ") " << cmd;
    if (param.length() > 0) std::cout << " (" << param << ")";
    std::cout << std::endl;
    SetStatus(eudaq::Status::LVL_WARN, "Just testing");
  }

private:
  struct BoardDisplay {
    bool islog;
    //counted_ptr<TGCompositeFrame> m_frame;
    counted_ptr<TRootEmbeddedCanvas> m_embedded;
    TCanvas * m_canvas;
    counted_ptr<TH2D> m_historaw2d, m_tempcds, m_tempcds2, m_histocds2d, m_histohit2d,
      m_histocluster2d, m_histotrack2d, m_histonoise2d, m_testhisto;
    counted_ptr<TH1D> m_historawx, m_historawy, m_historawval, m_histocdsval, m_histonoise, m_histonoiseeventnr,
      m_histoclusterx, m_histoclustery, m_histoclusterval, m_histonumclusters,
      m_histodeltax, m_histodeltay, m_histonumhits, rmshisto;
    std::vector<double> m_clusters, m_clusterx, m_clustery;
    double m_trackx, m_tracky;
    void Reset() {
      m_testhisto->Reset();
      m_historaw2d->Reset();
      m_tempcds->Reset();
      m_tempcds2->Reset();
      m_histocds2d->Reset();
      m_histohit2d->Reset();
      m_histocluster2d->Reset();
      m_histotrack2d->Reset();
      m_histonoise2d->Reset();
      m_historawx->Reset();
      m_historawy->Reset();
      m_historawval->Reset();
      m_histocdsval->Reset();
      m_histonoise->Reset();
      rmshisto->Reset();
      
      m_histonoiseeventnr->Reset();
      
      m_histoclusterx->Reset();
      m_histoclustery->Reset();
      m_histoclusterval->Reset();
      m_histonumclusters->Reset();
      m_histodeltax->Reset();
      m_histodeltay->Reset();
      m_histonumhits->Reset();
    }
  };

  void BookBoard(int board, BoardDisplay & b) {
    //allocate some memory for the cluster vectors
    b.m_clusters.reserve(50);
    b.m_clusterx.reserve(50);
    b.m_clustery.reserve(50);

    b.islog = false;
    //std::string name = "Board " + eudaq::to_string(board);
    TGCompositeFrame * frame = m_tabs->AddTab(make_name("Board ", board).c_str());
    b.m_embedded = new TRootEmbeddedCanvas(make_name("Canvas", board).c_str(), frame, 100, 100);
    frame->AddFrame(b.m_embedded.get(), m_hintbig.get());
    b.m_canvas = b.m_embedded->GetCanvas();
    //b.m_canvas->Divide(1, 1);

    b.rmshisto = new TH1DNew(make_name("rms histo",board).c_str(),"rms histo", 40, -50, 50);
    b.m_historaw2d      = new TH2DNew(make_name("RawProfile",    board).c_str(), "Raw 2D Profile",    264, 0, 264, 256, 0, 256);
    b.m_tempcds         = new TH2DNew(make_name("TempCDS",       board).c_str(), "Temp CDS",          264, 0, 264, 256, 0, 256);
    b.m_tempcds2        = new TH2DNew(make_name("TempCDS2",      board).c_str(), "Temp CDS2",         264, 0, 264, 256, 0, 256);
    b.m_histocds2d      = new TH2DNew(make_name("CDSProfile",    board).c_str(), "CDS Profile",       264, 0, 264, 256, 0, 256);
    b.m_histohit2d      = new TH2DNew(make_name("HitMap",        board).c_str(), "Hit Profile",       264, 0, 264, 256, 0, 256);
    b.m_histocluster2d  = new TH2DNew(make_name("ClusterMap",    board).c_str(), "Cluster Profile",   132, 0, 264, 128, 0, 256);
    b.m_histotrack2d    = new TH2DNew(make_name("TrackMap",      board).c_str(), "Track Candidates",  132, 0, 264, 128, 0, 256);
    b.m_histonoise2d    = new TH2DNew(make_name("NoiseMap",      board).c_str(), "Noise Profile",     264, 0, 264, 256, 0, 256);
    b.m_historawx       = new TH1DNew(make_name("RawXProfile",   board).c_str(), "Raw X Profile",     264, 0, 264);
    b.m_historawy       = new TH1DNew(make_name("RawYProfile",   board).c_str(), "Raw Y Profile",     256, 0, 256);
    b.m_histoclusterx   = new TH1DNew(make_name("ClustXProfile", board).c_str(), "Cluster X Profile", 264, 0, 264);
    b.m_histoclustery   = new TH1DNew(make_name("ClustYProfile", board).c_str(), "Cluster Y Profile", 256, 0, 256);
    b.m_historawval     = new TH1DNew(make_name("RawValues",     board).c_str(), "Raw Values",        512, 0, 4096);
    b.m_histocdsval     = new TH1DNew(make_name("CDSValues",     board).c_str(), "CDS Values",        150, -50, 100);
    b.m_histonoise     = new TH1DNew(make_name("Noise",     board).c_str(), "Noise",        40, 0, 20);
    b.m_histonoiseeventnr     = new TH1DNew(make_name("NoiseEventNr",     board).c_str(), "NoiseEventNr",        30, 0, 1500);
    
    b.m_histoclusterval = new TH1DNew(make_name("ClusterValues", board).c_str(), "Cluster Charge",    500,  0, 1000);
    b.m_histonumclusters= new TH1DNew(make_name("NumClusters",   board).c_str(), "Num Clusters",      100,  0,  100);
    b.m_histodeltax     = new TH1DNew(make_name("DeltaX",        board).c_str(), "Delta X",           200,-100, 100);
    b.m_histodeltay     = new TH1DNew(make_name("DeltaY",        board).c_str(), "Delta Y",           200,-100, 100);
    b.m_histonumhits    = new TH1DNew(make_name("NumSeeds",      board).c_str(), "Num Seeds",         100,   0, 100);
    b.m_histocds2d->Sumw2();

    b.m_testhisto = new TH2DNew(make_name("CDSLego",    board).c_str(), "CDS Lego",       264, 0, 264, 256, 0, 256);


  }
  void FillBoard(BoardDisplay & b, eudaq::EUDRBBoard & e, int boardnumber, unsigned int &numberofclusters, std::vector<double> & clusterposition,  std::vector<double> & clusterpositiony) {
    eudaq::EUDRBDecoder::arrays_t<double, double> a = m_decoder->GetArrays<double, double>(e);
    size_t npixels = m_decoder->NumPixels(e); //, nx=264, ny=256;
    //std::cout << "Filling " << e.LocalEventNumber() << " board" << boardnumber
    //          << " frames " << m_decoder->NumFrames(e) << " pixels " << npixels << std::endl;
    std::vector<double> ones(npixels, 1.0);
    std::vector<double> cds(a.m_adc[0]);
    if (m_decoder->NumFrames(e) > 1) {
      if (m_decoder->NumFrames(e) == 3) {
        for (size_t i = 0; i < cds.size(); ++i) {
          cds[i] = a.m_adc[0][i] * (a.m_pivot[i])
            + a.m_adc[1][i] * (1-2*a.m_pivot[i])
            + a.m_adc[2][i] * (a.m_pivot[i]-1);
        }
      } else if (m_decoder->NumFrames(e) == 2) {
        for (size_t i = 0; i < cds.size(); ++i) {
          cds[i] = a.m_adc[0][i] - a.m_adc[1][i];
        }
      } else {
        for (size_t i = 0; i < cds.size(); ++i) {
          cds[i] = 100;
        }
      }

      {
	//rms for noise determination
	b.rmshisto->FillN(npixels, &cds[0], &ones[0]);

	double rms = b.rmshisto->GetRMS();
	b.m_histonoise->Fill(rms);
	
	int te = ((totalnumevents-1) % (int)50);
	if(te == 0)
	  {
	    double  kom = (totalnumevents-1)/50;
	    int t = (int) kom;
	    
	    TF1 *f1 = new TF1("bla","gaus");

	    b.rmshisto->Fit(f1,"Q0","");
	    Double_t sigma = f1->GetParameter(2);
	    b.m_histonoiseeventnr->SetBinContent((t+1),sigma);
	    b.m_histonoiseeventnr->SetBinError((t+1),0.0);
	    
	    for(int h = t+2; h <= t+4 && h <= 30; h++)
	      {
		b.m_histonoiseeventnr->SetBinContent(h,0.0);
		b.m_histonoiseeventnr->SetBinError(h,0.0);
	      }
	    delete f1;
	    b.rmshisto->Reset();
	  }
	//end of rms for noise determination
      }

      b.m_historaw2d->FillN(npixels, &a.m_x[0], &a.m_y[0], &a.m_adc[1][0]);
      b.m_historaw2d->SetNormFactor(b.m_historaw2d->Integral() / m_histoevents);
      b.m_historawx->FillN(npixels, &a.m_x[0], &a.m_adc[1][0]);
      b.m_historawx->SetNormFactor(b.m_historawx->Integral() / m_histoevents);
      b.m_historawy->FillN(npixels, &a.m_y[0], &a.m_adc[1][0]);
      b.m_historawy->SetNormFactor(b.m_historawy->Integral() / m_histoevents);
      b.m_historawval->FillN(npixels, &a.m_adc[1][0], &ones[0]);
      //b.m_historawval->SetNormFactor(b.m_historawval->Integral() / m_histoevents);
    }
    b.m_histocdsval->FillN(npixels, &cds[0], &ones[0]);
    //b.m_histocdsval->SetNormFactor(b.m_histocdsval->Integral() / m_histoevents);
    std::vector<double> newx(a.m_x); // TODO: shouldn't need to recalculate this for each event
//     for (int i = 0; i < cds.size(); ++i) {
//       int mat = a.m_x[i] / 66, col = (int)a.m_x[i] % 66;
//       if (col >= 2) {
//         newx[i] = mat*64 + col - 2;
//       } else {
//         newx[i] = -1;
//         cds[i] = 0;
//       }
//     }
    b.m_tempcds->Reset();
    b.m_tempcds->FillN(npixels, &newx[0], &a.m_y[0], &cds[0]);

    b.m_tempcds2->Reset();
    b.m_tempcds2->FillN(npixels, &newx[0], &a.m_y[0], &cds[0]);

    
    
    
    if((totalnumevents % (int)conf.UPDATE_EVERY_N_EVENTS) == 0)
      {
        b.m_testhisto->Reset();
        TString tmpstring;
        char tmpstring2[50];
        sprintf(tmpstring2, "Board %1.0f, event: %1.0i", (float)boardnumber, totalnumevents);
        tmpstring = tmpstring2;
        b.m_testhisto->SetTitle(tmpstring);
        b.m_testhisto->FillN(npixels, &newx[0], &a.m_y[0], &cds[0]);
      }
    

    b.m_histocds2d->FillN(npixels, &newx[0], &a.m_y[0], &cds[0]);
    b.m_clusters.clear();
    b.m_clusterx.clear();
    b.m_clustery.clear();
    if (m_histoevents >= 20) {
      if (m_histoevents < 500) {
        b.m_histonoise2d->Reset();
        for (int iy = 1; iy <= b.m_tempcds->GetNbinsY(); ++iy) {
          for (int ix = 1; ix <= b.m_tempcds->GetNbinsX(); ++ix) {
            double rms = b.m_histocds2d->GetBinError(ix, iy) / std::sqrt((double)m_histoevents);
            b.m_histonoise2d->Fill(ix-1, iy-1, rms);
            //if (ix < 5 && iy < 5) std::cout << ix << ", " << iy << " rms = " << rms << " bin = " << bin << std::endl;
          }
        }
      }


      std::vector<Seed> seeds;
      seeds.reserve(20); // allocate memory for 20 seed pixels
      const double seed_thresh = conf.SEED_THRESHOLD /* sigma */ , cluster_thresh = conf.CLUSTER_THRESHOLD /* sigma */, seedneighbour_thresh = conf.SEED_NEIGHBOUR_THRESHOLD;
 
      for (int iy = 1; iy <= b.m_tempcds->GetNbinsY(); ++iy) {
        for (int ix = 1; ix <= b.m_tempcds->GetNbinsX(); ++ix) {
          double s = b.m_tempcds->GetBinContent(ix, iy);
          double noise = DEFAULT_NOISE; //b.m_histonoise2d->GetBinContent(ix, iy);
          if (s > seed_thresh*noise) {
            seeds.push_back(Seed(ix, iy, s));
          }
        }
      }
      
      //construct the cluster 
      if (seeds.size() < MAX_SEEDS) {
        std::sort(seeds.begin(), seeds.end(), &Seed::compare);
        for (size_t i = 0; i < seeds.size(); ++i) {
          int clustersizeindex = 1;
          //chose the cluster type. so far 3x3 or 5x5
          if (conf.CLUSTER_TYPE == 3)
            clustersizeindex = 1;
          if (conf.CLUSTER_TYPE == 5)
            clustersizeindex = 2;
   
          if (b.m_tempcds->GetBinContent((int)seeds[i].x, (int)seeds[i].y) > 0) {
            double cluster = 0;
            double noise = 0;
            for (int dy = -clustersizeindex; dy <= clustersizeindex; ++dy) {
              for (int dx = -clustersizeindex; dx <= clustersizeindex; ++dx) {
                if((seeds[i].x+dx) >= 0 && (seeds[i].y+dy) >= 0 && (seeds[i].x+dx) < b.m_tempcds->GetXaxis()->GetLast()
                   && (seeds[i].y+dy) < b.m_tempcds->GetYaxis()->GetLast() //check whether we are inside the histogram
                   && b.m_tempcds->GetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy) > seedneighbour_thresh * DEFAULT_NOISE)
                  {
                    noise += DEFAULT_NOISE*DEFAULT_NOISE;
                    cluster += b.m_tempcds->GetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy);
                    b.m_tempcds->SetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy, 0);
                  }
              }
            }
            noise = std::sqrt(noise);
            if (cluster > cluster_thresh*noise) { //cut on the cluster charge
              if (conf.CLUSTER_POSITION == 1)
                {
                  //compute the center of gravity
                  int array_size=3;
                  if (conf.CLUSTER_TYPE == 3)
                    array_size=3;
                  if (conf.CLUSTER_TYPE == 5)
                    array_size=5;
                  std::vector<double> sumx(array_size,0.0);
                  std::vector<double> sumy(array_size,0.0);
   
                  for(int dx =-clustersizeindex ; dx<=clustersizeindex;dx++)
                    {
                      for(int dy = -clustersizeindex; dy<=clustersizeindex;dy++)
                        {
                          double noise = 5.0;
                          double value = b.m_tempcds2->GetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy);
                          if((seeds[i].x+dx) >= 0 && (seeds[i].y+dy) >= 0 && (seeds[i].x+dx) < b.m_tempcds2->GetXaxis()->GetLast()
                             && (seeds[i].y+dy) < b.m_tempcds2->GetYaxis()->GetLast() //check whether we are inside the histogram
                             && value > seedneighbour_thresh*noise) // cut on s/n of the seed pixels neighbours
                            {
                              sumy[dy+1] += value;
                              sumx[dx+1] += value;
                            }
                        }
                    }
                  double x = 0.0;
                  double y = 0.0;
                  double sumweight_x = 0.0;
                  double sumweight_y = 0.0;
       
                  for(int u = -clustersizeindex; u <= clustersizeindex;u++)
                    {
                      x += ( seeds[i].x + u ) * sumx[u+1];
                      y += ( seeds[i].y + u ) * sumy[u+1];
                      sumweight_x += sumx[u+1];
                      sumweight_y += sumy[u+1];
                    }
                  double_t cluster_x = x / sumweight_x;
                  double_t cluster_y = y / sumweight_y;
       
                  b.m_clusterx.push_back(cluster_x);
                  b.m_clustery.push_back(cluster_y);
                  //end of center of gravity
                }
              else
                {
                  // seed position
                  b.m_clusterx.push_back(seeds[i].x);
                  b.m_clustery.push_back(seeds[i].y);
                }
              b.m_clusters.push_back(cluster);
       
            }
          }
        }
        /*if (b.m_clusters.size())*/ b.m_histonumclusters->Fill(b.m_clusters.size());
        numberofclusters = b.m_clusters.size();

	clusterposition = b.m_clusterx;
	clusterpositiony = b.m_clustery;
	
	b.m_histohit2d->Reset();
        b.m_histohit2d->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histocluster2d->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histoclusterx->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clusters[0]);
        b.m_histoclusterx->SetNormFactor(b.m_histoclusterx->Integral() / m_histoevents);
        b.m_histoclustery->FillN(b.m_clusters.size(), &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histoclustery->SetNormFactor(b.m_histoclustery->Integral() / m_histoevents);
        b.m_histoclusterval->FillN(b.m_clusters.size(), &b.m_clusters[0], &ones[0]);
      }
      //if (m_decoder->NumFrames(e) > 1) {
      npixels = seeds.size();
      //}
      /*if (npixels)*/ b.m_histonumhits->Fill(npixels);
    }
    if (!b.islog && m_histoevents > 100) {
      b.islog = true;
      //b.m_canvas->cd(3)->SetLogy();
      //b.m_canvas->cd(7)->SetLogy();
      //b.m_canvas->cd(11)->SetLogy();
      //m_canvasmain->cd(board + m_board.size() + 1)->SetLogy();
    }
  }

  eudaq::Time m_lastupdate;
  bool m_modified, m_runended;
  unsigned long long m_prevt;
  unsigned long m_histoevents; // count of histogrammed events for normalization factor
  counted_ptr<eudaq::EUDRBDecoder> m_decoder;
  std::vector<TColor*> m_colours;

  // Layout hints
  counted_ptr<TGLayoutHints> m_hinttop;
  counted_ptr<TGLayoutHints> m_hintleft;
  counted_ptr<TGLayoutHints> m_hintbig;
  counted_ptr<TGLayoutHints> m_hint_l;
  counted_ptr<TGLayoutHints> m_hint_test;
  
  // Toolbar
  counted_ptr<TGCompositeFrame> m_toolbar;
  counted_ptr<TGLabel>          m_tb_filename;
  counted_ptr<TGLabel>          clustertypelabel;
  counted_ptr<TGLabel> numbinshitcorr;

  counted_ptr<TGLabel>          clusterpositionlabel;
  counted_ptr<TGComboBox> clusterpositionComboBox;
  counted_ptr<TGComboBox> clustertypeComboBox;
  counted_ptr<TGLabel>          m_tb_runnum;
  counted_ptr<TGLabel>          m_tb_evtnum;
  counted_ptr<TGNumberEntry>    m_tb_reduce;
  counted_ptr<TGNumberEntry>    m_tb_update;

  // Tabs
  counted_ptr<TGTab> m_tabs;
  //configuration tab
  counted_ptr<TGCompositeFrame> m_conf_tab;
  counted_ptr<TGCompositeFrame> m_cds_lego_tab;
  counted_ptr<TGGroupFrame>m_conf_group_frame;
  counted_ptr<TGGroupFrame>m_conf_group_frame_boarddisplay;
  counted_ptr<TGGroupFrame>m_conf_group_frame_main;
  counted_ptr<TGGroupFrame>m_conf_group_frame_cdslego;
  counted_ptr<TGGroupFrame>m_conf_group_frame_board;

  counted_ptr<TGLabel> updatecdslabel;
  counted_ptr<TGLabel> seedthresholdlabel;
  counted_ptr<TGLabel> seedneighbourthresholdlabel;
  counted_ptr<TGLabel> clusterthresholdlabel;
  
  counted_ptr<TGNumberEntry> m_conf_cds_lego_update;
 counted_ptr<TGNumberEntry> m_conf_numbinshitcorr;

  counted_ptr<TGNumberEntry> m_conf_seedthreshold;
  counted_ptr<TGNumberEntry> m_conf_seedneighbourthreshold;
  
  counted_ptr<TGNumberEntry> m_conf_clusterthreshold;
  
  counted_ptr<TGTextButton> m_conf_apply;
  counted_ptr<TGTextButton> m_reset_histos;
  //checkboxes
  counted_ptr<TGCheckButton> m_conf_checkbox_main_numclusters;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_hitcorr;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_clustercorr;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_clustercorry;
  
  counted_ptr<TGCheckButton> m_conf_checkbox_main_rawval;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_cluster2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_deltax;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_deltay;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_numhits;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_cdsval;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_noise;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_noiseeventnr;
  
  counted_ptr<TGCheckButton> m_conf_checkbox_main_track2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_clusterval;
  counted_ptr<TGCheckButton> m_conf_checkbox_main_numtracks;
  
  counted_ptr<TGCheckButton> m_conf_checkbox_cdslego;
  
  counted_ptr<TGCheckButton> m_conf_checkbox_clusterx;
  counted_ptr<TGCheckButton> m_conf_checkbox_clustery;
  counted_ptr<TGCheckButton> m_conf_checkbox_raw2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_cds2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_cluster2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_rawval;
  counted_ptr<TGCheckButton> m_conf_checkbox_noise2d;
  counted_ptr<TGCheckButton> m_conf_checkbox_rawx;
  counted_ptr<TGCheckButton> m_conf_checkbox_rawy;
  counted_ptr<TGCheckButton> m_conf_checkbox_cdsval;
  counted_ptr<TGCheckButton> m_conf_checkbox_noise;
  counted_ptr<TGCheckButton> m_conf_checkbox_noiseeventnr;
  
  counted_ptr<TGCheckButton> m_conf_checkbox_numhits;
  counted_ptr<TGCheckButton> m_conf_checkbox_clusterval;
  counted_ptr<TGCheckButton> m_conf_checkbox_numclusters;

  //end of checkboxes
  // Main tab
  //TGCompositeFrame * m_framemain;
  TRootEmbeddedCanvas * m_embedmain;
  TRootEmbeddedCanvas * m_embedcdslego;
  TCanvas * m_canvasmain;
  TCanvas *m_cds_lego_canvas;

  counted_ptr<TH1DNew> m_histonumtracks;
  std::vector< TH2DNew* > m_hitcorrelation;
  std::vector< TH2DNew* > m_clustercorrelation;
  std::vector< TH2DNew* > m_clustercorrelationy;

  // Board tabs (1 per board)
  std::vector<BoardDisplay> m_board;


  //histogram pads
  std::vector<histopad> main_pads;
  std::vector<histopad> cdslego_pads;
  std::vector<std::vector<histopad> > board_pads;

  int m_cluster_size;
  //TTimer * m_timer;
  //TProcessEventTimer * m_processtimer;

  ConfigurationClass conf; //object, that holds the configuration

  //counter
  int totalnumevents;
};

int main(int argc, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Root Monitor", "1.0", "A Monitor using root for gui and graphics");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:7000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> file (op, "f", "data-file", "", "filename",
                                   "A data file to load - setting this changes the default"
                                   " run control address to 'null://'");
  eudaq::Option<int>             x(op, "x", "left",    100, "pos");
  eudaq::Option<int>             y(op, "y", "top",       0, "pos");
  eudaq::Option<int>             w(op, "w", "width",  1400, "pos");
  eudaq::Option<int>             h(op, "g", "height",  700, "pos", "The initial position of the window");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    if (file.IsSet() && !rctrl.IsSet()) rctrl.SetValue("null://");
    gROOT->Reset();
    gROOT->SetStyle("Plain");
    gStyle->SetPalette(1);
    gStyle->SetOptStat(1000010);
    
    RootMonitor mon(rctrl.Value(), file.Value(), x.Value(), y.Value(), w.Value(), h.Value(), argc, argv);
    mon.Run();
  } catch (...) {
    std::cout << "RootMonitor exception handler" << std::endl;
    return op.HandleMainException();
  }
  return 0;
}

