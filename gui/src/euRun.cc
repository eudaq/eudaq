#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRun.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "Colours.hh"
#include "eudaq/Status.hh"
#include <exception>
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
  // std::cout << __FUNCTION__ << std::endl;
  // painter->save();
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  // painter->restore();
}

//  RunControlGUI(const std::string &listenaddress, QRect geom,
//                QWidget *parent = 0, Qt::WindowFlags flags = 0);
//  ~RunControlGUI() override;
RunControlGUI::RunControlGUI(const std::string &listenaddress,
                             QWidget *parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags), eudaq::RunControl(listenaddress),
    m_delegate(&m_run), m_prevtrigs(0), m_prevtime(0.0), m_runstarttime(0.0),
    m_filebytes(0), m_events(0), dostatus(false),
    m_producer_pALPIDEfs_not_ok(false), m_producer_pALPIDEss_not_ok(false),
    m_startrunwhenready(false),m_lastconfigonrunchange(false), m_data_taking(false), configLoaded(false), configLoadedInit(false), disableButtons(false), curState(STATE_UNINIT),lastUsedDirectory(""),lastUsedDirectoryInit("") {  // m_nextconfigonrunchange(false) taken out for test!
  setupUi(this);
  QRect geom(-1,-1, 150, 200);
  if (!grpStatus->layout())
    grpStatus->setLayout(new QGridLayout(grpStatus));
  QGridLayout *layout = dynamic_cast<QGridLayout *>(grpStatus->layout());
  int row = 0, col = 0;
  for (const char **st = statuses; st[0] && st[1]; st += 2) {
    QLabel *lblname = new QLabel(grpStatus);
    lblname->setObjectName(QString("lbl_st_") + st[0]);
    lblname->setText((std::string(st[1]) + ": ").c_str());
    QLabel *lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName(QString("txt_st_") + st[0]);
    // lblvalue->setText("");
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

  // cmbConfig->setEditText("default");
  QRect geom_from_last_program_run;

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


  QSettings settings("EUDAQ collaboration", "EUDAQ");

  settings.beginGroup("MainWindowEuRun");
  geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
  geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
  lastUsedDirectory =
      settings.value("lastConfigFileDirectory", "").toString();
  txtConfigFileName->setText(
      settings.value("lastConfigFile", "config file not set").toString());
  settings.endGroup();


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

  // following states have been taken out now, Check if to change ?
  /*  if(txtConfigFileName->text()!="config file not set") {
   QFile file(txtConfigFileName->text());
   if(file.exists()){
     SetState(ST_CONFIGLOADED);
   } else {
     SetState(ST_NONE);
     txtConfigFileName->setText("config file not set");
   }
   file.close();
   }*/
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
    if (id->GetName() == "caliceahcalbif") {  // AHCALBIG just added from 1.7
      EmitStatus("TRIG", status->GetTag("TRIG"));
      EmitStatus("PARTICLES", status->GetTag("PARTICLES"));
    }{
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
      } else if (id->GetName() == "pALPIDEfs") {
	m_producer_pALPIDEfs_not_ok = (status->GetLevel() != 1);
      } else if (id->GetName() == "pALPIDEss") {
	m_producer_pALPIDEss_not_ok = (status->GetLevel() != 1);
      }
    }
    
    m_run.SetStatus(*id, *status);
  }
} 
void RunControlGUI::DoConnect(eudaq::ConnectionSPC id) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  // QMessageBox::information(this, "EUDAQ Run Control",
  //                         "This will reset all connected Producers etc.");
  m_run.newconnection(id);
  if (id->GetType() == "DataCollector") {
    EmitStatus("RUN", "(" + to_string(GetRunNumber()) + ")");
    //    SetState(ST_NONE);
  }
  if (id->GetType() == "LogCollector") {
    btnLogSetStatus(true);
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
 
 RunControlGUI::~RunControlGUI() {
   QSettings settings("EUDAQ collaboration", "EUDAQ");
   
   settings.beginGroup("MainWindowEuRun");
   settings.setValue("size", size());
   settings.setValue("pos", pos());
   // settings.setValue("screen", windowHandle()->screen()->screenNumber());
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
 
