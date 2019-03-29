#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRun.hh"
#include "Colours.hh"
#include "eudaq/Config.hh"

using std::cout;
using std::endl;
RunControlGUI::RunControlGUI()
  : QMainWindow(0, 0),
    m_display_col(0),
    m_scan_active(false),
    m_scan_interrupt_received(false),
    m_display_row(0){
    m_map_label_str = {{"RUN", "Run Number"}};

  qRegisterMetaType<QModelIndex>("QModelIndex");
  setupUi(this);

  lblCurrent->setText(m_map_state_str.at(eudaq::Status::STATE_UNINIT));
  for(auto &label_str: m_map_label_str) {
    QLabel *lblname = new QLabel(grpStatus);
    lblname->setObjectName("lbl_st_" + label_str.first);
    lblname->setText(label_str.second + ": ");
    QLabel *lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName("txt_st_" + label_str.first);
    grpGrid->addWidget(lblname, m_display_row, m_display_col * 2);
    grpGrid->addWidget(lblvalue, m_display_row, m_display_col * 2 + 1);
    m_str_label[label_str.first] = lblvalue;
    if (++m_display_col > 1){
      ++m_display_row;
      m_display_col = 0;
    }
  }

  viewConn->setModel(&m_model_conns);
  viewConn->setItemDelegate(&m_delegate);

  viewConn->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(viewConn, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(onCustomContextMenu(const QPoint &)));

  QRect geom(-1,-1, 150, 200);
  QRect geom_from_last_program_run;
  QSettings settings("EUDAQ collaboration", "EUDAQ");
  settings.beginGroup("euRun2");
  m_run_n_qsettings = settings.value("runnumber", 0).toUInt();
  m_lastexit_success = settings.value("successexit", 1).toUInt();
  geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
  geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
  //TODO: check last if last file exits. if not, use defalt value.
  txtConfigFileName
    ->setText(settings.value("lastConfigFile", "config file not set").toString());
  txtInitFileName
    ->setText(settings.value("lastInitFile", "init file not set").toString());
  txtScanFile
    ->setText(settings.value("lastScanFile", "scan file not set").toString());

  settings.endGroup();

  QSize fsize = frameGeometry().size();
  if((geom.x() == -1)||(geom.y() == -1)||(geom.width() == -1)||(geom.height() == -1)) {
    if((geom_from_last_program_run.x() == -1)||(geom_from_last_program_run.y() == -1)||(geom_from_last_program_run.width() == -1)||(geom_from_last_program_run.height() == -1)) {
      geom.setX(x());
      geom.setY(y());
      geom.setWidth(fsize.width());
      geom.setHeight(fsize.height());
      move(geom.topLeft());
      resize(geom.size());
    } else {
      move(geom_from_last_program_run.topLeft());
      resize(geom_from_last_program_run.size());
    }
  }

  setWindowTitle("eudaq Run Control " PACKAGE_VERSION);
  connect(&m_timer_display, SIGNAL(timeout()), this, SLOT(DisplayTimer()));
  connect(&m_scanningTimer,SIGNAL(timeout()), this, SLOT(nextScanStep()));
  m_timer_display.start(1000); // internal update time of GUI
  btnInit->setEnabled(1);
  btnConfig->setEnabled(1);
  btnLoadInit->setEnabled(1);
  btnLoadConf->setEnabled(1);
  btnStart->setEnabled(1);
  btnStop->setEnabled(1);
  btnReset->setEnabled(1);
  btnTerminate->setEnabled(1);
  btnLog->setEnabled(1);

  QSettings settings_output("EUDAQ collaboration", "EUDAQ");
  settings_output.beginGroup("euRun2");
  settings_output.setValue("successexit", 0);
  settings_output.endGroup();
}

void RunControlGUI::SetInstance(eudaq::RunControlUP rc){
  m_rc = std::move(rc);
  if(m_lastexit_success)
    m_rc->SetRunN(m_run_n_qsettings);
  else
    m_rc->SetRunN(m_run_n_qsettings+1);
  auto thd_rc = std::thread(&eudaq::RunControl::Exec, m_rc.get());
  thd_rc.detach();
}

void RunControlGUI::on_btnInit_clicked(){
  std::string settings = txtInitFileName->text().toStdString();
  if(!checkFile(QString::fromStdString(settings),QString::fromStdString("init file")))
      return;
  if(m_rc){
    m_rc->ReadInitilizeFile(settings);
    m_rc->Initialise();
  }
}

void RunControlGUI::on_btnTerminate_clicked(){
  close();
}

void RunControlGUI::on_btnConfig_clicked(){
  std::string settings = txtConfigFileName->text().toStdString();
  if(!checkFile(QString::fromStdString(settings),QString::fromStdString("Config file")))
      return;
  if(m_rc){
    m_rc->ReadConfigureFile(settings);
    m_rc->Configure();
  }

  if(m_rc)
  {
  eudaq::ConfigurationSPC conf = m_rc->GetConfiguration();
  conf->SetSection("RunControl");
  std::string additionalDisplays = conf->Get("ADDITIONAL_DISPLAY_NUMBERS","");
  if(additionalDisplays!="")
    addAdditionalStatus(additionalDisplays);
  }
}

void RunControlGUI::on_btnStart_clicked(){
  QString qs_next_run = txtNextRunNumber->text();
  if(!qs_next_run.isEmpty()){
    bool succ;
    uint32_t run_n = qs_next_run.toInt(&succ);
    if(succ){
      m_rc->SetRunN(run_n);
    }
    txtNextRunNumber->clear();
  }
  if(m_rc)
    m_rc->StartRun();
}

void RunControlGUI::on_btnStop_clicked() {
  if(m_rc)
    m_rc->StopRun();
}

void RunControlGUI::on_btnReset_clicked() {
  if(m_rc)
    m_rc->Reset();
  EUDAQ_USER("Reset called");
}

void RunControlGUI::on_btnLog_clicked() {
  std::string msg = txtLogmsg->displayText().toStdString();
  EUDAQ_USER(msg);
}

void RunControlGUI::on_btnLoadInit_clicked() {
  QString usedpath =QFileInfo(txtInitFileName->text()).path();
  QString filename =QFileDialog::getOpenFileName(this, tr("Open File"),
                         usedpath,
                         tr("*.ini (*.ini)"));
  if (!filename.isNull()){
    txtInitFileName->setText(filename);
  }
}

void RunControlGUI::on_btnLoadConf_clicked() {
  QString usedpath =QFileInfo(txtConfigFileName->text()).path();
  QString filename =QFileDialog::getOpenFileName(this, tr("Open File"),
                         usedpath,
                         tr("*.conf (*.conf)"));
  if (!filename.isNull()) {
    txtConfigFileName->setText(filename);
  }



}

void RunControlGUI::DisplayTimer(){
  updateInfos();
  updateStatusDisplay();
  if(m_scan_active == false)
      btnStartScan->setText("Start Scan");
  else
      updateProgressBar();

  if(!m_scan.scan_is_time_based && m_scan_active == true)
      if(checkEventsInStep())
          nextScanStep();
}

void RunControlGUI::updateInfos(){
    std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> map_conn_status;
    auto state = eudaq::Status::STATE_RUNNING;
    if(m_rc)
      map_conn_status= m_rc->GetActiveConnectionStatusMap();

    for(auto &conn_status_last: m_map_conn_status_last){
      if(!map_conn_status.count(conn_status_last.first)){
        m_model_conns.disconnected(conn_status_last.first);
        removeStatusDisplay(conn_status_last);
      }
    }
    for(auto &conn_status: map_conn_status){
      if(!m_map_conn_status_last.count(conn_status.first)){
        m_model_conns.newconnection(conn_status.first);
        if(! (conn_status.first->GetType()== "LogCollector"))
            addStatusDisplay(conn_status);
      }
    }
    if(map_conn_status.empty()){
      state = eudaq::Status::STATE_UNINIT;
    }
    else{
      state = eudaq::Status::STATE_RUNNING;
      for(auto &conn_status: map_conn_status){
        if(!conn_status.second)
      continue;
        auto state_conn = conn_status.second->GetState();
        state_conn < state ? state = (eudaq::Status::State)state_conn : state = state ;
        m_model_conns.SetStatus(conn_status.first, conn_status.second);
      }
    }

    QRegExp rx_init(".+(\\.ini$)");
    QRegExp rx_conf(".+(\\.conf$)");
    bool confLoaded = rx_conf.exactMatch(txtConfigFileName->text());
    bool initLoaded = rx_init.exactMatch(txtInitFileName->text());

    btnInit->setEnabled(state == eudaq::Status::STATE_UNINIT && initLoaded);
    btnConfig->setEnabled((state == eudaq::Status::STATE_UNCONF ||
               state == eudaq::Status::STATE_CONF )&& confLoaded);
    btnLoadInit->setEnabled(state != eudaq::Status::STATE_RUNNING);
    btnLoadConf->setEnabled(state != eudaq::Status::STATE_RUNNING);
    btnStart->setEnabled(state == eudaq::Status::STATE_CONF);
    btnStop->setEnabled(state == eudaq::Status::STATE_RUNNING && !m_scan_active);
    btnReset->setEnabled(state != eudaq::Status::STATE_RUNNING);
    btnTerminate->setEnabled(state != eudaq::Status::STATE_RUNNING);

    lblCurrent->setText(m_map_state_str.at(state));

    uint32_t run_n = m_rc->GetRunN();
    if(m_run_n_qsettings != run_n){
      m_run_n_qsettings = run_n;
      QSettings settings("EUDAQ collaboration", "EUDAQ");
      settings.beginGroup("euRun2");
      settings.setValue("runnumber", m_run_n_qsettings);
      settings.endGroup();
    }

    if(m_rc&&m_str_label.count("RUN")){
      if(state == eudaq::Status::STATE_RUNNING){
        m_str_label.at("RUN")->setText(QString::number(run_n));
      } else {
        m_str_label.at("RUN")->setText(QString::number(run_n)+" (next run)");
      }
    }
    m_map_conn_status_last = map_conn_status;
}

void RunControlGUI::closeEvent(QCloseEvent *event) {
  if (QMessageBox::question(this, "Quitting",
                "Terminate all connections and quit?",
                QMessageBox::Ok | QMessageBox::Cancel)
      == QMessageBox::Cancel){
    event->ignore();
  } else {
    QSettings settings("EUDAQ collaboration", "EUDAQ");
    settings.beginGroup("euRun2");
    if(m_rc)
      settings.setValue("runnumber", m_rc->GetRunN());
    else
      settings.setValue("runnumber", m_run_n_qsettings);
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("lastConfigFile", txtConfigFileName->text());
    settings.setValue("lastInitFile", txtInitFileName->text());
    settings.setValue("lastScanFile", txtScanFile->text());
    settings.setValue("successexit", 1);
    settings.endGroup();
    if(m_rc)
      m_rc->Terminate();
    event->accept();
  }
}

void RunControlGUI::Exec(){
  show();
  if(QApplication::instance())
    QApplication::instance()->exec();
  else
    std::cerr<<"ERROR: RUNContrlGUI::EXEC\n";
}


std::map<int, QString> RunControlGUI::m_map_state_str ={
    {eudaq::Status::STATE_UNINIT,
     "<font size=12 color='red'><b>Current State: Uninitialised </b></font>"},
    {eudaq::Status::STATE_UNCONF,
     "<font size=12 color='red'><b>Current State: Unconfigured </b></font>"},
    {eudaq::Status::STATE_CONF,
     "<font size=12 color='orange'><b>Current State: Configured </b></font>"},
    {eudaq::Status::STATE_RUNNING,
     "<font size=12 color='green'><b>Current State: Running </b></font>"},
    {eudaq::Status::STATE_ERROR,
     "<font size=12 color='darkred'><b>Current State: Error </b></font>"}
};


void RunControlGUI::onCustomContextMenu(const QPoint &point)
{
    QModelIndex index = viewConn->indexAt(point);
    if(index.isValid()) {
    QMenu *contextMenu = new QMenu(viewConn);

    if(!m_rc->GetInitConfiguration()){
    loadInitFile();
    }
    if(m_rc->GetInitConfiguration()){
    QAction *initialiseAction = new QAction("Initialise", this);
    connect(initialiseAction, &QAction::triggered, this, [this,index]() {m_rc->InitialiseSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(initialiseAction);
    }

    if(!m_rc->GetConfiguration()){
    loadConfigFile();
    }
    if(m_rc->GetConfiguration()){
    QAction *configureAction = new QAction("Configure", this);
    connect(configureAction, &QAction::triggered, this, [this,index]() {m_rc->ConfigureSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(configureAction);
    }

    QAction *startAction = new QAction("Start", this);
    connect(startAction, &QAction::triggered, this, [this,index]() {m_rc->StartSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(startAction);

    QAction *stopAction = new QAction("Stop", this);
    connect(stopAction, &QAction::triggered, this, [this,index]() {m_rc->StopSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(stopAction);

    QAction *resetAction = new QAction("Reset", this);
    connect(resetAction, &QAction::triggered, this, [this,index]() {m_rc->ResetSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(resetAction);

    QAction *terminateAction = new QAction("Terminate", this);
    connect(terminateAction, &QAction::triggered, this, [this,index]() {m_rc->TerminateSingleConnection(m_model_conns.getConnection(index));});
    contextMenu->addAction(terminateAction);

    contextMenu->exec(viewConn->viewport()->mapToGlobal(point));
    }

}


bool RunControlGUI::loadInitFile() {
  std::string settings = txtInitFileName->text().toStdString();
  QFileInfo check_file(txtInitFileName->text());
  if(!check_file.exists() || !check_file.isFile()){
    QMessageBox::warning(NULL, "ERROR", "Init file does not exist.");
    return false;
  }
  if(m_rc){
    m_rc->ReadInitilizeFile(settings);
  }
  return true;
}

bool RunControlGUI::loadConfigFile() {
  std::string settings = txtConfigFileName->text().toStdString();
  QFileInfo check_file(txtConfigFileName->text());
  if(!check_file.exists() || !check_file.isFile()){
    QMessageBox::warning(NULL, "ERROR", "Config file does not exist.");
    return false;
  }
  if(m_rc){
    m_rc->ReadConfigureFile(settings);
  }
  return true;
}

bool RunControlGUI::addStatusDisplay(std::pair<eudaq::ConnectionSPC, eudaq::StatusSPC> connection) {
    QString name = QString::fromStdString(connection.first->GetName()
                                         +":"+connection.first->GetType());
    QString displayName = QString::fromStdString(connection.first->GetName()
                                         +":EventN");
    addToGrid(displayName,name);
    return true;
}

bool RunControlGUI::removeStatusDisplay(std::pair<eudaq::ConnectionSPC, eudaq::StatusSPC> connection) {
    // remove obsolete information from disconnected values
    for(auto idx=0; idx<grpGrid->count();idx++) {
        QLabel * l = dynamic_cast<QLabel *> (grpGrid->itemAt(idx)->widget());
        if(l->objectName()==QString::fromStdString(connection.first->GetName()
                                                   +":"+connection.first->GetType())) {
            // Status updates are always pairs
            m_map_label_str.erase(l->objectName());
            m_str_label.erase(l->objectName());
            grpGrid->removeWidget(l);
            delete l;
            l = dynamic_cast<QLabel *> (grpGrid->itemAt(idx)->widget());
            grpGrid->removeWidget(l);
            delete l;
        }
    }
    return true;
}
bool RunControlGUI::addToGrid(const QString objectName, QString displayedName) {

    if(m_str_label.count(objectName)==1) {
        //QMessageBox::warning(NULL,"ERROR - Status display","Duplicating display entry request: "+objectName);
        return false;
    }
    if(displayedName=="")
        displayedName = objectName;
    QLabel *lblname = new QLabel(grpStatus);
    lblname->setObjectName(objectName);
    lblname->setText(displayedName+": ");
    QLabel *lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName("val_"+objectName);
    lblvalue->setText("val_"+objectName);

    int colPos = 0, rowPos = 0;
    if( 2* (m_str_label.size()+1) < grpGrid->rowCount() * grpGrid->columnCount() ) {
        colPos = m_display_col;
        rowPos = m_display_row;
        if (++m_display_col > 1) {
            ++m_display_row;
            m_display_col = 0;
        }
    }
    else {
        colPos = m_display_col;
        rowPos = m_display_row;
        if (++m_display_col > 1){
            ++m_display_row;
            m_display_col = 0;
        }
    }
    m_map_label_str.insert(std::pair<QString, QString>(objectName,objectName+": "));
    m_str_label.insert(std::pair<QString, QLabel*>(objectName, lblvalue));
    grpGrid->addWidget(lblname, rowPos, colPos * 2);
    grpGrid->addWidget(lblvalue, rowPos, colPos * 2 + 1);
    return true;
}
/**
 * @brief RunControlGUI::updateStatusDisplay
 * @return true if success, false otherwise (cannot happen currently)
 */
bool RunControlGUI::updateStatusDisplay() {
    auto it = m_map_conn_status_last.begin();
    while(it!=m_map_conn_status_last.end()) {
        // elements might not be existing at startup/beeing asynchronously changed
        if(it->first && it->second) {
            auto labelit = m_str_label.begin();
            while(labelit!=m_str_label.end()) {
                std::string labelname     = (labelit->first.toStdString()).substr(0,labelit->first.toStdString().find(":"));
                std::string displayedItem = (labelit->first.toStdString()).substr(labelit->first.toStdString().find(":")+1,labelit->first.toStdString().size());
                if(it->first->GetName()==labelname) {
                    auto tags = it->second->GetTags();
                    // obviously not really elegant...
                    for(auto &tag: tags){
                        if(tag.first==displayedItem && displayedItem=="EventN")
                            labelit->second->setText(QString::fromStdString(tag.second+" Events"));
                        else if(tag.first==displayedItem && displayedItem=="Freq. (avg.) [kHz]")
                            labelit->second->setText(QString::fromStdString(tag.second+" kHz"));
                        else if(tag.first==displayedItem)
                            labelit->second->setText(QString::fromStdString(tag.second));
                    }
                }
                labelit++;
            }
        }
        it++;
    }
       return true;
}

bool RunControlGUI::addAdditionalStatus(std::string info) {
    std::vector<std::string> results = eudaq::splitString(info,',');
    if(results.size()%2!=0) {
        QMessageBox::warning(NULL,"ERROR","Additional Status Display inputs are not correctly formatted - please check");
       return false;
    } else {
        for(auto c = 0; c < results.size();c+=2) {
            // check if the connection exists, otherwise do not display
            auto it = m_map_conn_status_last.begin();
            bool found = false;
            while(it != m_map_conn_status_last.end()) {
                if(it->first && it->first->GetName()==results.at(c)){
                    addToGrid(QString::fromStdString(results.at(c)+":"+results.at(c+1)));
                    found = true;
                }
                it++;
            }
            if(!found) {
                QMessageBox::warning(NULL,"ERROR",QString::fromStdString("Element \""+results.at(c)+ "\" is not connected"));
                return false;
            }
        }
    }
    return true;
}

bool RunControlGUI::checkFile(QString file, QString usecase)
{
    QFileInfo check_file(file);
    if(!check_file.exists() || !check_file.isFile()){
      QMessageBox::warning(NULL, "ERROR",QString(usecase + "Init file does not exist."));
      return false;
    }
    else
        return true;
}

/**

 SCAN FUNCTIONALITIES
*/

/**
 * @brief RunControlGUI::on_btn_LoadScanFile_clicked
 * @abstract push Button to open file dialog to select the scan configuration
 * file.
 * @group Scanning utils, RunControlGUI
 */

void RunControlGUI::on_btn_LoadScanFile_clicked()
{
    QString usedpath =QFileInfo(txtScanFile->text()).path();
    QString filename =QFileDialog::getOpenFileName(this, tr("Open File"),
                           usedpath,
                           tr("*.scan (*.scan)"));
    if (!filename.isNull()){
      txtScanFile->setText(filename);
    }

}
/**
 * @brief RunControlGUI::on_btnStartScan_clicked
 * @abstract Button to control the scanning procedure. Does not implement any real
 * functionality, only changes status bools and texts
 *
 */
void RunControlGUI::on_btnStartScan_clicked()
{
   if(!checkFile(txtScanFile->text(),QString::fromStdString("Scan File")))
       return;
  if(!readScanConfig()){
      QMessageBox::warning(NULL,"ERROR","invalid scan config file");
      return;
    }
   if(m_scan_active == true) {
       QMessageBox::StandardButton reply;
       reply = QMessageBox::question(NULL,"Interrupt Scan","Do you want to stop immediately?\n Hitting no will stop after finishing the current step",
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Abort);
       if(reply==QMessageBox::Yes) {
           m_scan_active = false;
           m_scanningTimer.stop();
           btnStartScan->setText("Start Scan");

       } else if(reply==QMessageBox::Abort) {
           m_scan_active = true;
           btnStartScan->setText("Interrupt scan");
       } else if(reply==QMessageBox::No) {
           m_scan_interrupt_received = true;
           btnStartScan->setText("Scan stops after current step");
       }
   } else {
       // init scan parameters:
       m_scan.current_step = 0;
       progressBar_scan->setMaximum(100);
       m_scan_active = true;
       btnStartScan->setText("Interrupt scan");
       nextScanStep();
   }
}

/**
 * @brief RunControlGUI::nextScanStep
 * @abstract Slot to start the next step - Can be either triggered via a QTimer
 * or an eventnumber of one producer
 */
void RunControlGUI::nextScanStep()
{
    // stop readout
    if((m_scan.current_step > m_scan.n_steps || m_scan_interrupt_received) && m_scan_active) {
        m_scan_active = false;
        m_scanningTimer.stop();
        btnStartScan->setText("Start Scan");
        on_btnStop_clicked();
        QMessageBox::information(NULL,"Scan finished","Scan successfully completed");
    }else if(m_scan_active) {
        txtConfigFileName
                ->setText(QString::fromStdString(m_scan.config_files.at(m_scan.current_step)));
        if(!prepareAndStartStep())
        {
            m_scan_active = false;
            return;
        }
        if(m_scan.scan_is_time_based)
            m_scanningTimer.start();

        m_scan.current_step++;
    }

}
/**
 * @brief RunControlGUI::prepareAndStartStep
 * @abstract stop the data taking, update the configuration and start a new run
 * @return Returns true if step has been successfull
 */
bool RunControlGUI::prepareAndStartStep()
{
    if(m_scan_active==true) {
        on_btnStop_clicked();
        on_btnReset_clicked();
        while(!allConnectionsInState(eudaq::Status::STATE_UNINIT)){
            std::this_thread::sleep_for (std::chrono::seconds(1));
            cout << "Waiting for reset"<<endl;
        }
        EUDAQ_USER("Resetted");
        std::this_thread::sleep_for (std::chrono::seconds(3));
        on_btnInit_clicked();
        while(!allConnectionsInState(eudaq::Status::STATE_UNCONF)){
            std::this_thread::sleep_for (std::chrono::seconds(1));
            cout << "Waiting for init"<<endl;
        }
        updateInfos();
        std::this_thread::sleep_for (std::chrono::seconds(3));
        EUDAQ_USER("Initialized");
        on_btnConfig_clicked();
        while(!allConnectionsInState(eudaq::Status::STATE_CONF)){
            std::this_thread::sleep_for (std::chrono::seconds(1));
            cout << "Waiting for configuration"<<endl;
        }
        updateInfos();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        EUDAQ_USER("configured");
        on_btnStart_clicked();
        EUDAQ_USER("Running");
        // stop the scan here

    } else {
        on_btnStop_clicked();
        btnStartScan->setText("Start scan");

    }
    return true;
}
/**
 * @brief RunControlGUI::allConnectionsInState
 * @param state to be cheked
 * @return true if all connections are in state, false otherwise
 */
bool RunControlGUI::allConnectionsInState(eudaq::Status::State state){
    std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> map_conn_status;
    if(m_rc)
      map_conn_status= m_rc->GetActiveConnectionStatusMap();
    else
        return false;
    for(auto &conn_status: map_conn_status){
        if(!conn_status.second)
            continue;
        auto state_conn = conn_status.second->GetState();
        if((int)state_conn != (int)state)
            return false;
    }
    return true;
}

/**
 * @brief RunControlGUI::readScanConfig
 * @abstract Read the scan config file and prepare all parameters
 * @return true if sucessfull
 */
bool RunControlGUI::readScanConfig(){
    m_scan_config = eudaq::Configuration::MakeUniqueReadFile(txtScanFile->text().toStdString());

    if(!m_scan_config->HasSection("general"))
    {
        cout << "No general section given in config"<<endl;
        return false;
    }
    m_scan_config->SetSection("general");
    return checkScanParameters();
}
/**
 * @brief RunControlGUI::checkScanParameters
 * @return true if a correct minimal configuration
 * is existing, false otherwise
 * @abstract Check if the configuration file is correctly
 * formatted.
 */
bool RunControlGUI::checkScanParameters(){

    if(!m_scan_config->Has("scans")
            || !m_scan_config->Has("allowNested")
            || !m_scan_config->Has("scans")
            || (! m_scan_config->Has("time") && (! m_scan_config->Has("nevents"))) // either events or time need to be provided
            || !m_scan_config->Has("timeModeActive")
           )
        return false;
    // reset the configs
    m_scan.scanned_parameter.clear();
    m_scan.events_counting_component.clear();
    m_scan.scanned_producer.clear();
    m_scan.config_files.clear();
    m_scan.allow_nested_scan = m_scan_config->Get("allowNested",-1);
    m_scan.n_scans =m_scan_config->Get("scans",-1);
    m_scan.time_per_step = m_scan_config->Get("time",0.0);
    m_scan.events_per_step = m_scan_config->Get("nevents",-1);
    m_scan.scan_is_time_based = m_scan_config->Get("timeModeActive",1);
    m_scanningTimer.setInterval(m_scan.time_per_step*1000);
    m_scan.n_steps = 0;
    for(int i =0; i < m_scan.n_scans ; ++i){
        if(!m_scan_config->HasSection(std::to_string(i)))
            return false;
        m_scan_config->SetSection(std::to_string(i));
        // check if minimal config exists
    if(! m_scan_config->Has("start")
            || ! m_scan_config->Has("stop")
            || ! m_scan_config->Has("step")
            || ! m_scan_config->Has("name")
            || ! m_scan_config->Has("default")

            || ! m_scan_config->Has("parameter"))
        return false;
    if(m_scan.allow_nested_scan==true && m_scan_config->Get("isNested",0))
    m_scan.n_steps += m_scan.n_steps * (1+(m_scan_config->Get("stop",0.0)-m_scan_config->Get("start",0.0))/
                      double(m_scan_config->Get("step",0.0)));
    else
        m_scan.n_steps += 1+(m_scan_config->Get("stop",0.0)-m_scan_config->Get("start",0.0))/
                          double(m_scan_config->Get("step",0.0));
    m_scan.steps_per_scan.push_back(1+(m_scan_config->Get("stop",0.0)-m_scan_config->Get("start",0.0))/
                          double(m_scan_config->Get("step",0.0)));
    }

    createConfigs();
    return true;
}

/**
 * @brief RunControlGUI::createConfigs
 * @abstract create the relevant config files to perform a scan from a scan-config file.
 * Additionally, the settings for each step - runtime or evetns in a certain producer to
 * complete a step.
 *
*/
void RunControlGUI::createConfigs(){

    // start with a default config file
    std::string config = txtConfigFileName->text().toStdString();
    eudaq::ConfigurationSP defaultconf = eudaq::Configuration::MakeUniqueReadFile(config);
    m_scan_config->SetSection("general");
    config = m_scan_config->Get("configpath",config);


    int step = 0;
    for(int i = 0; i< m_scan.n_scans; i++){
        m_scan_config->SetSection(std::to_string(i));
        m_scan.scanned_parameter.push_back(m_scan_config->Get("parameter",""));
        if(!m_scan.allow_nested_scan)
        {
            for(int j = 0; j < m_scan.steps_per_scan.at(i);j++)
            {
                defaultconf->SetSection(m_scan_config->Get("name",""));

                std::string filename = (config.substr(0,config.size()-5)+"_scan_"+std::to_string(step)+".conf");
                if(config != txtConfigFileName->text().toStdString())
                    filename = (config+"_scan_"+std::to_string(step)+".conf");
                m_scan.config_files.push_back(filename);
                m_scan.events_counting_component.push_back(m_scan_config->Get("eventCounter","no processor found"));
                defaultconf->SetString(m_scan.scanned_parameter.at(i),std::to_string(m_scan_config->Get("start",0)
                                                                                     +j*m_scan_config->Get("step",0)));
                // write it all
                std::filebuf fb;
                fb.open (filename,std::ios::out);
                std::ostream os(&fb);
                defaultconf->Save(os);
                fb.close();

                step++;
            }
            // set value to defautl again
            defaultconf->SetString(m_scan.scanned_parameter.at(i),std::to_string(m_scan_config->Get("default",-1)));

        }
        else {
            QMessageBox::warning(NULL, "SCAN CONFIGURATION FAILED",
                                  "A nested scan has been requestet - not yet implemented!");
        return;
        }
    }
    // set number of steps to config files created - might not correspond to the initial guess:
    m_scan.n_steps = m_scan.config_files.size()-1;
}

/**
 * @brief RunControlGUI::checkEventsInStep
 * @abstract check if the reuqested number of events for a certain step is recorded
 * @return true if reached/surpassed, false otherwise
 */
bool RunControlGUI::checkEventsInStep(){
    int events = getEventsCurrent();
    return ( (events > 0 ? events : (m_scan.events_per_step-2))>m_scan.events_per_step);
}

/**
 * @brief RunControlGUI::getEventsCurrent
 * @return Number of events in current step of scans
 */

int RunControlGUI::getEventsCurrent(){
    std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> map_conn_status;
    if(m_scan.events_counting_component.size()>m_scan.current_step-1)
        return m_scan.events_per_step+1;
    if(m_rc)
        map_conn_status= m_rc->GetActiveConnectionStatusMap();
    else
        return -2;
    for(auto conn : map_conn_status) {
        if((conn.first->GetType()+"."+conn.first->GetName())==m_scan.events_counting_component.at(m_scan.current_step)){
            auto tags = conn.second->GetTags();
            for(auto &tag: tags)
                if(tag.first=="EventN")
                    return std::stoi(tag.second);
        }
    }
    return -1;
}

void RunControlGUI::updateProgressBar(){
    double scanProgress = 0;
    if(m_scan_active){
        scanProgress = (m_scan.current_step-1)/double(std::max(1,m_scan.n_steps))*100;
    if(m_scan.scan_is_time_based)
        scanProgress+= ((m_scanningTimer.interval()-m_scanningTimer.remainingTime())/double(std::max(1,m_scanningTimer.interval())) *100./std::max(1,m_scan.n_steps));
    else
        scanProgress += getEventsCurrent()/double(m_scan.events_per_step)*100./std::max(1,m_scan.n_steps);
    }
    progressBar_scan->setValue(scanProgress);

}
