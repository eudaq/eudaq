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
  QFileInfo check_file(txtInitFileName->text());
  if(!check_file.exists() || !check_file.isFile()){
    QMessageBox::warning(NULL, "ERROR", "Init file does not exist.");
    return;
  }
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
  QFileInfo check_file(txtConfigFileName->text());
  if(!check_file.exists() || !check_file.isFile()){
    QMessageBox::warning(NULL, "ERROR", "Config file does not exist.");
    return;
  }
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
      switch(state_conn){
      case eudaq::Status::STATE_ERROR:{
    state = eudaq::Status::STATE_ERROR;
    break;
      }
      case eudaq::Status::STATE_UNINIT:{
    if(state != eudaq::Status::STATE_ERROR){
      state = eudaq::Status::STATE_UNINIT;
    }
    break;
      }
      case eudaq::Status::STATE_UNCONF:{
    if(state != eudaq::Status::STATE_ERROR &&
       state != eudaq::Status::STATE_UNINIT){
      state = eudaq::Status::STATE_UNCONF;
    }
    break;
      }
      case eudaq::Status::STATE_CONF:{
    if(state != eudaq::Status::STATE_ERROR &&
       state != eudaq::Status::STATE_UNINIT &&
       state != eudaq::Status::STATE_UNCONF){
      state = eudaq::Status::STATE_CONF;
    }
    break;
      }
      }
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
  btnStop->setEnabled(state == eudaq::Status::STATE_RUNNING);
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
  updateStatusDisplay();
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
        QMessageBox::warning(NULL,"ERROR - Status display","Duplicating display entry request: "+objectName);
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
    }
    else {
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
