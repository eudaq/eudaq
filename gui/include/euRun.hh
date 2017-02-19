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
// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

using eudaq::to_string;
using eudaq::from_string;

class RunConnectionDelegate : public QItemDelegate {
public:
  RunConnectionDelegate(RunControlModel *model);

private:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const;
  RunControlModel *m_model;
};


class RunControlGUI : public QMainWindow,
		      public Ui::wndRun,
		      public eudaq::RunControl {
  Q_OBJECT
public:
  RunControlGUI(const std::string &listenaddress,
                QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ~RunControlGUI() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoStatus(eudaq::ConnectionSPC id,
		std::shared_ptr<const eudaq::Status> status) override;
  void Exec() override final;
private:
  void EmitStatus(const char *name, const std::string &val);
  void closeEvent(QCloseEvent *event);
  bool eventFilter(QObject *object, QEvent *event);
  bool checkInitFile();
  bool checkConfigFile();
  void updateButtons(int state);
  std::string to_bytes(const std::string &val);

  void on_btnInit_clicked();
  void on_btnTerminate_clicked();
  void on_btnConfig_clicked();
  void on_btnStart_clicked(bool cont = false);
  void on_btnStop_clicked();
  void on_btnLog_clicked();
  void on_btnLoadInit_clicked();
  void on_btnLoad_clicked();

signals:
  void StatusChanged(const QString &, const QString &);
  void btnLogSetStatus(bool status);
  void btnStartSetStatus(bool status);
  void SetState(int status);
			   
private slots:
  void timer();
  void SetStateSlot(int state);
  void ChangeStatus(const QString &name, const QString &value);
  void btnLogSetStatusSlot(bool status);
  void btnStartSetStatusSlot(bool status);

private:
  enum state_t { STATE_UNINIT, STATE_UNCONF, STATE_CONF, STATE_RUNNING, STATE_ERROR };
  bool configLoaded;
  bool configLoadedInit;
  bool disableButtons;
  bool m_nextconfigonrunchange; 
  int curState;
  QString lastUsedDirectory;
  QString lastUsedDirectoryInit;  
  QStringList allConfigFiles;
  const int FONT_SIZE=12;

  RunControlModel m_run;
  RunConnectionDelegate m_delegate;
  QTimer m_statustimer;
  typedef std::map<std::string, QLabel *> status_t;
  status_t m_status;
  int m_prevtrigs;
  double m_prevtime, m_runstarttime;
  int64_t m_filebytes;
  int64_t m_events;
  bool m_data_taking;
  bool dostatus;
  // Using automatic configuration file changes, the two variables below are
  // needed to ensure that the producers finished their configuration, before
  // the next run is started. If that is covered by the control FSM in a later
  // version of the software, these variables and the respective code need to
  // be removed again.
  bool m_startrunwhenready;
  bool m_lastconfigonrunchange;

  int64_t m_runsizelimit;
  int64_t m_runeventlimit;
};
