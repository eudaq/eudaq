#include <QApplication>
#include <QDateTime>
#include <fstream>
#include "euRunApplication.h"
#include "euRun.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "Colours.hh"
#include "eudaq/ConnectionState.hh"
#include <exception>
#include "config.h" // for version symbols
//#include <QWindow>
//#include <QScreen>

static const char *statuses[] = {
    "RUN",       "Run Number", "EVENT",    "Events Built", "FULLRATE",
    "Rate",      "TRIG",       "Triggers", "FILEBYTES",    "File Bytes",
    "PARTICLES", "Particles",  "TLUSTAT",  "TLU Status",   "SCALERS",
    "Scalers",   0};

euRunApplication::euRunApplication(int &argc, char **argv)
    : QApplication(argc, argv) {}

bool euRunApplication::notify(QObject *receiver, QEvent *event) {
  bool done = true;
  try {
    done = QApplication::notify(receiver, event);
  } catch (const std::exception &ex) {
    // get current date
    QDate date = QDate::currentDate();
    std::cout << date.toString().toStdString()
              << ": euRun GUI caught (and ignored) exception: " << ex.what()
              << std::endl;

  } catch (...) {
    // get current date
    QDate date = QDate::currentDate();
    std::cout << date.toString().toStdString()
              << ": euRun GUI caught (and ignored) unspecified exception "
              << std::endl;
  }
  return done;
}

int main(int argc, char **argv) {
  euRunApplication app(argc, argv);
  eudaq::OptionParser op("EUDAQ Run Control", PACKAGE_VERSION,
                         "A Qt version of the Run Control");
  eudaq::Option<std::string> addr(
      op, "a", "listen-address", "tcp://44000", "address",
      "The address on which to listen for connections");
  eudaq::Option<std::string> level(
      op, "l", "log-level", "NONE", "level",
      "The minimum level for displaying log messages locally");
  eudaq::Option<int> x(op, "x", "left", 0, "pos");
  eudaq::Option<int> y(op, "y", "top", 0, "pos");
  eudaq::Option<int> w(op, "w", "width", 150, "pos");
  eudaq::Option<int> h(op, "g", "height", 200, "pos",
                       "The initial position of the window");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    QRect rect(x.Value(), y.Value(), w.Value(), h.Value());
    RunControlGUI gui(addr.Value(), rect);
    gui.show();
    return app.exec();
  } catch (...) {
    std::cout << "euRun exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    if (err.str() != "") {
      QMessageBox::warning(0, "Exception", err.str().c_str());
    }
    return result;
  }
  return 0;
}

namespace {
  static const char *GEOID_FILE = "GeoID.dat";
}

RunConnectionDelegate::RunConnectionDelegate(RunControlModel *model)
    : m_model(model) {}

void RunConnectionDelegate::GetModelData(){
   std::cout<<"DEBUG: Function works: \n";
}
void RunConnectionDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const {
  // std::cout << __FUNCTION__ << std::endl;
  // painter->save();
  int level = m_model->GetState(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  // painter->restore();
}

RunControlGUI::RunControlGUI(const std::string &listenaddress, QRect geom,
                             QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags), eudaq::RunControl(listenaddress),
      m_delegate(&m_run), m_prevtrigs(0), m_prevtime(0.0), m_runstarttime(0.0),
      m_filebytes(0), m_events(0), dostatus(false),
      m_producer_pALPIDEfs_not_ok(false), m_producer_pALPIDEss_not_ok(false),
      m_startrunwhenready(false),m_lastconfigonrunchange(false) {
  setupUi(this);
  if (!grpStatus->layout())
    grpStatus->setLayout(new QGridLayout(grpStatus));
  lblCurrent->setText(QString("<font size=%1  color='red'><b>Current State: Uninitialised </b></font>").arg(FONT_SIZE));
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

  //cmbConfig->setEditText("default");
  QSize fsize = frameGeometry().size();
  if (geom.x() == -1)
    geom.setX(x());
  if (geom.y() == -1)
    geom.setY(y());
  else
    geom.setY(geom.y() + MAGIC_NUMBER);
  if (geom.width() == -1)
    geom.setWidth(fsize.width());
  if (geom.height() == -1)
    geom.setHeight(fsize.height());
  // else geom.setHeight(geom.height() - MAGIC_NUMBER);
  move(geom.topLeft());
  resize(geom.size());
    QSettings settings("EUDAQ collaboration", "EUDAQ");

    settings.beginGroup("MainWindow");
    resize(settings.value("size", geom.size()).toSize());
    move(settings.value("pos", geom.topLeft()).toPoint());
    lastUsedDirectory = settings.value("lastConfigFileDirectory","../conf").toString();
    txtConfigFileName->setText(settings.value("lastConfigFile","config file not set").toString());
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
  setWindowTitle("eudaq Run Control " + PACKAGE_VERSION);
}

void RunControlGUI::OnReceive(const eudaq::ConnectionInfo &id,
                              std::shared_ptr<eudaq::ConnectionState> connectionstate) 
{

  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  if (id.GetType() == "DataCollector") {
    m_filebytes = from_string(connectionstate->GetTag("FILEBYTES"), 0LL);
    m_events = from_string(connectionstate->GetTag("EVENT"), 0LL);
    EmitStatus("EVENT", connectionstate->GetTag("EVENT"));
    EmitStatus("FILEBYTES", to_bytes(connectionstate->GetTag("FILEBYTES")));
  } else if (id.GetType() == "Producer") {
    if (id.GetName() == "caliceahcalbif") {
      EmitStatus("TRIG", status->GetTag("TRIG"));
      EmitStatus("PARTICLES", status->GetTag("PARTICLES"));
    } else  if (id.GetName() == "TLU" || id.GetName() == "miniTLU") {
      EmitStatus("TRIG", status->GetTag("TRIG"));
      EmitStatus("PARTICLES", status->GetTag("PARTICLES"));
      EmitStatus("TIMESTAMP", status->GetTag("TIMESTAMP"));
      EmitStatus("LASTTIME", status->GetTag("LASTTIME"));
      EmitStatus("TLUSTAT", status->GetTag("STATUS"));

      bool ok = true;
      std::string scalers;
      for (int i = 0; i < 4; ++i) {
        std::string s = connectionstate->GetTag("SCALER" + to_string(i));
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
      int trigs = from_string(connectionstate->GetTag("TRIG"), -1);
      double time = from_string(connectionstate->GetTag("TIMESTAMP"), 0.0);
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
    } else if (id.GetName() == "pALPIDEfs") {
      m_producer_pALPIDEfs_not_ok = (connectionstate->GetState() == 0);
    } else if (id.GetName() == "pALPIDEss") {
      m_producer_pALPIDEss_not_ok = (connectionstate->GetState() == 0);
    }
  }
  m_run.SetConnectionState(id, *connectionstate);
}

void RunControlGUI::OnConnect(const eudaq::ConnectionInfo &id) {
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    registered = true;
  }
  // QMessageBox::information(this, "EUDAQ Run Control",
  //                         "This will reset all connected Producers etc.");
  m_run.newconnection(id);
  if (id.GetType() == "DataCollector") {
    EmitStatus("RUN", "(" + to_string(m_runnumber) + ")");
  }
  else if (id.GetType() == "LogCollector") {
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

RunControlGUI::~RunControlGUI(){
    QSettings settings("EUDAQ collaboration", "EUDAQ");
  
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    //settings.setValue("screen", windowHandle()->screen()->screenNumber());
    settings.setValue("lastConfigFileDirectory",lastUsedDirectory);
    settings.setValue("lastConfigFile",txtConfigFileName->text());
    settings.endGroup();
}
