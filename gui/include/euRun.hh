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

    typedef struct{
        std::vector<std::string> config_files;
        bool allow_nested_scan = false;
        bool scan_is_time_based = true;
        bool repeatScans = false;
        int time_per_step = 0;
        int events_per_step = 0;
        std::vector<int> steps_per_scan;
        int n_steps = 0;
        int n_scans = 0;
        std::vector<std::string> scanned_parameter;
        std::vector<std::string> scanned_producer;
        std::vector<std::string> events_counting_component;
        int current_step = 0;
    } scanSettings;

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

  void on_btn_LoadScanFile_clicked();
  void on_btnStartScan_clicked();
  void nextScanStep();
private:
  eudaq::Status::State updateInfos();
  bool loadInitFile();
  bool loadConfigFile();
  bool addStatusDisplay(std::pair<eudaq::ConnectionSPC, eudaq::StatusSPC> connection);
  bool removeStatusDisplay(std::pair<eudaq::ConnectionSPC, eudaq::StatusSPC> connection);
  bool updateStatusDisplay();
  bool addToGrid(const QString objectName, QString displayedName="");
  bool addAdditionalStatus(std::string info);
  bool checkFile(QString file, QString usecase);

  bool prepareAndStartStep();
  bool readScanConfig();
  bool checkScanParameters();
  void createConfigs();
  bool allConnectionsInState(eudaq::Status::State state);
  bool checkEventsInStep();
  int getEventsCurrent();

  static std::map<int, QString> m_map_state_str;
  std::map<QString, QString> m_map_label_str;
  eudaq::RunControlUP m_rc;
  RunControlModel m_model_conns;
  QItemDelegate m_delegate;
  QTimer m_timer_display;
  std::map<QString, QLabel*> m_str_label;
  std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> m_map_conn_status_last;
  uint32_t m_run_n_qsettings;
  int m_display_col, m_display_row;
  QMenu* contextMenu;
  bool m_lastexit_success;
  eudaq::LogSender m_log;
  bool m_scan_active;
  bool m_scan_interrupt_received;
  QTimer m_scanningTimer;
  std::shared_ptr<eudaq::Configuration> m_scan_config;
  scanSettings m_scan;


  void updateProgressBar();
};
