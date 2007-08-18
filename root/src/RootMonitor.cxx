#include "eudaq/Monitor.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/EUDRBEvent.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/OptionParser.hh"

#include "TROOT.h"
#include "TApplication.h"
#include "TGClient.h"
#include "TGMenu.h"
#include "TGTab.h"
#include "TGButton.h"
#include "TGLabel.h"
#include "TGTextEntry.h"
#include "TGNumberEntry.h"
#include "TStyle.h"
#include "TCanvas.h"
#include "TRootEmbeddedCanvas.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TThread.h"
#include "TFile.h"
//#include "TSystem.h" // for TProcessEventTimer
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

int colours[] = { kRed, kGreen, kBlue, kMagenta, kCyan,
                  kRed, kGreen, kBlue, kMagenta, kCyan };

struct Seed {
  double x, y, c;
  Seed(double x, double y, double c) : x(x), y(y), c(c) {}
  static bool compare(const Seed & lhs, const Seed & rhs) { return lhs.c > rhs.c; }
};

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

      m_toolbar(new TGHorizontalFrame(this, 800, 20, kFixedWidth)),
      m_tb_filename(new TGLabel(m_toolbar.get(), "                              ")),
      m_tb_runnum(new TGLabel(m_toolbar.get(), "0     ")),
      m_tb_evtnum(new TGLabel(m_toolbar.get(), "0          ")),
      m_tb_reduce(new TGNumberEntry(m_toolbar.get(), 1.0, 3)),
      m_tb_update(new TGNumberEntry(m_toolbar.get(), 5.0, 4)),
      m_tabs(new TGTab(this, 700, 500)),
      m_cluster_size(8)
      //m_timer(new TTimer(this, 500, kFALSE)),
      //m_processtimer(new TProcessEventTimer(100)),
    {

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

      AddFrame(m_toolbar.get(), m_hinttop.get());

      // Main tab
      TGCompositeFrame * frame = m_tabs->AddTab("Main");
      m_embedmain = new TRootEmbeddedCanvas("MainCanvas", frame, w, h);
      frame->AddFrame(m_embedmain, m_hintbig.get());
      m_canvasmain = m_embedmain->GetCanvas();

      m_board = std::vector<BoardDisplay>(5); // Maximum number of boards displayed
      m_canvasmain->Divide(4, 2);
      for (size_t i = 0; i < m_board.size(); ++i) {
        BookBoard(i+1, m_board[i]);
      }

      for (size_t i = 0; i < m_board.size(); ++i) {
        m_canvasmain->cd(1)->SetLogy();
        m_board[i].m_historawval->SetLineColor(colours[i]);
        m_board[i].m_historawval->Draw(i == 0 ? "" : "same");
        m_canvasmain->cd(2)->SetLogy();
        m_board[i].m_histocdsval->SetLineColor(colours[i]);
        m_board[i].m_histocdsval->Draw(i == 0 ? "" : "same");
        m_canvasmain->cd(3)->SetLogy();
        m_board[i].m_histoclusterval->SetLineColor(colours[i]);
        m_board[i].m_histoclusterval->Draw(i == 0 ? "" : "same");
        m_canvasmain->cd(4);
        m_board[i].m_histocluster2d->SetFillColor(colours[i]);
        m_board[i].m_histocluster2d->Draw(i == 0 ? "box" : "same box");
        if (i > 0) {
          m_canvasmain->cd(6);
          m_board[i].m_histodeltax->SetLineColor(colours[i]);
          m_board[i].m_histodeltax->Draw(i == 1 ? "" : "same");
          m_canvasmain->cd(7);
          m_board[i].m_histodeltay->SetLineColor(colours[i]);
          m_board[i].m_histodeltay->Draw(i == 1 ? "" : "same");
        }
        m_canvasmain->cd(8);
        m_board[i].m_histotrack2d->SetFillColor(colours[i]);
        m_board[i].m_histotrack2d->Draw(i == 0 ? "box" : "same box");
      }

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
    gApplication->Terminate();
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
      m_board[i].Reset();
    }
  }
  virtual void OnEvent(counted_ptr<eudaq::DetectorEvent> ev) {
    RootLocker lock;
    std::cout << *ev << std::endl;
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
      // Fill hostograms
      m_prevt = ev->GetTimestamp();

      unsigned numplanes = 0;

      if ((ev->GetEventNumber() % reduce) == 0) {
        m_tb_evtnum->SetText(eudaq::to_string(ev->GetEventNumber()).c_str());
        for (size_t i = 0; i < ev->NumEvents(); ++i) {
          if (eudaq::EUDRBEvent * drbev = dynamic_cast<eudaq::EUDRBEvent *>(ev->GetEvent(i))) {
            if (drbev->NumBoards() > 0) {
              m_histoevents++;
              try {
                for (size_t i = 0; i < drbev->NumBoards() && i < m_board.size(); ++i) {
                  numplanes++;
                  FillBoard(m_board[i], drbev->GetBoard(i));
                }
              } catch (const eudaq::Exception & e) {
                EUDAQ_ERROR("Bad data size in event " + eudaq::to_string(ev->GetEventNumber()) + ": " + e.what());
              }
            }
            break;
          }
        }
      }
      unsigned planeshit = 0;
      for (size_t i = 0; i < numplanes; ++i) {
        if (m_board[i].m_clusters.size() == 1) {
          planeshit++;
        }
      }
      if (planeshit >= numplanes-1) {
        std::ostringstream s;
        s << "Found track in event " << ev->GetEventNumber() << ":";
        for (size_t i = 0; i < numplanes; ++i) {
          if (m_board[i].m_clusters.size() == 1) {
            m_board[i].m_histotrack2d->Fill(m_board[i].m_clusterx[0], m_board[i].m_clustery[0]);
            if (i > 0) {
              m_board[i].m_histodeltax->Fill(m_board[i].m_clusterx[0] - m_board[i-1].m_clusterx[0]);
              m_board[i].m_histodeltay->Fill(m_board[i].m_clustery[0] - m_board[i-1].m_clustery[0]);
            }
            s << " (" << m_board[i].m_clusterx[0]
              << ", " << m_board[i].m_clustery[0]
              << ", " << m_board[i].m_clusters[0]
              << ")";
          }
        }
        EUDAQ_EXTRA(s.str());
      }
      m_board[0].m_histoclusterval->SetMaximum();
      m_board[1].m_histodeltax->SetMaximum();
      m_board[1].m_histodeltay->SetMaximum();
      double maxr = m_board[0].m_historawval->GetMaximum();
      double maxd = m_board[0].m_histocdsval->GetMaximum();
      double maxc = m_board[0].m_histoclusterval->GetMaximum();
      double maxx = m_board[1].m_histodeltax->GetMaximum();
      double maxy = m_board[1].m_histodeltay->GetMaximum();
      for (size_t i = 1; i < numplanes; ++i) {
        if (m_board[i].m_historawval->GetMaximum() > maxr) maxr = m_board[i].m_historawval->GetMaximum();
        if (m_board[i].m_histocdsval->GetMaximum() > maxd) maxd = m_board[i].m_histocdsval->GetMaximum();
        if (m_board[i].m_histoclusterval->GetMaximum() > maxc) maxc = m_board[i].m_histoclusterval->GetMaximum();
        if (m_board[i].m_histodeltax->GetMaximum() > maxx) maxx = m_board[i].m_histodeltax->GetMaximum();
        if (m_board[i].m_histodeltay->GetMaximum() > maxy) maxy = m_board[i].m_histodeltay->GetMaximum();
      }
      m_board[0].m_historawval->SetMaximum(maxr*1.1);
      m_board[0].m_histocdsval->SetMaximum(maxd*1.1);
      m_board[0].m_histoclusterval->SetMaximum(maxc*1.1);
      m_board[1].m_histodeltax->SetMaximum(maxx*1.1);
      m_board[1].m_histodeltay->SetMaximum(maxy*1.1);
      m_modified = true;
    }
  }
  virtual void OnBadEvent(counted_ptr<eudaq::Event> ev) {
    EUDAQ_ERROR("Bad event type found in data file");
    std::cout << "Bad Event: " << *ev << std::endl;
  }
  void Update() {
    TSeqCollection * list = gROOT->GetListOfCanvases();
    Int_t NCanvas = list->GetEntries();
    for (Int_t i = 0; i < NCanvas; ++i) {
      TCanvas * c = (TCanvas*)list->At(i);
      int j = 0;
      while (TVirtualPad * p = c->GetPad(++j)) {
        p->Modified();
      }
      c->Update();
    }
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
    counted_ptr<TH2D> m_historaw2d, m_tempcds, m_histocds2d, m_histohit2d,
      m_histocluster2d, m_histotrack2d, m_histonoise2d;
    counted_ptr<TH1D> m_historawx, m_historawy, m_historawval, m_histocdsval,
      m_histoclusterx, m_histoclustery, m_histoclusterval, m_histonumclusters,
      m_histodeltax, m_histodeltay;
    std::vector<double> m_clusters, m_clusterx, m_clustery;
    void Reset() {
      m_historaw2d->Reset();
      m_tempcds->Reset();
      m_histocds2d->Reset();
      m_histohit2d->Reset();
      m_histocluster2d->Reset();
      m_histotrack2d->Reset();
      m_histonoise2d->Reset();
      m_historawx->Reset();
      m_historawy->Reset();
      m_historawval->Reset();
      m_histocdsval->Reset();
      m_histoclusterx->Reset();
      m_histoclustery->Reset();
      m_histoclusterval->Reset();
      m_histonumclusters->Reset();
      m_histodeltax->Reset();
      m_histodeltay->Reset();
    }
  };

  void BookBoard(int board, BoardDisplay & b) {
    b.islog = false;
    //std::string name = "Board " + eudaq::to_string(board);
    TGCompositeFrame * frame = m_tabs->AddTab(make_name("Board ", board).c_str());
    b.m_embedded = new TRootEmbeddedCanvas(make_name("Canvas", board).c_str(), frame, 100, 100);
    frame->AddFrame(b.m_embedded.get(), m_hintbig.get());
    b.m_canvas = b.m_embedded->GetCanvas();
    b.m_canvas->Divide(5, 2);

    b.m_historaw2d      = new TH2D(make_name("RawProfile",    board).c_str(), "Raw 2D Profile",    264, 0, 264, 256, 0, 256);
    b.m_tempcds         = new TH2D(make_name("TempCDS",       board).c_str(), "Temp CDS",          264, 0, 264, 256, 0, 256);
    b.m_histocds2d      = new TH2D(make_name("CDSProfile",    board).c_str(), "CDS Profile",       264, 0, 264, 256, 0, 256);
    b.m_histohit2d      = new TH2D(make_name("HitMap",        board).c_str(), "Hit Profile",       264, 0, 264, 256, 0, 256);
    b.m_histocluster2d  = new TH2D(make_name("ClusterMap",    board).c_str(), "Cluster Profile",   132, 0, 264, 128, 0, 256);
    b.m_histotrack2d    = new TH2D(make_name("TrackMap",      board).c_str(), "Track Candidates",  132, 0, 264, 128, 0, 256);
    b.m_histonoise2d    = new TH2D(make_name("NoiseMap",      board).c_str(), "Noise Profile",     264, 0, 264, 256, 0, 256);
    b.m_historawx       = new TH1D(make_name("RawXProfile",   board).c_str(), "Raw X Profile",     264, 0, 264);
    b.m_historawy       = new TH1D(make_name("RawYProfile",   board).c_str(), "Raw Y Profile",     256, 0, 256);
    b.m_histoclusterx   = new TH1D(make_name("ClustXProfile", board).c_str(), "Cluster X Profile", 264, 0, 264);
    b.m_histoclustery   = new TH1D(make_name("ClustYProfile", board).c_str(), "Cluster Y Profile", 256, 0, 256);
    b.m_historawval     = new TH1D(make_name("RawValues",     board).c_str(), "Raw Values",        512, 0, 64);
    b.m_histocdsval     = new TH1D(make_name("CDSValues",     board).c_str(), "CDS Values",        100, 0, 10);
    b.m_histoclusterval = new TH1D(make_name("ClusterValues", board).c_str(), "Cluster Charge",    50,  0, 100);
    b.m_histonumclusters= new TH1D(make_name("NumClusters",   board).c_str(), "Num Clusters",      20,  0, 20);
    b.m_histodeltax     = new TH1D(make_name("DeltaX",        board).c_str(), "Delta X",           80,-40, 40);
    b.m_histodeltay     = new TH1D(make_name("DeltaY",        board).c_str(), "Delta Y",           80,-40, 40);
    b.m_histocds2d->Sumw2();
    b.m_historawval->SetBit(TH1::kCanRebin);
    b.m_histocdsval->SetBit(TH1::kCanRebin);
    b.m_histoclusterval->SetBit(TH1::kCanRebin);

    b.m_canvas->cd(1);
    b.m_histocluster2d->Draw("colz");
    b.m_canvas->cd(2);
    b.m_histoclusterx->Draw();
    b.m_canvas->cd(3);
    b.m_histoclustery->Draw();
    b.m_canvas->cd(4);
    b.m_histoclusterval->Draw();
    b.m_canvas->cd(5);
    b.m_histonoise2d->Draw("colz");

    b.m_canvas->cd(6);
    b.m_historaw2d->Draw("colz");
    b.m_canvas->cd(7);
    b.m_historawx->Draw();
    b.m_canvas->cd(8);
    b.m_historawy->Draw();
    b.m_canvas->cd(9);
    b.m_historawval->Draw();
    b.m_canvas->cd(10);
    b.m_histocdsval->Draw();
  }
  void FillBoard(BoardDisplay & b, eudaq::EUDRBBoard & e) {
    eudaq::EUDRBDecoder::arrays_t<double, double> a = m_decoder->GetArrays<double, double>(e);
    size_t npixels = m_decoder->NumPixels(e), nx=264, ny=256;
    std::vector<double> ones(npixels, 1.0);
    std::vector<double> cds(a.m_adc[0]);
    if (m_decoder->NumFrames(e) > 1) {
      for (size_t i = 0; i < cds.size(); ++i) {
        cds[i] = a.m_adc[0][i] * (a.m_pivot[i])
          + a.m_adc[1][i] * (1-2*a.m_pivot[i])
          + a.m_adc[2][i] * (a.m_pivot[i]-1);
      }
      b.m_historaw2d->FillN(npixels, &a.m_x[0], &a.m_y[0], &a.m_adc[1][0]);
      b.m_historaw2d->SetNormFactor(b.m_historaw2d->Integral() / m_histoevents);
      b.m_historawx->FillN(npixels, &a.m_x[0], &a.m_adc[1][0]);
      b.m_historawx->SetNormFactor(b.m_historawx->Integral() / m_histoevents / ny);
      b.m_historawy->FillN(npixels, &a.m_y[0], &a.m_adc[1][0]);
      b.m_historawy->SetNormFactor(b.m_historawy->Integral() / m_histoevents / nx);
      b.m_historawval->FillN(npixels, &a.m_adc[1][0], &ones[0]);
      b.m_historawval->SetNormFactor(b.m_historawval->Integral() / m_histoevents);
    }
    b.m_histocdsval->FillN(npixels, &cds[0], &ones[0]);
    b.m_histocdsval->SetNormFactor(b.m_histocdsval->Integral() / m_histoevents);
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
    b.m_histocds2d->FillN(npixels, &newx[0], &a.m_y[0], &cds[0]);
    b.m_clusters.clear();
    b.m_clusterx.clear();
    b.m_clustery.clear();
    if (m_histoevents >= 50) {
      if (m_histoevents < 500) {
        for (int iy = 1; iy <= b.m_tempcds->GetNbinsY(); ++iy) {
          for (int ix = 1; ix <= b.m_tempcds->GetNbinsX(); ++ix) {
            double rms = b.m_histocds2d->GetBinError(ix, iy) / sqrt(m_histoevents);
            b.m_histonoise2d->SetBinContent(ix, iy, rms);
          }
        }
      }
      std::vector<Seed> seeds;
      const double seed_thresh = 3.5 /* sigma */ , cluster_thresh = 7.5 /* sigma */;
      for (int iy = 1; iy <= b.m_tempcds->GetNbinsY(); ++iy) {
        for (int ix = 1; ix <= b.m_tempcds->GetNbinsX(); ++ix) {
          double s = b.m_tempcds->GetBinContent(ix, iy);
          double noise = 4.0; //b.m_histonoise2d->GetBinContent(ix, iy);
          if (s > seed_thresh*noise) {
            seeds.push_back(Seed(ix, iy, s));
          }
        }
      }
      if (seeds.size() < 1000) {
        std::sort(seeds.begin(), seeds.end(), &Seed::compare);
        for (size_t i = 0; i < seeds.size(); ++i) {
          if (b.m_tempcds->GetBinContent((int)seeds[i].x, (int)seeds[i].y) > 0) {
            double cluster = 0;
            double noise = 0;
            for (int dy = -1; dy <= 1; ++dy) {
              for (int dx = -1; dx <= 1; ++dx) {
                cluster += b.m_tempcds->GetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy);
                double n = 4.0; //b.m_histonoise2d->GetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy);
                noise += n*n;
                b.m_tempcds->SetBinContent((int)seeds[i].x+dx, (int)seeds[i].y+dy, 0);
              }
            }
            noise = sqrt(noise);
            if (cluster > cluster_thresh*noise) {
              b.m_clusters.push_back(cluster);
              b.m_clusterx.push_back(seeds[i].x);
              b.m_clustery.push_back(seeds[i].y);
            }
          }
        }
        b.m_histonumclusters->Fill(b.m_clusters.size());
        b.m_histohit2d->Reset();
        b.m_histohit2d->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histocluster2d->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histoclusterx->FillN(b.m_clusters.size(), &b.m_clusterx[0], &b.m_clusters[0]);
        b.m_histoclusterx->SetNormFactor(b.m_histoclusterx->Integral() / m_histoevents / ny);
        b.m_histoclustery->FillN(b.m_clusters.size(), &b.m_clustery[0], &b.m_clusters[0]);
        b.m_histoclustery->SetNormFactor(b.m_histoclustery->Integral() / m_histoevents / 256);
        b.m_histoclusterval->FillN(b.m_clusters.size(), &b.m_clusters[0], &ones[0]);
      }
    }
    if (!b.islog) {
      b.islog = true;
      b.m_canvas->cd(4)->SetLogy();
      b.m_canvas->cd(9)->SetLogy();
      b.m_canvas->cd(10)->SetLogy();
      //m_canvasmain->cd(board + m_board.size() + 1)->SetLogy();
    }
  }

  eudaq::Time m_lastupdate;
  bool m_modified, m_runended;
  unsigned long long m_prevt;
  unsigned long m_histoevents; // count of histogrammed events for normalization factor
  counted_ptr<eudaq::EUDRBDecoder> m_decoder;

  // Layout hints
  counted_ptr<TGLayoutHints> m_hinttop;
  counted_ptr<TGLayoutHints> m_hintleft;
  counted_ptr<TGLayoutHints> m_hintbig;

  // Toolbar
  counted_ptr<TGCompositeFrame> m_toolbar;
  counted_ptr<TGLabel>          m_tb_filename;
  counted_ptr<TGLabel>          m_tb_runnum;
  counted_ptr<TGLabel>          m_tb_evtnum;
  counted_ptr<TGNumberEntry>    m_tb_reduce;
  counted_ptr<TGNumberEntry>    m_tb_update;

  // Tabs
  counted_ptr<TGTab> m_tabs;

  // Main tab
  //TGCompositeFrame * m_framemain;
  TRootEmbeddedCanvas * m_embedmain;
  TCanvas * m_canvasmain;
  //counted_ptr<TH2D> m_histotrack, m_histotrack_coinc, m_histotrack_sum, m_histotrack_seed;

//   TH1I * m_hist_adc1;
//   TH1I * m_hist_x1;
//   TH1I * m_hist_y1;
//   TH1I * m_hist_adcds;
//   TH2F * m_hist_prof1;
//   TH2F * m_hist_profcds;

  // Board tabs (1 per board)
  std::vector<BoardDisplay> m_board;

  int m_cluster_size;
  //TTimer * m_timer;
  //TProcessEventTimer * m_processtimer;
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
