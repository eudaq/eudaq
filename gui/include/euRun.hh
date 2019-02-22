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
#include <QGridLayout>


class RunControlGUI : public QMainWindow,
		      public Ui::wndRun{
  Q_OBJECT
public:
  RunControlGUI();
  void SetInstance(eudaq::RunControlUP rc);
  void Exec();
private:
  void closeEvent(QCloseEvent *event) override;
			
private slots:
  void DisplayTimer();  
  void on_btnInit_clicked();
  void on_btnConfig_clicked();
  void on_btnStart_clicked();
  void on_btnStop_clicked();
  void on_btnReset_clicked();
  void on_btnTerminate_clicked();
  void on_btnLog_clicked();
  void on_btnLoadInit_clicked();
  void on_btnLoadConf_clicked();
  void onCustomContextMenu(const QPoint &point);

private:
  bool loadInitFile();
  bool loadConfigFile();
  bool addStatusDisplay(auto connection);
  bool removeStatusDisplay(auto connection);
  bool updateStatusDisplay(auto map_conn_status);
  static std::map<int, QString> m_map_state_str;
  std::map<QString, QString> m_map_label_str;
  eudaq::RunControlUP m_rc;
  RunControlModel m_model_conns;
  QItemDelegate m_delegate;
  QTimer m_timer_display;
  std::map<QString, QLabel*> m_str_label;
  std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> m_map_conn_status_last;
  uint32_t m_run_n_qsettings;
  bool m_lastexit_success;
  int m_display_col, m_display_row;
  QMenu* contextMenu;
};
