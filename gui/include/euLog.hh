#include "ui_euLog.h"
#include "LogCollectorModel.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Logger.hh"
#include <QMainWindow>
#include <QMessageBox>
#include <QItemDelegate>
#include <QPainter>

#include <iostream>

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class LogItemDelegate : public QItemDelegate {
public:
  LogItemDelegate(LogCollectorModel * model);
private:
  void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
  LogCollectorModel * m_model;
};

class LogCollectorGUI : public QMainWindow,
                        public Ui::wndLog,
                        public eudaq::LogCollector {
  Q_OBJECT
public:
  LogCollectorGUI(const std::string & runcontrol,
                  const std::string & listenaddress,
                  QRect geom,
                  const std::string & filename,
                  int loglevel,
                  QWidget *parent = 0,
                  Qt::WindowFlags flags = 0)
    : QMainWindow(parent, flags),
      eudaq::LogCollector(runcontrol, listenaddress),
      m_delegate(&m_model)
  {
    setupUi(this);
    viewLog->setModel(&m_model);
    viewLog->setItemDelegate(&m_delegate);
    viewLog->setColumnWidth(0, 96);
    viewLog->setColumnWidth(1, 70);
    viewLog->setColumnWidth(2, 240);
    int level = 0;
    for(;;) {
      std::string text = eudaq::Status::Level2String(level);
      if (text == "") break;
      text = eudaq::to_string(level) + "-" + text;
      cmbLevel->addItem(text.c_str());
      level++;
    }
    cmbLevel->setCurrentIndex(loglevel);
    QSize fsize = frameGeometry().size();
    if (geom.x() == -1) geom.setX(x());
    if (geom.y() == -1) geom.setY(y());
    else geom.setY(geom.y() + MAGIC_NUMBER);
    if (geom.width() == -1) geom.setWidth(fsize.width());
    if (geom.height() == -1) geom.setHeight(fsize.height());
    //else geom.setHeight(geom.height() - MAGIC_NUMBER);
    move(geom.topLeft());
    resize(geom.size());
    try {
      if (filename != "") LoadFile(filename);
    } catch (const std::runtime_error &) {
      // probably file not found: ignore
    }
  }
protected:
  void LoadFile(const std::string & filename) {
    std::vector<std::string> sources = m_model.LoadFile(filename);
    std::cout << "File loaded, sources = " << sources.size() << std::endl;
    for (size_t i = 0; i < sources.size(); ++i) {
      size_t dot = sources[i].find('.');
      if (dot != std::string::npos) {
        AddSender(sources[i].substr(0, dot), sources[i].substr(dot+1));
      } else {
        AddSender(sources[i]);
      }
    }
  }
  virtual void OnConnect(const eudaq::ConnectionInfo & id) {
    static bool registered = false;
    if (!registered) {
      qRegisterMetaType<QModelIndex>("QModelIndex");
      registered = true;
    }
    EUDAQ_INFO("Connection from " + to_string(id));
    AddSender(id.GetType(), id.GetName());
  }
  virtual void OnDisconnect(const eudaq::ConnectionInfo & id) {
    EUDAQ_INFO("Disconnected " + to_string(id));
  }
  virtual void OnReceive(const eudaq::LogMessage & msg) {
    static bool registered = false;
    if (!registered) {
      qRegisterMetaType<QModelIndex>("QModelIndex");
      registered = true;
    }
    QModelIndex pos = m_model.AddMessage(msg);
    if (pos.isValid()) viewLog->scrollTo(pos);
  }
  void AddSender(const std::string & type, const std::string & name = "");
private slots:
  void on_cmbLevel_currentIndexChanged(int index) {
    //std::cout << "Index " << index << "!!!" << std::endl;
    m_model.SetDisplayLevel(index);
  }
  void on_cmbFrom_currentIndexChanged(const QString & text) {
    std::string type = text.toStdString(), name;
    size_t dot = type.find('.');
    if (dot != std::string::npos) {
      name = type.substr(dot+1);
      type = type.substr(0, dot);
    }
    //std::cout << "From " << type << " " << name << std::endl;
    m_model.SetDisplayNames(type, name);
  }
  void on_txtSearch_editingFinished() {
    m_model.SetSearch(txtSearch->displayText().toStdString());
  }
private:
  LogCollectorModel m_model;
  LogItemDelegate m_delegate;
};
