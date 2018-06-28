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
  LogCollectorGUI(const std::string &name,
                  const std::string &runcontrol);
  ~LogCollectorGUI();
  void Exec() override;
public:
  void DoInitialise() override;
  void DoConnect(eudaq::ConnectionSPC id) override;
  void DoReceive(const eudaq::LogMessage &msg) override;
  void LoadFile(const std::string &filename);
  void AddSender(const std::string &type, const std::string &name = "");
  void closeEvent(QCloseEvent *) override;
  void DoTerminate() override;
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
  std::ofstream m_os_file;
};
