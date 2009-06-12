#include "ui_euRun.h"
#include "RunControlModel.hh"
#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
#include <QMainWindow>
#include <QMessageBox>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QDir>
#include <QPainter>
#include <QTimer>

using eudaq::to_string;
using eudaq::from_string;

inline std::string to_bytes(const std::string & val) {
  if (val == "") return "";
  unsigned long long n = from_string(val, 0ULL);
  const char * suff[] = { " B", " kB", " MB", " GB", " TB" };
  const int numsuff = sizeof suff / sizeof *suff;
  int mult = 0;
  while (n/1024 >= 10 && mult < numsuff-1) {
    n /= 1024;
    mult++;
  }
  return to_string(n) + suff[mult];
}

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class RunConnectionDelegate : public QItemDelegate {
public:
  RunConnectionDelegate(RunControlModel * model);
private:
  void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
  RunControlModel * m_model;
};

class RunControlGUI : public QMainWindow, public Ui::wndRun, public eudaq::RunControl {
  Q_OBJECT
public:
  RunControlGUI(const std::string & listenaddress,
                QRect geom,
                QWidget *parent = 0,
                Qt::WindowFlags flags = 0);
private:
  virtual void OnConnect(const eudaq::ConnectionInfo & id) {
    static bool registered = false;
    if (!registered) {
      qRegisterMetaType<QModelIndex>("QModelIndex");
      registered = true;
    }
    //QMessageBox::information(this, "EUDAQ Run Control",
    //                         "This will reset all connected Producers etc.");
    m_run.newconnection(id);
    if (id.GetType() == "DataCollector") {
      EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
    }
  }
  virtual void OnDisconnect(const eudaq::ConnectionInfo & id) {
    m_run.disconnected(id);
  }
  virtual void OnReceive(const eudaq::ConnectionInfo & id, counted_ptr<eudaq::Status> status) {
    static bool registered = false;
    if (!registered) {
      qRegisterMetaType<QModelIndex>("QModelIndex");
      registered = true;
    }
    if (id.GetType() == "DataCollector") {
      EmitStatus("EVENT", status->GetTag("EVENT"));
      EmitStatus("FILEBYTES", to_bytes(status->GetTag("FILEBYTES")));
    } else if (id.GetType() == "Producer" && id.GetName() == "TLU") {
      EmitStatus("TRIG", status->GetTag("TRIG"));
      EmitStatus("PARTICLES", status->GetTag("PARTICLES"));
      EmitStatus("TIMESTAMP", status->GetTag("TIMESTAMP"));
      EmitStatus("LASTTIME", status->GetTag("LASTTIME"));
      bool ok = true;
      std::string scalers;
      for (int i = 0; i < 4; ++i) {
        std::string s = status->GetTag("SCALER" + to_string(i));
        if (s == "") {
          ok = false;
          break;
        }
        if (scalers != "") scalers += ", ";
        scalers += s;
      }
      if (ok) EmitStatus("SCALERS", scalers);
      int trigs = from_string(status->GetTag("TRIG"), -1);
      double time = from_string(status->GetTag("TIMESTAMP"), 0.0);
      if (trigs >= 0) {
        if (m_runstarttime == 0.0) {
          if (trigs > 0) m_runstarttime = time;
        } else {
          EmitStatus("MEANRATE", to_string((trigs-1)/(time-m_runstarttime)) + " Hz");
        }
        int dtrigs = trigs - m_prevtrigs;
        double dtime = time - m_prevtime;
        if (dtrigs >= 10 || dtime >= 1.0) {
          m_prevtrigs = trigs;
          m_prevtime = time;
          EmitStatus("RATE", to_string(dtrigs/dtime) + " Hz");
        }
      }
    }
    m_run.SetStatus(id, *status);
  }
  void EmitStatus(const char * name, const std::string & val) {
    if (val == "") return;
    emit StatusChanged(name, val.c_str());
  }
  void closeEvent(QCloseEvent * event) {
    if (m_run.rowCount() > 0 &&
        QMessageBox::question(this, "Quitting", "Terminate all connections and quit?",
                              QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
      event->ignore();
    } else {
      Terminate();
      event->accept();
    }
  }
  virtual void StartRun(const std::string & msg = "") {
    m_prevtrigs = 0;
    m_prevtime = 0.0;
    m_runstarttime = 0.0;
    RunControl::StartRun(msg);
    EmitStatus("RUN", to_string(m_runnumber));
    emit StatusChanged("EVENT", "0");
    emit StatusChanged("TRIG", "0");
    emit StatusChanged("PARTICLES", "0");
    emit StatusChanged("RATE", "");
    emit StatusChanged("MEANRATE", "");
  }
  virtual void StopRun(bool /*listen*/ = true) {
    RunControl::StopRun();
    EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
  }
private slots:
  void on_btnConfig_clicked() {
    std::string settings = cmbConfig->currentText().toStdString();
    Configure(settings);
  }
  void on_btnReset_clicked() {
    Reset();
  }
  void on_btnStart_clicked() {
    StartRun(txtRunmsg->displayText().toStdString());
  }
  void on_btnStop_clicked() {
    StopRun();
  }
  void on_btnLog_clicked() {
    std::string msg = txtLogmsg->displayText().toStdString();
    EUDAQ_USER(msg);
  }
  void timer() {
    if (!m_stopping) {
      GetStatus();
    }
  }
  void ChangeStatus(const QString & name, const QString & value) {
    status_t::iterator i = m_status.find(name.toStdString());
    if (i != m_status.end()) {
      i->second->setText(value);
    }
  }
signals:
  void StatusChanged(const QString &, const QString &);
private:
  RunControlModel m_run;
  RunConnectionDelegate m_delegate;
  QTimer m_statustimer;
  typedef std::map<std::string, QLabel *> status_t;
  status_t m_status;
  int m_prevtrigs;
  double m_prevtime, m_runstarttime;
};
