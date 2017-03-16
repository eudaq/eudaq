#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRun.hh"
#include "Colours.hh"
#include "config.h" // for version symbols

static const char *statuses[] = {
  "RUN",       "Run Number", "EVENT",    "Events Built", "FULLRATE",
  "Rate",      "TRIG",       "Triggers", "FILEBYTES",    "File Bytes",
  "PARTICLES", "Particles",  "TLUSTAT",  "TLU Status",   "SCALERS",
  "Scalers",   0};

RunControlGUI::RunControlGUI()
  : QMainWindow(0, 0),m_state(eudaq::Status::STATE_UNINIT){
    
  qRegisterMetaType<QModelIndex>("QModelIndex");
  
  setupUi(this);

  QRect geom(-1,-1, 150, 200);
  if (!grpStatus->layout())
    grpStatus->setLayout(new QGridLayout(grpStatus));
  lblCurrent->setText(m_map_state_str.at(m_state));
  QGridLayout *layout = dynamic_cast<QGridLayout *>(grpStatus->layout());
  int row = 0, col = 0;
  for (const char **st = statuses; st[0] && st[1]; st += 2) {
    QLabel *lblname = new QLabel(grpStatus);
    lblname->setObjectName(QString("lbl_st_") + st[0]);
    lblname->setText((std::string(st[1]) + ": ").c_str());
    QLabel *lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName(QString("txt_st_") + st[0]);
    layout->addWidget(lblname, row, col * 2);
    layout->addWidget(lblvalue, row, col * 2 + 1);
    m_status[st[0]] = lblvalue;
    if (++col > 1){
      ++row;
      col = 0;
    }
  }
  viewConn->setModel(&m_run);
  viewConn->setItemDelegate(&m_delegate);
  
  QRect geom_from_last_program_run;
  QSettings settings("EUDAQ collaboration", "EUDAQ");
  settings.beginGroup("euRun2");
  geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
  geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
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
  m_timer_display.start(500);
}

void RunControlGUI::SetInstance(eudaq::RunControlUP rc){
  m_rc = std::move(rc);
}

// void RunControlGUI::DoStatus(eudaq::ConnectionSPC id, eudaq::StatusSPC status){
//   m_run.SetStatus(id, status);
// }

// void RunControlGUI::DoConnect(eudaq::ConnectionSPC id) {
//   if (id->GetType() == "LogCollector"){
//     btnLog->setEnabled(true);
//   }
//   m_run.newconnection(id);
// }


// void RunControlGUI::DoDisconnect(eudaq::ConnectionSPC id){
//   m_run.disconnected(id);
// }

// void RunControlGUI::ChangeStatus(const QString &name, const QString &value){
//   auto i = m_status.find(name.toStdString());
//   if (i != m_status.end()) {
//     i->second->setText(value);
//   }
// }

void RunControlGUI::on_btnInit_clicked(){
  std::string settings = txtInitFileName->text().toStdString();
  std::cout<<settings<<std::endl;
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
  if(m_rc){
    m_rc->ReadConfigureFile(settings);
    m_rc->Configure();
  }
}

void RunControlGUI::on_btnStart_clicked(){
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
						 tr("*.init.conf (*.init.conf)"));
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
  updateButtons();
  lblCurrent->setText(m_map_state_str.at(m_state));

  // m_run.SetStatus(id, status);
  
  m_run.UpdateDisplayed();
}

void RunControlGUI::updateButtons() {
  auto confLoaded = checkConfigFile();
  auto initLoaded = checkInitFile();
  btnInit->setEnabled(m_state == eudaq::Status::STATE_UNINIT && initLoaded);
  btnConfig->setEnabled(m_state == eudaq::Status::STATE_UNCONF &&
			m_state == eudaq::Status::STATE_CONF &&
			confLoaded);
  
  btnLoadInit->setEnabled(m_state != eudaq::Status::STATE_RUNNING);
  btnLoadConf->setEnabled(m_state != eudaq::Status::STATE_RUNNING);
  btnStart->setEnabled(m_state == eudaq::Status::STATE_CONF);
  btnStop->setEnabled(m_state == eudaq::Status::STATE_RUNNING);
  btnReset->setEnabled(m_state != eudaq::Status::STATE_RUNNING);
  btnTerminate->setEnabled(m_state != eudaq::Status::STATE_RUNNING);
}

void RunControlGUI::closeEvent(QCloseEvent *event) {
  if (m_run.rowCount() > 0 &&
      QMessageBox::question(this, "Quitting",
			    "Terminate all connections and quit?",
			    QMessageBox::Ok | QMessageBox::Cancel)
      == QMessageBox::Cancel){
    event->ignore();
  } else {
    QSettings settings("EUDAQ collaboration", "EUDAQ");  
    settings.beginGroup("euRun2");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.setValue("lastConfigFile", txtConfigFileName->text());
    settings.setValue("lastInitFile", txtInitFileName->text());
    settings.endGroup();
    if(m_rc)
      m_rc->Terminate();
    event->accept();
  }
}

bool RunControlGUI::checkInitFile(){
  QString loadedFile = txtInitFileName->text();
  if(loadedFile.isNull())
    return false;
  QRegExp rx(".+(\\.init\\.conf$)");
  return rx.exactMatch(loadedFile);
}

bool RunControlGUI::checkConfigFile() {
  QString loadedFile = txtConfigFileName->text();
  if(loadedFile.isNull())
    return false;
  QRegExp rx (".+(\\.conf$)");
  return rx.exactMatch(loadedFile);
}

void RunControlGUI::Exec(){
  std::cout<<">>>>"<<std::endl;
  //TODO: check dump reason
  show();
  std::cout<<">>>>"<<std::endl;
  // if(m_rc)
  //   m_rc->StartRunControl();
  if(QApplication::instance())
    QApplication::instance()->exec(); 
  else
    std::cerr<<"ERROR: RUNContrlGUI::EXEC\n";   
}
