#ifndef ONLINE_MON_WINDOW_H
#define ONLINE_MON_WINDOW_H


#include <TTimer.h>
#include <TGFrame.h>
#include <TGLayout.h>
#include <TGWindow.h>
#include <TRootEmbeddedCanvas.h>
#include <TGListTree.h>
#include <RQ_OBJECT.h>
#include <TH1.h>
#include <TH2I.h>
#include <TContextMenu.h>
#include <TPad.h>
#include <TApplication.h>
#include <TGToolBar.h>
#include <TGButton.h>
#include <TGStatusBar.h>
#include <TGNumberEntry.h>

#include "BaseCollection.hh"
//#include "OnlineHistograms.hh"

#include <vector>
#include <map>


#ifndef __CINT__
#include "OnlineMon.hh"
//class RootMonitor;
#endif


static const unsigned int kLin = 0;
static const unsigned int kLogX = 1;
static const unsigned int kLogY = 2;
static const unsigned int kLogZ = 4;


class OnlineMonWindow : public TGMainFrame{
  RQ_OBJECT("OnlineMonWindow")
  protected:
    //#ifndef __CINT__
    //RootMonitor *_mon;
    //#endif
    std::string _rootfilename;
    std::vector<BaseCollection*> _colls;
    TTimer *timer;
    TGHorizontalFrame *Hfrm_windows;
    TGVerticalFrame *Hfrm_left;
    TGVerticalFrame *Hfrm_right;
    TGCanvas *Cvs_left;
    TRootEmbeddedCanvas *ECvs_right;
    TGListTree *LTr_left;
    std::vector<std::string> _activeHistos;

    TGToolBar *tb;
    TGButton *button_save;
    TGButton *button_reset;
    TGButton *button_autoreset;
    TGButton *button_quit;
    TGButton *button_snapshot;
    TGButton *button_about;
    TGStatusBar *fStatusBar;

    std::map<std::string,TGListTreeItem*> _treeMap;
    std::map<TGListTreeItem*, std::string> _treeBackMap;
    std::map<std::string, TH1*> _hitmapMap;
    std::map<std::string, std::vector<std::string> > _summaryMap;
    std::map<std::string,std::string> _hitmapOptions;
    std::map<std::string,unsigned int> _logScaleMap;
    TGListTreeItem * Itm_Eudet;
    TGListTreeItem * Itm_DUT;
    TGListTreeItem * Itm_EudetHM;
    TGListTreeItem * Itm_DUTHM;
    TGNumberEntry * nen_reduce;
    std::vector<TGListTreeItem*> Itm_EudetHM_Vec;


    TH2F *h1;
    TH2F *h2;
    unsigned int _eventnum, _runnum;
    bool _autoreset;
    unsigned int _reduce, _reduceUpdate;
    unsigned int _analysedEvents;

    TContextMenu *CtxMenu;

    RootMonitor *rmon;
    int snapshot_sequence;

  public:
    OnlineMonWindow(const TGWindow *p, UInt_t w, UInt_t h);
    //#ifndef __CINT__
    //void setRootMonitor(RootMonitor *mon) {_mon = mon;}
    //#endif
    void registerTreeItem(std::string);
    void makeTreeItemSummary(std::string);
    void addTreeItemSummary(std::string item, std::string histoitem);
    void registerHisto(std::string tree,TH1* h, std::string op = "", const unsigned int = kLin);
    void actor(TGListTreeItem* item, Int_t btn);
    void actorMenu(TGListTreeItem* item, Int_t btn, Int_t x, Int_t y);
    void registerPlane(char* sensor, int id);
    void autoUpdate();
    virtual ~OnlineMonWindow(); 
    void setCollections(std::vector<BaseCollection*> colls) {_colls = colls;}
    void Write();
    void Reset();
    void AutoReset();
    void ChangeReduce(Long_t num);
    void Quit();
    void SnapShot();
    void About();
    void Print();
    void setRootFileName(std::string rootfilename) {_rootfilename = rootfilename;}
    void UpdateRunNumber(const int num);
    void UpdateEventNumber(const int event);
    void UpdateStatus(const std::string & status);
    void UpdateTotalEventNumber(const int num);
    void setEventNumber(const int num) {_eventnum = num;}
    void setRunNumber(const int num) {_runnum = num; }
    void setAutoReset(bool reset);
    bool getAutoReset() const {return _autoreset; } 
    void setReduce(const unsigned int red);
    unsigned int getReduce() {return _reduce; }
    void setUpdate(const unsigned int up);
    void increaseAnalysedEventsCounter() {++_analysedEvents;}
    void ExecuteEvent(Int_t event, Int_t px, Int_t py, TObject *sel);

    void SetOnlineMon(RootMonitor *mymon);
    ClassDef(OnlineMonWindow,0)
};

#ifdef __CINT__
#pragma link C++ class OnlineMonWindow+;
#endif

#endif
