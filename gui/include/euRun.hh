#include "ui_euRun.h"
#include "ui_euRun.h"
#include "RunControlModel.hh"
#include "eudaq/RunControl.hh"
#include "eudaq/Utils.hh"
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

using eudaq::to_string;
using eudaq::from_string;

inline std::string to_bytes(const std::string &val) {
  if (val == "")
    return "";
  uint64_t n = from_string(val, 0ULL);
  const char *suff[] = {" B", " kB", " MB", " GB", " TB"};
  const int numsuff = sizeof suff / sizeof *suff;
  int mult = 0;
  while (n / 1024 >= 10 && mult < numsuff - 1) {
    n /= 1024;
    mult++;
  }
  return to_string(n) + suff[mult];
}

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class RunConnectionDelegate : public QItemDelegate {
public:
  RunConnectionDelegate(RunControlModel *model);
  void GetModelData();
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
  RunControlGUI(const std::string &listenaddress, QRect geom,
                QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ~RunControlGUI();

private:
  enum state_t {STATE_UNINIT, STATE_UNCONF, STATE_CONF, STATE_RUNNING, STATE_ERROR};// ST_CONFIGLOADED
  bool configLoaded = false;    // allows to disable / enable config button
                                // depending on whether config file was loaded
  bool configLoadedInit = false;
  bool disableButtons = false;
  int curState = STATE_UNINIT;
  QString lastUsedDirectory = "";
  QString lastUsedDirectoryInit = "";
  QStringList allConfigFiles;
  const int FONT_SIZE = 12;
  virtual void OnConnect(const eudaq::ConnectionInfo &id);
  virtual void OnDisconnect(const eudaq::ConnectionInfo &id) {
    m_run.disconnected(id);
  }
  virtual void OnReceive(const eudaq::ConnectionInfo &id,
                         std::shared_ptr<eudaq::ConnectionState> connectionstate);

  void SendState(int state){SetState(state); }
  void EmitStatus(const char *name, const std::string &val) {
    if (val == "")
      return;
    emit StatusChanged(name, val.c_str());
  }

  void closeEvent(QCloseEvent *event) {
    if (m_run.rowCount() > 0 &&
        QMessageBox::question(
            this, "Quitting", "Terminate all connections and quit?",
            QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
      event->ignore();
    } else {
      Terminate();
      event->accept();
    }
  }
  bool eventFilter(QObject *object, QEvent *event);

  bool checkInitFile() {
    QString loadedFile = txtInitFileName->text();
    if(loadedFile.isNull())
        return false;
    QRegExp rx (".+(\\.init$)");
    return rx.exactMatch(loadedFile);
  }

  bool checkConfigFile() {
    QString loadedFile = txtConfigFileName->text();
    if(loadedFile.isNull())
        return false;
    QRegExp rx (".+(\\.conf$)");
    return rx.exactMatch(loadedFile);
  }

  void updateButtons(int state) {
      if(state == STATE_RUNNING)
          disableButtons = false;
      configLoaded = checkConfigFile();
      configLoadedInit = checkInitFile();
      btnLoadInit->setEnabled(state != STATE_RUNNING && !disableButtons);
      btnInit->setEnabled(state != STATE_RUNNING && !disableButtons && configLoadedInit);
      btnLoad->setEnabled(state != STATE_RUNNING && state != STATE_UNINIT && !disableButtons);
      btnConfig->setEnabled(state != STATE_RUNNING && state != STATE_UNINIT && configLoaded && !disableButtons);
      btnTerminate->setEnabled(state != STATE_RUNNING);
      btnStart->setEnabled(state == STATE_CONF && !disableButtons);
      btnStop->setEnabled(state == STATE_RUNNING);
  }

private slots:

/* The function SetStateSlot is a slot function as defined by the Qt framework. When the signal is emmited, this function is triggered. 
This function takes a variable state, which corresponds to one of the four states which the program can be in. Depending on which state the
program is currently in the function will enable and disable certain buttons, and display the current state at the head of the GUI.*/

  void SetStateSlot(int state) {
    curState = state;
    updateButtons(state);
    if(state == STATE_UNINIT) {
       lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Uninitialised </b></font>").arg(FONT_SIZE));
    }
    else if(state == STATE_UNCONF) {
       lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Unconfigured </b></font>").arg(FONT_SIZE));
    } else if (state == STATE_CONF) {
       lblCurrent->setText(QString("<font size=%1 color='orange'><b>Current State: Configured </b></font>").arg(FONT_SIZE));
    }
    else if (state ==STATE_RUNNING)
       lblCurrent->setText(QString("<font size=%1 color='green'><b>Current State: Running </b></font>").arg(FONT_SIZE));
     else
       lblCurrent->setText(QString("<font size=%1 color='darkred'><b>Current State: Error </b></font>").arg(FONT_SIZE));
    m_run.UpdateDisplayed();
  }

  void on_btnInit_clicked() {
    std::string settings = txtInitFileName->text().toStdString();
    Init(settings);
  }

  void on_btnTerminate_clicked() { close(); }

  void on_btnConfig_clicked() {
    std::string settings = txtConfigFileName->text().toStdString();
    Configure(settings, txtGeoID->text().toInt());
    dostatus = true;
  }
  // void on_btnReset_clicked() {
  //  Reset();
  //}

  void on_btnStart_clicked(bool cont = false) { 
    disableButtons = true;
    m_prevtrigs = 0;
    m_prevtime = 0.0;
    m_runstarttime = 0.0;
    StartRun(cont ? "Continued" : txtRunmsg->displayText().toStdString());
    EmitStatus("RUN", to_string(m_runnumber));
    emit StatusChanged("EVENT", "0");
    emit StatusChanged("TRIG", "0");
    emit StatusChanged("PARTICLES", "0");
    emit StatusChanged("RATE", "");
    emit StatusChanged("MEANRATE", "");    
  }

  void on_btnStop_clicked() {
    StopRun();
    //std::cout << "DEBUG: Stop Button Pressed \n";
    EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
  }
  void on_btnLog_clicked() {
    std::string msg = txtLogmsg->displayText().toStdString();
    EUDAQ_USER(msg);
  }

  void on_btnLoadInit_clicked() {
    QString temporaryFileNameInit = QFileDialog::getOpenFileName(
        this, tr("Open File"), lastUsedDirectoryInit, tr("*.init (*.init)"));

    if (!temporaryFileNameInit.isNull()) {
      txtInitFileName->setText(temporaryFileNameInit);
      lastUsedDirectoryInit =
          QFileInfo(temporaryFileNameInit).path(); // store path for next time
      configLoadedInit = true;
      updateButtons(curState);
    }
  }

  void on_btnLoad_clicked() {
    QString temporaryFileName = QFileDialog::getOpenFileName(
        this, tr("Open File"), lastUsedDirectory, tr("*.conf (*.conf)"));

    if (!temporaryFileName.isNull()) {
      txtConfigFileName->setText(temporaryFileName);
      lastUsedDirectory =
          QFileInfo(temporaryFileName).path(); // store path for next time
      configLoaded = true;
      updateButtons(curState);
    }
  }

  void timer() {
    if (!m_stopping) {
      if ((m_runsizelimit >= 1024 && m_filebytes >= m_runsizelimit) ||
          (m_runeventlimit >= 1 && m_events >= m_runeventlimit)) {
        if (m_runsizelimit >= 1024 && m_filebytes >= m_runsizelimit) {
          EUDAQ_INFO("File limit reached: " + to_string(m_filebytes) + " > " +
                     to_string(m_runsizelimit));
        } else {
          EUDAQ_INFO("Event limit reached: " + to_string(m_events) + " > " +
                     to_string(m_runeventlimit));
        }
        eudaq::mSleep(1000);
	if(m_lastconfigonrunchange) {
          EUDAQ_INFO("All config files processed.");
	  m_startrunwhenready = false;
	  on_btnStop_clicked();
	  return;
	} else {	
          StopRun(false);
	}
        eudaq::mSleep(20000);
        if (m_nextconfigonrunchange) {
          QDir dir(lastUsedDirectory, "*.conf");
          // allConfigFiles
          for (size_t i = 0; i < dir.count(); ++i) {
            QString item = dir[i];
            allConfigFiles.append(dir.absoluteFilePath(item));
          }

          if (allConfigFiles.indexOf(
                  QRegExp("^" + QRegExp::escape(txtConfigFileName->text()))) +
                  1 <
              allConfigFiles.count()) {
            EUDAQ_INFO("Moving to next config file and starting a new run");
            txtConfigFileName->setText(allConfigFiles.at(allConfigFiles.indexOf(
                QRegExp("^" + QRegExp::escape(txtConfigFileName->text())))+1));
            on_btnConfig_clicked();
	    if(!m_nextconfigonrunchange) {
	      m_lastconfigonrunchange=true;
	    }
            eudaq::mSleep(1000);
            m_startrunwhenready = true;
          } else
            EUDAQ_INFO("All config files processed.");
        } else
          on_btnStart_clicked(true);
      } else if (dostatus) {
        GetConnectionState();
      }
    }
    if (m_startrunwhenready && !m_producer_pALPIDEfs_not_ok &&
        !m_producer_pALPIDEss_not_ok) {
      m_startrunwhenready = false;
      on_btnStart_clicked(false);
    }
  }
  void ChangeStatus(const QString &name, const QString &value) {
    status_t::iterator i = m_status.find(name.toStdString());
    if (i != m_status.end()) {
      i->second->setText(value);
    }
  }
  void btnLogSetStatusSlot(bool status) { btnLog->setEnabled(status); }
  void btnStartSetStatusSlot(bool status) { btnStart->setEnabled(status); }

signals:
  void StatusChanged(const QString &, const QString &);
  void btnLogSetStatus(bool status);
  void btnStartSetStatus(bool status);
  void SetState(int status);

private:
  RunControlModel m_run;
  RunConnectionDelegate m_delegate;
  QTimer m_statustimer;
  typedef std::map<std::string, QLabel *> status_t;
  status_t m_status;
  int m_prevtrigs;
  double m_prevtime, m_runstarttime;
  int64_t m_filebytes;
  int64_t m_events;
  bool dostatus;
  // Using automatic configuration file changes, the two variables below are
  // needed to ensure that the producers finished their configuration, before
  // the next run is started. If that is covered by the control FSM in a later
  // version of the software, these variables and the respective code need to
  // be removed again.
  bool m_producer_pALPIDEfs_not_ok;
  bool m_producer_pALPIDEss_not_ok;
  bool m_startrunwhenready;
  bool m_lastconfigonrunchange;
};
