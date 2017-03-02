#include "ui_euRun.h"
#include "RunControlModel.hh"
#include "eudaq/RunControl.hh"
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QDir>
#include <QPainter>
#include <QTimer>
#include <QInputDialog>
#include <QSettings>
#include <QRegExp>
#include <QString>

#define FONT_SIZE 12

using eudaq::from_string;

class RunConnectionDelegate : public QItemDelegate {
public:
  RunConnectionDelegate();
};

class RunControlGUI : public QMainWindow,
		      public Ui::wndRun,
		      public eudaq::RunControl {
  Q_OBJECT
public:
  RunControlGUI(const std::string &listenaddress);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoStatus(eudaq::ConnectionSPC id, eudaq::StatusSPC status) override;
  void Exec() override final;
private:
  void closeEvent(QCloseEvent *event) override;
  bool checkInitFile();
  bool checkConfigFile();
  void updateButtons();
  void updateTopbar();

signals:
  void StatusChanged(const QString &, const QString &);
		     
private slots:
  void DisplayTimer();
  void AutorunTimer();
  void ChangeStatus(const QString &k, const QString &v);
  
  void on_btnInit_clicked();
  void on_btnConfig_clicked();
  void on_btnStart_clicked();
  void on_btnStop_clicked();
  void on_btnReset_clicked();
  void on_btnTerminate_clicked();
  void on_btnLog_clicked();
  void on_btnLoadInit_clicked();
  void on_btnLoadConf_clicked();
private:
  enum state_t {STATE_UNINIT, STATE_UNCONF, STATE_CONF, STATE_RUNNING, STATE_ERROR };
  int m_state;

  RunControlModel m_run;
  QItemDelegate m_delegate;
  QTimer m_timer_display;
  QTimer m_timer_autorun;
  std::map<std::string, QLabel*> m_status;
  
  int m_prevtrigs;
  double m_prevtime;
  double m_runstarttime;
  int64_t m_filebytes;
  int64_t m_events;
  int64_t m_runeventlimit;
};
