// ROOT includes
#include <TH2F.h>
#include <TCanvas.h>
#include <TGPicture.h>
#include <TGLabel.h>
#include <TGMsgBox.h>

#include <TApplication.h>

// C++ includes
#include <iostream>
#include <sstream>
#include "OnlineMonWindow.hh"

#include "eudaq/Config.hh"

// File IO includes
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode)&S_IFMT) == S_IFREG)
#endif

using namespace std;

// the constructor
OnlineMonWindow::OnlineMonWindow(const TGWindow *p, UInt_t w, UInt_t h)
    : TGMainFrame(p, w, h, kVerticalFrame), _eventnum(0), _runnum(0),
      _analysedEvents(0) {

  // init snapshot counter
  snapshot_sequence = 0;
  _reduce = 1; // set a default value;
  Hfrm_windows = new TGHorizontalFrame(this);
  Hfrm_left = new TGVerticalFrame(Hfrm_windows);
  // counter for toolbar
  int i = 1;
  // directory root for Icons
  string icondir_root(PACKAGE_SOURCE_DIR);
  icondir_root += "/monitors/onlinemon/icons/";
  // build tool bar
  tb = new TGToolBar(Hfrm_left, 180, 80);
  if (tb != NULL) {
    /*------*/
    string path_save = icondir_root + "save_btn.xpm";
    ToolBarData_t icondata_save = {path_save.c_str(),
                                   "Save histograms to rootfile", kFALSE, i, 0};
    i++;
    button_save = tb->AddButton(Hfrm_left, &icondata_save, 0);
    button_save->Connect("Clicked()", "OnlineMonWindow", this, "Write()");

    /*------*/
    string path_delete = icondir_root + "delete_btn.xpm";
    ToolBarData_t icondata_delete = {path_delete.c_str(),
                                     "Reset all Histograms", kFALSE, i, 0};
    i++;
    button_reset = tb->AddButton(Hfrm_left, &icondata_delete, 5);
    button_reset->Connect("Clicked()", "OnlineMonWindow", this, "Reset()");

    /*------*/
    string path_autoreset = icondir_root + "autodel_btn.xpm";
    ToolBarData_t icondata_autoreset = {path_autoreset.c_str(),
                                        "Autoreset Histograms on StartRun",
                                        kTRUE, i, 0};
    i++;
    button_autoreset = tb->AddButton(Hfrm_left, &icondata_autoreset, 0);
    button_autoreset->Connect("Clicked()", "OnlineMonWindow", this,
                              "AutoReset()");

    /*------*/
    string path_snapshot = icondir_root + "eve_viewer.xpm";
    ToolBarData_t icondata_snapshot = {path_snapshot.c_str(),
                                       "Make Canvas Snapshot", kFALSE, i, 0};
    i++;
    button_snapshot = tb->AddButton(Hfrm_left, &icondata_snapshot, 10);
    button_snapshot->Connect("Clicked()", "OnlineMonWindow", this,
                             "SnapShot()");

    /*------*/
    string path_quit = icondir_root + "quit_btn.xpm";
    ToolBarData_t icondata_quit = {path_quit.c_str(), "Quit", kFALSE, i, 0};
    i++;
    button_quit = tb->AddButton(Hfrm_left, &icondata_quit, 5);
    button_quit->Connect("Clicked()", "OnlineMonWindow", this, "Quit()");

    /*------*/
    string path_about = icondir_root + "about.xpm";
    ToolBarData_t icondata_about = {path_about.c_str(), "About", kFALSE, i, 0};
    i++;
    button_about = tb->AddButton(Hfrm_left, &icondata_about, 10);
    button_about->Connect("Clicked()", "OnlineMonWindow", this, "About()");
  }
  /*------*/
  nen_reduce =
      new TGNumberEntry(Hfrm_left, 1, 3, -1, TGNumberFormat::kNESInteger,
                        TGNumberFormat::kNEAPositive);

  tb->AddFrame(nen_reduce,
               new TGLayoutHints(kLHintsTop | kLHintsLeft, 17, 0, 7, 2));
  nen_reduce->Connect("ValueSet(Long_t)", "OnlineMonWindow", this,
                      "ChangeReduce(Long_t)");

  Cvs_left = new TGCanvas(Hfrm_left, 200, h);
  TGViewPort *vp = Cvs_left->GetViewPort();

  LTr_left = new TGListTree(Cvs_left, kHorizontalFrame);
  LTr_left->Connect("Clicked(TGListTreeItem*, Int_t)", "OnlineMonWindow", this,
                    "actor(TGListTreeItem*, Int_t)");
  LTr_left->Connect("Clicked(TGListTreeItem*, Int_t, Int_t, Int_t)",
                    "OnlineMonWindow", this,
                    "actorMenu(TGListTreeItem*, Int_t, Int_t, Int_t)");

  vp->AddFrame(LTr_left,
               new TGLayoutHints(kLHintsExpandY | kLHintsExpandY, 5, 5, 5, 5));
  // LTr_left->MapSubwindows();
  Hfrm_left->AddFrame(tb, new TGLayoutHints(kLHintsExpandX, 5, 5, 5, 5));
  Hfrm_left->AddFrame(
      Cvs_left, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
  // Hfrm_left->AddFrame(LTr_left,new TGLayoutHints(kLHintsExpandX |
  // kLHintsExpandY,5,5,5,5));
  Hfrm_right = new TGVerticalFrame(Hfrm_windows);
  ECvs_right = new TRootEmbeddedCanvas("Canvas", Hfrm_right);
  Hfrm_right->AddFrame(
      ECvs_right,
      new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 5, 5, 5, 5));
  // ECvs_right->GetCanvas()->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
  //                                  "OnlineMonWindow", this,
  //                                  "ExecuteEvent(Int_t,Int_t,Int_t,TObject*)");

  Hfrm_windows->AddFrame(Hfrm_left,
                         new TGLayoutHints(kLHintsExpandY, 2, 2, 2, 2));
  Hfrm_windows->AddFrame(
      Hfrm_right,
      new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 2, 2, 2, 2));
  AddFrame(Hfrm_windows,
           new TGLayoutHints(kLHintsExpandY | kLHintsExpandX | kLHintsLeft, 0,
                             0, 0, 0));
  fStatusBar = new TGStatusBar(this, 510, 10, kHorizontalFrame);
  int parts[] = {25, 25, 25, 25};
  fStatusBar->SetParts(parts, 4);
  fStatusBar->SetText("IDLE", 0);
  fStatusBar->SetText("run: 0", 1);
  fStatusBar->SetText("Curr. event: ", 2);
  fStatusBar->SetText("Analysed events: ", 3);

  AddFrame(fStatusBar,
           new TGLayoutHints(kLHintsBottom | kLHintsExpandX, 0, 0, 2, 0));

  timer = new TTimer();
  timer->Connect("Timeout()", "OnlineMonWindow", this, "autoUpdate()");
  timer->Start(1000, kFALSE);
  _reduceUpdate = 0;

  h1 = new TH2F("h1", "hu h1", 11, 0, 10, 11, 0, 10);
  h2 = new TH2F("h2", "hu h2", 11, 0, 10, 11, 0, 10);
  // a->Draw();
  std::string name = "EUDAQ Online-Monitor ";
  name += PACKAGE_VERSION;
  SetWindowName(name.data());
  MapSubwindows();
  // Resize(GetDefaultSize());
  Resize(w, h);
  Layout();
  MapSubwindows();
  MapWindow();

  CtxMenu = new TContextMenu("", "");
}
// Execute action corresponding to an event at (px,py). This method
// must be overridden if an object can react to graphics events. From ROOT doc

void OnlineMonWindow::ExecuteEvent(Int_t event, Int_t /*px*/, Int_t /*py*/,
                                   TObject *sel) {
  if ((event == kButton1Down) &&
      (strstr(sel->ClassName(), "TH") !=
       NULL)) // only do this, if a histogramme has been clicked
  {
    _activeHistos.clear();
    // ECvs_right->GetCanvas()->BlockAllSignals(1);
    ECvs_right->GetCanvas()->Clear();
    ECvs_right->GetCanvas()->cd();
    sel->Draw("COLZ");
    ECvs_right->GetCanvas()->Update();
    MapSubwindows();
    MapWindow();
  }
}

void OnlineMonWindow::Write() {
  TFile *f = new TFile(_rootfilename.c_str(), "RECREATE");
  if (f != NULL) {
    for (unsigned int i = 0; i < _colls.size(); ++i) {
      _colls.at(i)->Write(f);
    }
    f->Close();
  } else {
    cerr << "Can't open root file" << endl;
  }
}

void OnlineMonWindow::Reset() {
  UpdateStatus("Resetting..");
  for (unsigned int i = 0; i < _colls.size(); ++i) {
    _colls.at(i)->Reset();
  }
  _analysedEvents = 0;
}

void OnlineMonWindow::AutoReset() {
  _autoreset = button_autoreset->IsOn();
}

void OnlineMonWindow::Quit() { gApplication->Terminate(0); }

void OnlineMonWindow::SnapShot() {
#ifdef DEBUG
  cout << "SnapShot pressed" << endl;
  cout << (rmon->mon_configdata).getSnapShotDir() << endl;
  cout << (rmon->mon_configdata).getSnapShotFormat() << endl;
#endif

  TCanvas *c1 = ECvs_right->GetCanvas();
  if (c1 != NULL) {
    // do the string conversion
    stringstream buffer_runnum;
    stringstream buffer_seqnum;
    buffer_runnum << _runnum;
    buffer_seqnum << snapshot_sequence;

    string filename = rmon->mon_configdata.getSnapShotDir() + "Snapshot_" +
                      buffer_runnum.str() + "_" + buffer_seqnum.str() +
                      rmon->mon_configdata.getSnapShotFormat();

    cout << "Trying to Save to " << filename << endl;
    // as ROOT doesn't tell you if it was successfully creating the file we have
    // to use a POSIX call
    struct stat dirbuf;
    if (stat(rmon->mon_configdata.getSnapShotDir().c_str(), &dirbuf) == 0) {
      if (S_ISDIR(dirbuf.st_mode)) {
        c1->SaveAs(filename.c_str());
      }
    } else {
      cout << "Error accessing snapshot directory "
           << rmon->mon_configdata.getSnapShotDir() << endl;
    }

    snapshot_sequence++;
  }
}
void OnlineMonWindow::Print() { cout << "Print not implemented yet" << endl; }

void OnlineMonWindow::About() {
  TGMsgBox *t1 = new TGMsgBox(
      gClient->GetRoot(), this, "About Onlinemon",
      "OnlineMonitor for EUDET/AIDA Telescopes\nVersion 1.0Beta4\n",
      kMBIconAsterisk, kMBDismiss, 0, kVerticalFrame,
      kTextCenterX | kTextCenterY);
}

void OnlineMonWindow::registerTreeItem(std::string item) {
  if (item.find("/") == std::string::npos) { // Yes
    _treeMap[item] = LTr_left->AddItem(NULL, item.c_str());
    _treeBackMap[_treeMap[item]] = item;
  } else { // No
    char post[1024] = "";
    char pre[1024] = "";
    item.copy(pre, item.find_last_of("/"), 0);
    item.copy(post, item.length() - item.find_last_of("/"),
              item.find_last_of("/") + 1);
    if (_treeMap[string(pre)] == NULL)
      registerTreeItem(string(pre));
    _treeMap[item] = LTr_left->AddItem(_treeMap[string(pre)], post);
    _treeBackMap[_treeMap[item]] = item;
  }
  Cvs_left->MapSubwindows();
  Cvs_left->MapWindow();
}

void OnlineMonWindow::makeTreeItemSummary(std::string item) {
  std::map<std::string, TNamed *>::iterator it;
  std::vector<std::string> v;
  for (it = _hitmapMap.begin(); it != _hitmapMap.end(); ++it) {
    std::string c = std::string(it->first, 0, item.length());
    if (c == item) {
      v.push_back(it->first);
    }
  }
  _summaryMap[item] = v;
}

void OnlineMonWindow::addTreeItemSummary(std::string item,
                                         std::string histoitem) {

  std::vector<std::string> v;
  std::map<std::string, std::string>::iterator it;
  if (_summaryMap.find(item) != _summaryMap.end()) {
    v = _summaryMap[item];
  }
  v.push_back(histoitem);
  _summaryMap[item] = v;
}

void OnlineMonWindow::registerHisto(std::string tree, TNamed *h, std::string op,
                                    const unsigned int l) {
  if (h == NULL) // check if valid histogram
  {
    cout << "OnlineMonWindow::registerHisto Null pointer for entry " << op
         << endl;
  }
  _hitmapMap[tree] = h;
  _hitmapOptions[tree] = op;
  _logScaleMap[tree] = l;
#ifdef DEBUG
  cout << "OnlineMonWindow::registerHisto Registering : " << h->GetName() << " "
       << l << " " << tree << " " << endl;
#endif
  if (op == "") {
    const TGPicture *thp = gClient->GetPicture("h2_t.xpm");
    _treeMap[tree]->SetPictures(thp, thp);
  } else {
    const TGPicture *thp = gClient->GetPicture("h3_t.xpm");
    _treeMap[tree]->SetPictures(thp, thp);
  }
}

void OnlineMonWindow::registerMutex(std::string tree, std::mutex *m){
  _mutexMap[tree] = m;
}

void OnlineMonWindow::autoUpdate() {
  _reduceUpdate++;
  unsigned int activeHistoSize = _activeHistos.size();
  if (activeHistoSize && _reduceUpdate > activeHistoSize){
    TCanvas *fCanvas = ECvs_right->GetCanvas();
    for (unsigned int i = 0; i < activeHistoSize; ++i) {
      if(activeHistoSize ==1){
	fCanvas->cd();
	fCanvas->Clear();
      }
      else{
	fCanvas->GetPad(i+1)->Clear();
	fCanvas->cd(i + 1);
      }
      std::string tree = _activeHistos.at(i);
      TNamed *hg = _hitmapMap[tree];
      if(hg) {
	TH1 *h = dynamic_cast<TH1 *> (hg);
	std::mutex mu_dummy;
	std::mutex *mu = &mu_dummy;
	auto it = _mutexMap.find(tree);
	if(it != _mutexMap.end())
	  mu=it->second;
	if(h){
	  std::lock_guard<std::mutex> lck(*mu);
	  h->Draw(_hitmapOptions[tree].c_str());
	  gPad->Update();
	}
	TGraph *g = dynamic_cast<TGraph *> (hg);
	if(g){
	  std::lock_guard<std::mutex> lck(*mu);
	  g->Draw(_hitmapOptions[tree].c_str());
	  gPad->Update();
	}
      }
    }
    UpdateEventNumber(_eventnum);
    UpdateRunNumber(_runnum);
    UpdateTotalEventNumber(_analysedEvents);

    MapSubwindows();
    MapWindow();
    _reduceUpdate = 0;
  }
}


void OnlineMonWindow::ChangeReduce(Long_t /*num*/) {
  _reduce = (unsigned int)(nen_reduce->GetNumber());
  for (unsigned int i = 0; i < _colls.size(); ++i) {
    _colls.at(i)->setReduce(_reduce);
  }
}

OnlineMonWindow::~OnlineMonWindow() { gApplication->Terminate(0); }

void OnlineMonWindow::actorMenu(TGListTreeItem * /*item*/, Int_t btn, Int_t x,
                                Int_t y) {

  if (btn == 3) {
    CtxMenu->Popup(x, y, this);
  }
}

void OnlineMonWindow::actor(TGListTreeItem *item, Int_t /*btn*/) {
  TCanvas *fCanvas = ECvs_right->GetCanvas();
  fCanvas->Clear();

  std::string tree = _treeBackMap[item];

  _activeHistos.clear();

  if (_hitmapMap.find(tree) != _hitmapMap.end()){
    _activeHistos.push_back(tree);
  }
  if (_summaryMap.find(tree) != _summaryMap.end()){
    std::vector<std::string> v = _summaryMap[tree];
    size_t s = v.size();
    for (unsigned int i = 0; i < s; ++i) {
      _activeHistos.push_back(v.at(i));
    }
    int d1 = 1;
    int d2 = 1;
    if (s == 2)
      d2 = 2;
    if (s > 2) {
      d1 = 2;
      d2 = 2;
    }
    if (s > 4) {
      d1 = 3;
      d2 = 2;
    }
    if (s > 6) {
      d1 = 3;
      d2 = 3;
    }
    if (s > 9) {
      d1 = 4;
      d2 = 3;
    }
    if (s > 12) {
      d1 = 4;
      d2 = 4;
    }
    if (s > 16) {
      d1 = 5;
      d2 = 4;
    }
    if (s > 20) {
      d1 = 5;
      d2 = 5;
    }
    if (s > 25) {
      d1 = 6;
      d2 = 5;
    }
    if (s > 30) {
      d1 = 6;
      d2 = 6;
    }
    if (s > 36) {
      d1 = 7;
      d2 = 6;
    }
    if (s > 42) {
      d1 = 7;
      d2 = 7;
    }
    fCanvas->Divide(d2, d1);
  }
  autoUpdate();
}

void OnlineMonWindow::registerPlane(char *sensor, int id) {
  cout << "Plane " << sensor << " " << id << "should be registered" << endl;
}

void OnlineMonWindow::UpdateRunNumber(const int num) {
  char out[1024];
  sprintf(out, "run: %u", num);
  fStatusBar->SetText(out, 1);
}

void OnlineMonWindow::UpdateEventNumber(const int event) {
  char out[1024] = "";
  sprintf(out, "Curr. event: %u", event);
  fStatusBar->SetText(out, 2);
}

void OnlineMonWindow::UpdateTotalEventNumber(const int num) {
  char out[1024] = "";
  sprintf(out, "Analysed events: %u", num);
  fStatusBar->SetText(out, 3);
}

void OnlineMonWindow::setReduce(const unsigned int red) {
  _reduce = red;
  nen_reduce->SetNumber(red);
}

void OnlineMonWindow::UpdateStatus(const std::string &status) {
  fStatusBar->SetText(status.c_str(), 0);
}

void OnlineMonWindow::setAutoReset(bool reset) {
  _autoreset = reset;
  button_autoreset->SetOn(_autoreset);
}

void OnlineMonWindow::setUpdate(const unsigned int up) {
  timer->Stop();
  timer->Start(up, kFALSE);
}

void OnlineMonWindow::SetOnlineMon(RootMonitor *mymon) {
  if (mymon != NULL) {
    rmon = mymon;
  } else {
    cout << "OnlineMonWindow::SetOnlineMon : Passing Null reference for "
            "RootMonitorObject, bailing out" << endl;
  }
}
