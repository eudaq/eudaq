#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "myeuRun.hh"
#include "Colours.hh"
#include "eudaq/Config.hh"

#include <iostream>

myRunControlGUI::myRunControlGUI()
  : QMainWindow(0, 0){
  /*
    map ordering by the key (number, alphabet, capitalized etc)
    thus showing up to the window as widget
   */
  m_map_label_str = {{"3RunRate", "Run Rate"},
		     {"1RUN",     "Run Number"},
		     {"2EventN",  "Event#"},
		     {"4DtEvt",   "Data/Event"},
		     {"5ConfInfo","Configuration Tab"}};
  
  qRegisterMetaType<QModelIndex>("QModelIndex");
  setupUi(this);

  if (!grpStatus->layout())
    grpStatus->setLayout(new QGridLayout(grpStatus));
  lblCurrent->setText(m_map_state_str.at(eudaq::Status::STATE_UNINIT));
  QGridLayout *layout = dynamic_cast<QGridLayout *>(grpStatus->layout());
  int row = 0, col = 0;
  int counter=0;
  for(auto &label_str: m_map_label_str) {
    printf("\n[dev] give me a counter == %d\n", counter++);
    std::cout<<"\tname = "
	     << label_str.first.toUtf8().constData()
	     <<"; to_show = "
	     << label_str.second.toUtf8().constData()
	     <<std::endl;
    
    QLabel *lblname = new QLabel(grpStatus);
    lblname->setObjectName("lbl_st_" + label_str.first);
    lblname->setText(label_str.second + ": ");
    QLabel *lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName("txt_st_" + label_str.first);
    layout->addWidget(lblname, row, col * 2);
    layout->addWidget(lblvalue, row, col * 2 + 1);
    m_str_label[label_str.first] = lblvalue;
    if (++col > 1){
      ++row;
      col = 0;
    }
  }
  
  viewConn->setModel(&m_model_conns);
  viewConn->setItemDelegate(&m_delegate);
  
  QRect geom(-1,-1, 150, 200);
  QRect geom_from_last_program_run;
  QSettings settings("EUDAQ collaboration", "EUDAQ");
  settings.beginGroup("myeuRun2");
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
  m_timer_display.start(500);
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
  settings_output.beginGroup("myeuRun2");
  settings_output.setValue("successexit", 0);
  settings_output.endGroup();
}

void myRunControlGUI::SetInstance(eudaq::RunControlUP rc){
  m_rc = std::move(rc);
  if(m_lastexit_success)
    m_rc->SetRunN(m_run_n_qsettings);
  else
    m_rc->SetRunN(m_run_n_qsettings+1);
  auto thd_rc = std::thread(&eudaq::RunControl::Exec, m_rc.get());
  thd_rc.detach();
}

// void myRunControlGUI::on_btn


void myRunControlGUI::on_btnInit_clicked(){
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

void myRunControlGUI::on_btnTerminate_clicked(){
  close();
}

void myRunControlGUI::on_btnConfig_clicked(){
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
}

void myRunControlGUI::on_btnStart_clicked(){
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

void myRunControlGUI::on_btnStop_clicked() {
  if(m_rc)
    m_rc->StopRun();
}

void myRunControlGUI::on_btnReset_clicked() {
  if(m_rc)
    m_rc->Reset();
}

void myRunControlGUI::on_btnLog_clicked() {
  std::string msg = txtLogmsg->displayText().toStdString();
  EUDAQ_USER(msg);
}

void myRunControlGUI::on_btnLoadInit_clicked() {
  QString usedpath =QFileInfo(txtInitFileName->text()).path();
  QString filename =QFileDialog::getOpenFileName(this, tr("Open File"),
						 usedpath,
						 tr("*.ini (*.ini)"));
  if (!filename.isNull()){
    txtInitFileName->setText(filename);
  }
}

void myRunControlGUI::on_btnLoadConf_clicked() {
  QString usedpath =QFileInfo(txtConfigFileName->text()).path();
  QString filename =QFileDialog::getOpenFileName(this, tr("Open File"),
						 usedpath,
						 tr("*.conf (*.conf)"));
  if (!filename.isNull()) {
    txtConfigFileName->setText(filename);
  }
}

void myRunControlGUI::DisplayTimer(){
  std::map<eudaq::ConnectionSPC, eudaq::StatusSPC> map_conn_status;
  auto state = eudaq::Status::STATE_RUNNING;
  if(m_rc)
    map_conn_status= m_rc->GetActiveConnectionStatusMap();

  /* wmq dev: 
   * print out status map to test
   */
  std::string tagkey="Run Rate";
  if (m_rc && false){
    for (const auto &itr1: map_conn_status){
      std::ostream & objOstream = std::cout;
      objOstream << "*^* Map Win:";
      itr1.first->Print(objOstream);
      objOstream << "\n";

      if (!itr1.second) objOstream << "\tNo status value in map!";
      else{
	//objOstream<<itr1.second.get();
	auto getTheTag = itr1.second->GetTag(tagkey);
	objOstream << getTheTag;
	//itr1.second->Print(objOstream);
      }
      objOstream << "\n";
    }
  }
  /*end of wmq-dev*/
  
  
  for(auto &conn_status_last: m_map_conn_status_last){
    if(!map_conn_status.count(conn_status_last.first)){
      m_model_conns.disconnected(conn_status_last.first);
    }
  }
  for(auto &conn_status: map_conn_status){
    if(!m_map_conn_status_last.count(conn_status.first)){
      m_model_conns.newconnection(conn_status.first);
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
    settings.beginGroup("myeuRun2");
    settings.setValue("runnumber", m_run_n_qsettings);
    settings.endGroup();
  }
  
  if(m_rc&&m_str_label.count("1RUN")){
    if(state == eudaq::Status::STATE_RUNNING){
      m_str_label.at("1RUN")->setText(QString::number(run_n));
    }
    else{
      m_str_label.at("1RUN")->setText(QString::number(run_n)+" (next run)");
    }
  }

  /* start of wmq-dev */
  if (m_rc && true) {
    for (auto &itr1: map_conn_status){
      /*TODO: check if you retrieving the correct tags from TCP of the kpixProducer*/
      if (!itr1.second) continue;

      if (m_str_label.count("2EventN") && state==eudaq::Status::STATE_RUNNING) {
	auto ev_n = itr1.second->GetTag("EventN");
	m_str_label.at("2EventN")->setText( QString::fromUtf8(ev_n.c_str()) );
      }
      if (m_str_label.count("3RunRate") && state!= eudaq::Status::STATE_RUNNING){
	/*No need to check run rate when running, as read from config*/
	auto runrate = itr1.second->GetTag("Run Rate");
	m_str_label.at("3RunRate")->setText( QString::fromUtf8(runrate.c_str()) );
      }
      if (m_str_label.count("4DtEvt")){
	/*Always check*/
	auto dtevt = itr1.second->GetTag("Data/Event");
	m_str_label.at("4DtEvt")->setText( QString::fromUtf8(dtevt.c_str()) );
      } 
      if (m_str_label.count("5ConfInfo")&& ( state == eudaq::Status::STATE_CONF)){
	/*Check only when configured*/
	auto confinfo = itr1.second->GetTag("Configuration Tab");
	  m_str_label.at("5ConfInfo")->setText( QString::fromUtf8(confinfo.c_str()) );
      } 
    }
  }
  /* end of wmq-dev */

  m_map_conn_status_last = map_conn_status;
}

void myRunControlGUI::closeEvent(QCloseEvent *event) {
  if (QMessageBox::question(this, "Quitting",
			    "Terminate all connections and quit?",
			    QMessageBox::Ok | QMessageBox::Cancel)
      == QMessageBox::Cancel){
    event->ignore();
  } else {
    QSettings settings("EUDAQ collaboration", "EUDAQ");  
    settings.beginGroup("myeuRun2");
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

void myRunControlGUI::Exec(){
  show();
  if(QApplication::instance())
    QApplication::instance()->exec(); 
  else
    std::cerr<<"ERROR: RUNContrlGUI::EXEC\n";   
}


std::map<int, QString> myRunControlGUI::m_map_state_str ={
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
