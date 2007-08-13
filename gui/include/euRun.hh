#include "ui_euRun.h"
#include "RunControlModel.hh"
#include "eudaq/RunControl.hh"
#include <QMainWindow>
#include <QMessageBox>
#include <QCloseEvent>
#include <QItemDelegate>
#include <QDir>
#include <QPainter>
#include <QTimer>

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
                Qt::WindowFlags flags = 0)
    : QMainWindow(parent, flags),
      eudaq::RunControl(listenaddress),
      m_delegate(&m_run)
  {
    setupUi(this);
    viewConn->setModel(&m_run);
    viewConn->setItemDelegate(&m_delegate);
    QDir dir("./conf/", "*.conf");
    for (size_t i = 0; i < dir.count(); ++i) {
      QString item = dir[i];
      item.chop(5);
      cmbConfig->addItem(item);
    }
    cmbConfig->setEditText("default");
    QSize fsize = frameGeometry().size();
    if (geom.x() == -1) geom.setX(x());
    if (geom.y() == -1) geom.setY(y());
    else geom.setY(geom.y() + MAGIC_NUMBER);
    if (geom.width() == -1) geom.setWidth(fsize.width());
    if (geom.height() == -1) geom.setHeight(fsize.height());
    //else geom.setHeight(geom.height() - MAGIC_NUMBER);
    move(geom.topLeft());
    resize(geom.size());
    connect(this, SIGNAL(RunNumberChanged(const QString &)), txtRunNumber, SLOT(setText(const QString &)));
    connect(this, SIGNAL(TrigNumberChanged(const QString &)), txtTriggers, SLOT(setText(const QString &)));
    connect(this, SIGNAL(EventNumberChanged(const QString &)), txtEvents,  SLOT(setText(const QString &)));
    connect(this, SIGNAL(TimestampChanged(const QString &)), txtTimestamp, SLOT(setText(const QString &)));
    connect(&m_statustimer, SIGNAL(timeout()), this, SLOT(timer()));
    m_statustimer.start(500);
  }
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
      emit RunNumberChanged(("(" + eudaq::to_string(m_runnumber) + ")").c_str());
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
      emit EventNumberChanged(status->GetTag("EVENT").c_str());
    } else if (id.GetType() == "Producer" && id.GetName() == "TLU") {
      emit TrigNumberChanged(status->GetTag("TRIG").c_str());
      emit TimestampChanged(status->GetTag("TIMESTAMP").c_str());
    }
    m_run.SetStatus(id, *status);
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
    emit RunNumberChanged(eudaq::to_string(m_runnumber).c_str());
  }
  void on_btnStop_clicked() {
    StopRun();
    txtRunNumber->setText(("(" + eudaq::to_string(m_runnumber) + ")").c_str());
  }
  void on_btnLog_clicked() {
    std::string msg = txtLogmsg->displayText().toStdString();
    EUDAQ_USER(msg);
  }
  void timer() {
    GetStatus();
  }
signals:
  void RunNumberChanged(const QString &);
  void TrigNumberChanged(const QString &);
  void EventNumberChanged(const QString &);
  void TimestampChanged(const QString &);
private:
  RunControlModel m_run;
  RunConnectionDelegate m_delegate;
  QTimer m_statustimer;
};
