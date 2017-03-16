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

class RunConnectionDelegate : public QItemDelegate {
public:
  RunConnectionDelegate();
};

class RunControlGUI : public QMainWindow,
		      public Ui::wndRun{
  Q_OBJECT
public:
  RunControlGUI();
  void SetInstance(eudaq::RunControlUP rc);
  void Exec();
private:
  void closeEvent(QCloseEvent *event) override;
  bool checkInitFile();
  bool checkConfigFile();
  void updateButtons();

private slots:
  void DisplayTimer();
  // void ChangeStatus(const QString &k, const QString &v);
  
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
  const std::map<int, QString> m_map_state_str ={
    {eudaq::Status::STATE_UNINIT,
     "<font size=12 color='red'><b>Current State: Uninitialised </b></font>"},
    {eudaq::Status::STATE_UNCONF,
     "<font size=12 color='red'><b>Current State: Unconfigured </b></font>"},
    {eudaq::Status::STATE_CONF,
     "<font size=12 color='orange'><b>Current State: Configured </b></font>"},
    {eudaq::Status::STATE_RUNNING,
     "<font size=12 color='green'><b>Current State: Running </b></font>"},
    {eudaq::Status::STATE_ERROR,
     "<font size=12 color='darkred'><b>Current State: Error </b></font>"},
    };

  int m_state;
  eudaq::RunControlUP m_rc;
  
  RunControlModel m_run;
  QItemDelegate m_delegate;
  QTimer m_timer_display;
  std::map<std::string, QLabel*> m_status;  
};
