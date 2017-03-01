#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRun.hh"
#include "Colours.hh"
#include "config.h" // for version symbols

namespace{
  auto dummy0 = eudaq::Factory<eudaq::RunControl>::
    Register<RunControlGUI, const std::string&>(eudaq::cstr2hash("RunControlGUI"));
  auto dummy1 = eudaq::Factory<eudaq::RunControl>::
    Register<RunControlGUI, const std::string&>(eudaq::cstr2hash("GuiRunControl"));
}

static const char *statuses[] = {
  "RUN",       "Run Number", "EVENT",    "Events Built", "FULLRATE",
  "Rate",      "TRIG",       "Triggers", "FILEBYTES",    "File Bytes",
  "PARTICLES", "Particles",  "TLUSTAT",  "TLU Status",   "SCALERS",
  "Scalers",   0};

RunConnectionDelegate::RunConnectionDelegate(RunControlModel *model)
  : m_model(model){}

void RunConnectionDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
}

RunControlGUI::RunControlGUI(const std::string &listenaddress)
  : QMainWindow(0, 0), eudaq::RunControl(listenaddress),
  m_delegate(&m_run), m_state(STATE_UNINIT), m_prevtrigs(0), m_prevtime(0.0), m_runstarttime(0.0),
  m_filebytes(0), m_events(0){

  qRegisterMetaType<QModelIndex>("QModelIndex");
  
  setupUi(this);

  QRect geom(-1,-1, 150, 200);
  if (!grpStatus->layout())
    grpStatus->setLayout(new QGridLayout(grpStatus));
  lblCurrent->setText(QString("<font size=%1 color='red'> <b> Current State: Uninitilised </b. </font>").arg(FONT_SIZE));
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
    if (++col > 1) {
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

  // setWindowIcon(QIcon("../images/Icon_euRun.png"));
  setWindowTitle("eudaq Run Control " PACKAGE_VERSION);

  connect(this, SIGNAL(StatusChanged(const QString &, const QString &)), this,
	  SLOT(ChangeStatus(const QString &, const QString &)));
  connect(&m_timer_display, SIGNAL(timeout()), this, SLOT(DisplayTimer()));
  connect(&m_timer_autorun, SIGNAL(timeout()), this, SLOT(AutorunTimer()));
  m_timer_display.start(500);
}

void RunControlGUI::DoStatus(eudaq::ConnectionSPC id,
			     std::shared_ptr<const eudaq::Status> status) {
  if (id->GetType() == "DataCollector") {
    m_filebytes = from_string(status->GetTag("FILEBYTES"), 0LL);
    m_events = from_string(status->GetTag("EVENT"), 0LL);
    emit StatusChanged("EVENT", QString::number(m_events));
    emit StatusChanged("FILEBYTES", QString::number(m_filebytes));
  } else if (id->GetType() == "Producer") {
    if (id->GetName().find("TLU") != std::string::npos) {
      emit StatusChanged("TRIG", status->GetTag("TRIG").c_str());
      emit StatusChanged("PARTICLES", status->GetTag("PARTICLES").c_str());
      emit StatusChanged("TIMESTAMP", status->GetTag("TIMESTAMP").c_str());
      emit StatusChanged("LASTTIME", status->GetTag("LASTTIME").c_str());
      emit StatusChanged("TLUSTAT", status->GetTag("STATUS").c_str());
      bool ok = true;
      std::string scalers;
      for (int i = 0; i < 4; ++i) {
	std::string s = status->GetTag("SCALER" + std::to_string(i));
	if (s == "") {
          ok = false;
          break;
	}
	if (scalers != "")
	  scalers += ", ";
	scalers += s;
      }
      if (ok)
	emit StatusChanged("SCALERS", scalers.c_str());
      int trigs = from_string(status->GetTag("TRIG"), -1);
      double time = from_string(status->GetTag("TIMESTAMP"), 0.0);
      if(trigs >= 0) {
	bool dorate = true;
	if (m_runstarttime == 0.0) {
	  if (trigs > 0)
	    m_runstarttime = time;
	  dorate = false;
	} else {
	  emit StatusChanged("MEANRATE",
			     QString::number((trigs - 1) / (time - m_runstarttime)) + " Hz");
	}
	int dtrigs = trigs - m_prevtrigs;
	double dtime = time - m_prevtime;
	if(dtrigs >= 10 || dtime >= 1.0) {
	  m_prevtrigs = trigs;
	  m_prevtime = time;
	  emit StatusChanged("RATE", QString::number(dtrigs / dtime) + " Hz");
	} else {
	  dorate = false;
	}
	if(dorate) {
	  emit StatusChanged("FULLRATE",
			     QString::number((trigs - 1) / (time - m_runstarttime)) + " (" +
			     QString::number(dtrigs / dtime) + ") Hz");
	}
      }
    }
  }
  m_run.SetStatus(id, *status);
}

void RunControlGUI::DoConnect(eudaq::ConnectionSPC id) {
  if (id->GetType() == "LogCollector") {
    btnLog->setEnabled(true);
  }
  m_run.newconnection(id);
}


void RunControlGUI::DoDisconnect(eudaq::ConnectionSPC id){
  m_run.disconnected(id);
}

void RunControlGUI::ChangeStatus(const QString &name, const QString &value){
  auto i = m_status.find(name.toStdString());
  if (i != m_status.end()) {
    i->second->setText(value);
  }
}

void RunControlGUI::on_btnInit_clicked(){
  std::string settings = txtInitFileName->text().toStdString();
  std::cout<<settings<<std::endl;
  ReadInitilizeFile(settings);
  Initialise();
}

void RunControlGUI::on_btnTerminate_clicked(){
  close();
}

void RunControlGUI::on_btnConfig_clicked(){
  std::string settings = txtConfigFileName->text().toStdString();
  ReadConfigureFile(settings);
  m_runeventlimit = GetConfiguration()->Get("RunEventSizeLimit", 0);
  Configure();
}

void RunControlGUI::on_btnStart_clicked(){
  m_prevtrigs = 0;
  m_prevtime = 0.0;
  m_runstarttime = 0.0;
  StartRun();
  emit StatusChanged("RUN", QString::number(GetRunNumber()));
  emit StatusChanged("EVENT", "0");
  emit StatusChanged("TRIG", "0");
  emit StatusChanged("PARTICLES", "0");
  emit StatusChanged("RATE", "");
  emit StatusChanged("MEANRATE", "");
}

void RunControlGUI::on_btnStop_clicked() {
  StopRun();
  emit StatusChanged("RUN", "("+QString::number(GetRunNumber())+")");
}

void RunControlGUI::on_btnReset_clicked() {
  Reset();
  emit StatusChanged("RUN", "("+QString::number(GetRunNumber())+")");
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
  updateTopbar();
  m_run.UpdateDisplayed();
}

void RunControlGUI::AutorunTimer(){
  if ((m_runeventlimit >= 2 && m_events >= m_runeventlimit)){
    EUDAQ_INFO("Event limit per run reached: " + std::to_string(m_events) + " > " +
	       std::to_string(m_runeventlimit));
    on_btnStop_clicked();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    on_btnConfig_clicked();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    on_btnStart_clicked();
  }
}

void RunControlGUI::updateButtons() {
  auto confLoaded = checkConfigFile();
  auto initLoaded = checkInitFile();
  btnInit->setEnabled(m_state == STATE_UNINIT && initLoaded);
  btnConfig->setEnabled(m_state == STATE_UNCONF && m_state == STATE_CONF
			&& confLoaded);
  
  btnLoadInit->setEnabled(m_state != STATE_RUNNING);
  btnLoadConf->setEnabled(m_state != STATE_RUNNING);
  btnStart->setEnabled(m_state == STATE_CONF);
  btnStop->setEnabled(m_state == STATE_RUNNING);
  btnReset->setEnabled(m_state != STATE_RUNNING);
  btnTerminate->setEnabled(m_state != STATE_RUNNING);
}

void RunControlGUI::updateTopbar(){
  if(m_state == STATE_UNINIT){
    lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Uninitialised </b><\
/font>").arg(FONT_SIZE));
  }
  else if(m_state == STATE_UNCONF) {
    lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Unconfigured </b></\
font>").arg(FONT_SIZE));
  } else if (m_state == STATE_CONF) {
    lblCurrent->setText(QString("<font size=%1 color='orange'><b>Current State: Configured </b><\
/font>").arg(FONT_SIZE));
  }
  else if (m_state ==STATE_RUNNING)
    lblCurrent->setText(QString("<font size=%1 color='green'><b>Current State: Running </b></fon\
t>").arg(FONT_SIZE));
  else
    lblCurrent->setText(QString("<font size=%1 color='darkred'><b>Current State: Error </b></fon\
t>").arg(FONT_SIZE));
}

void RunControlGUI::closeEvent(QCloseEvent *event) {
  if (m_run.rowCount() > 0 &&
      QMessageBox::question(this, "Quitting",
			    "Terminate all connections and quit?",
			    QMessageBox::Ok | QMessageBox::Cancel)
      == QMessageBox::Cancel) {
    event->ignore();
  } else {
    Terminate();
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

RunControlGUI::~RunControlGUI() {
  QSettings settings("EUDAQ collaboration", "EUDAQ");   
  settings.beginGroup("euRun2");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.setValue("lastConfigFile", txtConfigFileName->text());
  settings.setValue("lastInitFile", txtInitFileName->text());
  settings.endGroup();
}

void RunControlGUI::Exec(){
  show();
  StartRunControl();
  if(QApplication::instance())
    QApplication::instance()->exec(); 
  else
    std::cerr<<"ERROR: RUNContrlGUI::EXEC\n";
   
  while(IsActiveRunControl()){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
