#include "ui_euLog.h"
#include "LogCollectorModel.hh"
#include "LogDialog.hh"
#include "eudaq/LogCollector.hh"
#include "eudaq/Logger.hh"
#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include <QItemDelegate>
#include <QPainter>

#include <iostream>
#include <QSettings>

// To make Qt behave on OSX (to be checked on other OSes)
#define MAGIC_NUMBER 22

class LogItemDelegate : public QItemDelegate {
public:
  LogItemDelegate(LogCollectorModel *model);

private:
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const;
  LogCollectorModel *m_model;
};

class LogCollectorGUI:public QMainWindow,
		      public Ui::wndLog,
		      public eudaq::LogCollector {
  Q_OBJECT
public:
  LogCollectorGUI(const std::string &runcontrol,
                  const std::string &listenaddress,
		  const std::string &directory,
		  int loglevel);
  ~LogCollectorGUI();
  void Exec() override;
protected:
  void LoadFile(const std::string &filename);
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoDisconnect(eudaq::ConnectionSPC id) override;
  void DoReceive(const eudaq::LogMessage &msg) override;
  void DoTerminate() override;
  void AddSender(const std::string &type, const std::string &name = "");
  void closeEvent(QCloseEvent *) override;
signals:
  void RecMessage(const eudaq::LogMessage &msg);
private slots:
  void on_cmbLevel_currentIndexChanged(int index);
  void on_cmbFrom_currentIndexChanged(const QString &text);
  void on_txtSearch_editingFinished();
  void on_viewLog_activated(const QModelIndex &i);
  void AddMessage(const eudaq::LogMessage &msg);
private:
  static void CheckRegistered();
  LogCollectorModel m_model;
  LogItemDelegate m_delegate;
};
