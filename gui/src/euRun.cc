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

namespace {
  static const char *GEOID_FILE = "GeoID.dat";
}
RunConnectionDelegate::RunConnectionDelegate(RunControlModel *model)
  : m_model(model) {}

void RunConnectionDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
  // painter->save();
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  // painter->restore();
}

RunControlGUI::RunControlGUI(const std::string &listenaddress,
                             QWidget *parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags), eudaq::RunControl(listenaddress),
  m_delegate(&m_run), m_prevtrigs(0), m_prevtime(0.0), m_runstarttime(0.0),
  m_filebytes(0), m_events(0), dostatus(false),
    m_startrunwhenready(false),m_lastconfigonrunchange(false), m_data_taking(false), configLoaded(false), configLoadedInit(false), disableButtons(false), curState(STATE_UNINIT),lastUsedDirectory(""),lastUsedDirectoryInit(""),  m_nextconfigonrunchange(false) {  
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
  
  settings.beginGroup("MainWindowEuRun");
  geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
  geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
  lastUsedDirectory =
    settings.value("lastConfigFileDirectory", "").toString();
  txtConfigFileName
    ->setText(settings.value("lastConfigFile", "config file not set").toString());
  settings.endGroup();
  
  QSize fsize = frameGeometry().size();
  if ((geom.x() == -1)||(geom.y() == -1)||(geom.width() == -1)||(geom.height() == -1)) {
    if ((geom_from_last_program_run.x() == -1)||(geom_from_last_program_run.y() == -1)||(geom_from_last_program_run.width() == -1)||(geom_from_last_program_run.height() == -1)) {
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

  connect(this, SIGNAL(StatusChanged(const QString &, const QString &)), this,
          SLOT(ChangeStatus(const QString &, const QString &)));
  connect(&m_statustimer, SIGNAL(timeout()), this, SLOT(timer()));
  connect(this, SIGNAL(btnLogSetStatus(bool)), this,
          SLOT(btnLogSetStatusSlot(bool)));
  connect(this, SIGNAL(SetState(int)), this, SLOT(SetStateSlot(int)));
  m_statustimer.start(500);
  txtGeoID->setText(QString::number(eudaq::ReadFromFile(GEOID_FILE, 0U)));
  txtGeoID->installEventFilter(this);
  setWindowIcon(QIcon("../images/Icon_euRun.png"));
  setWindowTitle("eudaq Run Control " PACKAGE_VERSION);
}

void RunControlGUI::DoStatus(eudaq::ConnectionSPC id,
			     std::shared_ptr<const eudaq::Status> status) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  if (id->GetType() == "DataCollector") {
    m_filebytes = from_string(status->GetTag("FILEBYTES"), 0LL);
    m_events = from_string(status->GetTag("EVENT"), 0LL);
    EmitStatus("EVENT", status->GetTag("EVENT"));
    EmitStatus("FILEBYTES", to_bytes(status->GetTag("FILEBYTES")));
  } else if (id->GetType() == "Producer") {
    if (id->GetName() == "TLU" || id->GetName() == "miniTLU") {
      EmitStatus("TRIG", status->GetTag("TRIG"));
      EmitStatus("PARTICLES", status->GetTag("PARTICLES"));
      EmitStatus("TIMESTAMP", status->GetTag("TIMESTAMP"));
      EmitStatus("LASTTIME", status->GetTag("LASTTIME"));
      EmitStatus("TLUSTAT", status->GetTag("STATUS"));
      bool ok = true;
      std::string scalers;
      for (int i = 0; i < 4; ++i) {
	std::string s = status->GetTag("SCALER" + to_string(i));
	if (s == "") {
          ok = false;
          break;
	}
	if (scalers != "")
	  scalers += ", ";
	scalers += s;
      }
      if (ok)
	EmitStatus("SCALERS", scalers);
      int trigs = from_string(status->GetTag("TRIG"), -1);
      double time = from_string(status->GetTag("TIMESTAMP"), 0.0);
      if (trigs >= 0) {
	bool dorate = true;
	if (m_runstarttime == 0.0) {
	  if (trigs > 0)
	    m_runstarttime = time;
	  dorate = false;
	} else {
	  EmitStatus("MEANRATE",
		     to_string((trigs - 1) / (time - m_runstarttime)) + " Hz");
	}
	int dtrigs = trigs - m_prevtrigs;
	double dtime = time - m_prevtime;
	if (dtrigs >= 10 || dtime >= 1.0) {
	  m_prevtrigs = trigs;
	  m_prevtime = time;
	  EmitStatus("RATE", to_string(dtrigs / dtime) + " Hz");
	} else {
	  dorate = false;
	}
	if (dorate) {
	  EmitStatus("FULLRATE",
		     to_string((trigs - 1) / (time - m_runstarttime)) + " (" +
		     to_string(dtrigs / dtime) + ") Hz");
	}
      }
    }
  }
  m_run.SetStatus(id, *status);
}

void RunControlGUI::DoConnect(eudaq::ConnectionSPC id) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  m_run.newconnection(id);
  if (id->GetType() == "DataCollector") {
    EmitStatus("RUN", "(" + to_string(GetRunNumber()) + ")");
  }
  if (id->GetType() == "LogCollector") {
    btnLogSetStatus(true);
  }
}


void RunControlGUI::DoDisconnect(eudaq::ConnectionSPC id){
  m_run.disconnected(id);
}


void RunControlGUI::EmitStatus(const char *name, const std::string &val) {
  if (val == "")
    return;
  emit StatusChanged(name, val.c_str());
}

void RunControlGUI::SetStateSlot(int state) {
  curState=state; 
  updateButtons(state);  
  if(state == STATE_UNINIT) {
    lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Uninitialised </b><\
/font>").arg(FONT_SIZE));
  }
  else if(state == STATE_UNCONF) {
    lblCurrent->setText(QString("<font size=%1 color='red'><b>Current State: Unconfigured </b></\
font>").arg(FONT_SIZE));
  } else if (state == STATE_CONF) {
    lblCurrent->setText(QString("<font size=%1 color='orange'><b>Current State: Configured </b><\
/font>").arg(FONT_SIZE));
  }
  else if (state ==STATE_RUNNING)
    lblCurrent->setText(QString("<font size=%1 color='green'><b>Current State: Running </b></fon\
t>").arg(FONT_SIZE));
  else
    lblCurrent->setText(QString("<font size=%1 color='darkred'><b>Current State: Error </b></fon\
t>").arg(FONT_SIZE));
  m_run.UpdateDisplayed();
}

void RunControlGUI::on_btnInit_clicked() {
  std::string settings = txtInitFileName->text().toStdString();
  ReadInitilizeFile(settings);
  Initialise();
}

void RunControlGUI::on_btnTerminate_clicked(){
  close();
}

void RunControlGUI::on_btnConfig_clicked(){
  std::string settings = txtConfigFileName->text().toStdString();
  ReadConfigureFile(settings);
  m_runsizelimit = GetConfiguration()->Get("RunSizeLimit", 0);
  m_runeventlimit = GetConfiguration()->Get("RunEventSizeLimit", 0);
  Configure();
  dostatus = true;
}

void RunControlGUI::on_btnStart_clicked(bool cont){
  disableButtons=true;   
  m_prevtrigs = 0;
  m_prevtime = 0.0;
  m_runstarttime = 0.0;
  StartRun();
  m_data_taking = true;
  EmitStatus("RUN", to_string(GetRunNumber()));
  emit StatusChanged("EVENT", "0");
  emit StatusChanged("TRIG", "0");
  emit StatusChanged("PARTICLES", "0");
  emit StatusChanged("RATE", "");
  emit StatusChanged("MEANRATE", "");
}

void RunControlGUI::on_btnStop_clicked() {
  m_data_taking = false;
  StopRun();
  EmitStatus("RUN", "(" + to_string(GetRunNumber()) + ")");
}

void RunControlGUI::on_btnLog_clicked() {
  std::string msg = txtLogmsg->displayText().toStdString();
  EUDAQ_USER(msg);
}

void RunControlGUI::on_btnLoadInit_clicked() {
  QString temporaryFileNameInit =
    QFileDialog::getOpenFileName(this, tr("Open File"), lastUsedDirectoryInit, tr("*.init (*.init)"));
  if (!temporaryFileNameInit.isNull()) {
    txtInitFileName->setText(temporaryFileNameInit);
    lastUsedDirectoryInit =
      QFileInfo(temporaryFileNameInit).path(); // store path for next time                      
    configLoadedInit = true;
    updateButtons(curState);
  }
}

void RunControlGUI::on_btnLoad_clicked() {
  QString temporaryFileName =
    QFileDialog::getOpenFileName(this, tr("Open File"), lastUsedDirectory, tr("*.conf (*.conf)"));

  if (!temporaryFileName.isNull()) {
    txtConfigFileName->setText(temporaryFileName);
    lastUsedDirectory =
      QFileInfo(temporaryFileName).path(); // store path for next time
    configLoaded=true;
    updateButtons(curState);
  }
}

void RunControlGUI::timer() {
  if (m_data_taking) {
    if ((m_runsizelimit >= 1024 && m_filebytes >= m_runsizelimit) ||
	(m_runeventlimit >= 1 && m_events >= m_runeventlimit)) {
      if (m_runsizelimit >= 1024 && m_filebytes >= m_runsizelimit) {
	EUDAQ_INFO("File limit reached: " + to_string(m_filebytes) + " > " +
		   to_string(m_runsizelimit));
      } else {
	EUDAQ_INFO("Event limit reached: " + to_string(m_events) + " > " +
		   to_string(m_runeventlimit));
      }
      eudaq::mSleep(1000);
      if(m_lastconfigonrunchange) {
	EUDAQ_INFO("All config files processed.");
	m_startrunwhenready = false;
	on_btnStop_clicked();
	return;
      } else {	
	StopRun();
      }
      eudaq::mSleep(20000); 
      if (m_nextconfigonrunchange) {
	QDir dir(lastUsedDirectory, "*.conf");
	for (size_t i = 0; i < dir.count(); ++i) {
	  QString item = dir[i];
	  allConfigFiles.append(dir.absoluteFilePath(item));
	}

	if (allConfigFiles.indexOf( QRegExp("^" + QRegExp::escape(txtConfigFileName->text()))) + 1 <
	    allConfigFiles.count()) {
	  EUDAQ_INFO("Moving to next config file and starting a new run");
	  txtConfigFileName->
	    setText(allConfigFiles.at(allConfigFiles.indexOf(QRegExp("^" + QRegExp::escape(txtConfigFileName->text())))+1));
	  on_btnConfig_clicked();
	  if(!m_nextconfigonrunchange) {
	    m_lastconfigonrunchange=true;
	  }
	  eudaq::mSleep(1000);
	  m_startrunwhenready = true;
	} else
	  EUDAQ_INFO("All config files processed.");
      } else
	on_btnStart_clicked(true);
    } else if (dostatus) {
      RemoteStatus();
    }
  }
  if (m_startrunwhenready) {
    m_startrunwhenready = false;
    on_btnStart_clicked(false);
  }
}

void RunControlGUI::ChangeStatus(const QString &name, const QString &value) {
  status_t::iterator i = m_status.find(name.toStdString());
  if (i != m_status.end()) {
    i->second->setText(value);
  }
}

void RunControlGUI::btnLogSetStatusSlot(bool status) {
  btnLog->setEnabled(status);
}

void RunControlGUI::btnStartSetStatusSlot(bool status) {
  btnStart->setEnabled(status);
}

void RunControlGUI::closeEvent(QCloseEvent *event) {
  if (m_run.rowCount() > 0 &&
      QMessageBox::question(this, "Quitting", "Terminate all connections and quit?",
			    QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
    event->ignore();
  } else {
    Terminate();
    event->accept();
  }
}

bool RunControlGUI::eventFilter(QObject *object, QEvent *event) {
  if (object == txtGeoID && event->type() == QEvent::MouseButtonDblClick) {
    int oldid = txtGeoID->text().toInt();
    bool ok = false;
    int newid = QInputDialog::getInt(this, "Increment GeoID to:", "value",
                                     oldid + 1, 0, 2147483647, 1, &ok);
    if (ok) {
      txtGeoID->setText(QString::number(newid));
      eudaq::WriteToFile(GEOID_FILE, newid);
    }
    return true;
  }
  return false;
}

bool RunControlGUI::checkInitFile(){
  QString loadedFile = txtInitFileName->text();
  if(loadedFile.isNull())
    return false;
  QRegExp rx(".+(\\.init$)");
  return rx.exactMatch(loadedFile);
}

bool RunControlGUI::checkConfigFile() {
  QString loadedFile = txtConfigFileName->text();
  if(loadedFile.isNull())
    return false;
  QRegExp rx (".+(\\.conf$)");
  return rx.exactMatch(loadedFile);
}

void RunControlGUI::updateButtons(int state) {
  if(state == STATE_RUNNING)
    disableButtons = false;
  if(state == STATE_ERROR)
    disableButtons = true;
  configLoaded = checkConfigFile();
  configLoadedInit = checkInitFile();
  btnLoadInit->setEnabled(state != STATE_RUNNING && !disableButtons);
  btnInit->setEnabled(state != STATE_RUNNING && !disableButtons && configLoadedInit);
  btnLoad->setEnabled(state != STATE_RUNNING && !disableButtons);
  btnConfig->setEnabled(state != STATE_RUNNING && configLoaded &&
			!disableButtons);
  btnTerminate->setEnabled(state != STATE_RUNNING);
  btnStart->setEnabled(state == STATE_CONF && !disableButtons);
  btnStop->setEnabled(state == STATE_RUNNING);
}

std::string RunControlGUI::to_bytes(const std::string &val) {
  if (val == "")
    return "";
  uint64_t n = from_string(val, 0ULL);
  const char *suff[] = {" B", " kB", " MB", " GB", " TB"};
  const int numsuff = sizeof suff / sizeof *suff;
  int mult = 0;
  while (n / 1024 >= 10 && mult < numsuff - 1) {
    n /= 1024;
    mult++;
  }
  return to_string(n) + suff[mult];
}


RunControlGUI::~RunControlGUI() {
  QSettings settings("EUDAQ collaboration", "EUDAQ");
   
  settings.beginGroup("MainWindowEuRun");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.setValue("lastConfigFileDirectory", lastUsedDirectory);
  settings.setValue("lastConfigFile", txtConfigFileName->text());
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
