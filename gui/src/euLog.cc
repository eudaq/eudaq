#include <QApplication>
#include <QMessageBox>
#include "eudaq/FileNamer.hh"
#include "euLog.hh"
#include "Colours.hh"


namespace{
  auto dummy0 = eudaq::Factory<eudaq::LogCollector>::
    Register<LogCollectorGUI, const std::string&, const std::string&>(eudaq::cstr2hash("LogCollectorGUI"));
  auto dummy1 = eudaq::Factory<eudaq::LogCollector>::
    Register<LogCollectorGUI, const std::string&, const std::string&>(eudaq::cstr2hash("GuiLogCollector"));
}

LogItemDelegate::LogItemDelegate(LogCollectorModel *model) : m_model(model) {}

void LogItemDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
}

LogCollectorGUI::LogCollectorGUI(const std::string &name,
				 const std::string &runcontrol)
  : QMainWindow(0, 0),
    eudaq::LogCollector(name, runcontrol), m_delegate(&m_model) {
  setupUi(this);
  std::string filename;
  viewLog->setModel(&m_model);
  viewLog->setItemDelegate(&m_delegate);
  for (int i = 0; i < LogMessage::NumColumns(); ++i) {
    int w = LogMessage::ColumnWidth(i);
    if (w >= 0)
      viewLog->setColumnWidth(i, w);
  }
  int level = 0;
  for (;;) {
    std::string text = eudaq::Status::Level2String(level);
    if (text == "")
      break;
    text = eudaq::to_string(level) + "-" + text;
    cmbLevel->addItem(text.c_str());
    level++;
  }
  cmbLevel->setCurrentIndex(0);//loglevel
  QRect geom(-1,-1, 100, 100);
  QRect geom_from_last_program_run;
  QSettings settings("EUDAQ collaboration", "EUDAQ");

  settings.beginGroup("MainWindowEuLog");
  geom_from_last_program_run.setSize(settings.value("size", geom.size()).toSize());
  geom_from_last_program_run.moveTo(settings.value("pos", geom.topLeft()).toPoint());
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
  connect(this, SIGNAL(RecMessage(const eudaq::LogMessage &)), this,
	  SLOT(AddMessage(const eudaq::LogMessage &)));
  try {
    if (filename != "")
      LoadFile(filename);
  } catch (const std::runtime_error &) {
    // probably file not found: ignore
  }
}

LogCollectorGUI::~LogCollectorGUI(){
  QSettings settings("EUDAQ collaboration", "EUDAQ");
  settings.beginGroup("MainWindowEuLog");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}


void LogCollectorGUI::LoadFile(const std::string &filename) {
  std::vector<std::string> sources = m_model.LoadFile(filename);
  std::cout << "File loaded, sources = " << sources.size() << std::endl;
  for (size_t i = 0; i < sources.size(); ++i) {
    size_t dot = sources[i].find('.');
    if (dot != std::string::npos) {
      AddSender(sources[i].substr(0, dot), sources[i].substr(dot + 1));
    } else {
      AddSender(sources[i]);
    }
  }
}

void LogCollectorGUI::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::string file_pattern = "euLog_$12D.log";
  if(ini){
    file_pattern = ini->Get("EULOG_GUI_LOG_FILE_PATTERN", file_pattern);
  }
  std::time_t time_now = std::time(nullptr);
  char time_buff[13];
  time_buff[12] = 0;
  std::strftime(time_buff, sizeof(time_buff),
		"%y%m%d%H%M%S", std::localtime(&time_now));
  std::string start_time(time_buff);
  m_os_file.open(std::string(eudaq::FileNamer(file_pattern).Set('D', start_time)).c_str(),
		 std::ios_base::app);
  std::stringstream ss;
  ss << "\n*** LogCollector started at "<< time_now<< " ***";
  m_os_file<<ss.str()<<std::endl;
}


void LogCollectorGUI::DoConnect(eudaq::ConnectionSPC id){
  eudaq::mSleep(100);
  CheckRegistered();
  EUDAQ_INFO("Connection from " + to_string(id));
  AddSender(id->GetType(), id->GetName());
}

void LogCollectorGUI::DoReceive(const eudaq::LogMessage &msg){

  std::cout<< msg<<std::endl;
  CheckRegistered();
  if(m_os_file.is_open())
    m_os_file << msg << std::endl;
  emit RecMessage(msg);
}

void LogCollectorGUI::closeEvent(QCloseEvent *) {
  std::cout << "Closing!" << std::endl;
  QApplication::quit();
}

void LogCollectorGUI::AddSender(const std::string &type,
                                const std::string &name) {
  bool foundtype = false;
  int count = cmbFrom->count();
  for (int i = 0; i <= count; ++i) {
    std::string curname,
      curtype = (i == count ? "" : cmbFrom->itemText(i).toStdString());
    size_t dot = curtype.find('.');
    if (dot != std::string::npos) {
      curname = curtype.substr(dot + 1);
      curtype = curtype.substr(0, dot);
    }
    if (curtype == type) {
      if (curname == name || (curname == "*" && name == ""))
        return;
      if (!foundtype) {
        if (curname == "" && name != "") {
          cmbFrom->setItemText(i, (curtype + ".*").c_str());
        }
        foundtype = true;
      }
    } else {
      bool insertedtype = false;
      if (i == count && !foundtype) {
        std::string text = type;
        if (name != "")
          text += ".*";
        cmbFrom->insertItem(i, text.c_str());
        insertedtype = true;
      }
      if (foundtype || (i == count && name != "")) {
        cmbFrom->insertItem(i + insertedtype, (type + "." + name).c_str());
        return;
      }
    }
  }
}

void LogCollectorGUI::on_cmbLevel_currentIndexChanged(int index) {
  m_model.SetDisplayLevel(index);
}

void LogCollectorGUI::on_cmbFrom_currentIndexChanged(const QString &text) {
  std::string type = text.toStdString(), name;
  size_t dot = type.find('.');
  if (dot != std::string::npos) {
    name = type.substr(dot + 1);
    type = type.substr(0, dot);
  }
  m_model.SetDisplayNames(type, name);
}

void LogCollectorGUI::on_txtSearch_editingFinished() {
  m_model.SetSearch(txtSearch->displayText().toStdString());
}

void LogCollectorGUI::on_viewLog_activated(const QModelIndex &i) {
  new LogDialog(m_model.GetMessage(i.row()));
}

void LogCollectorGUI::AddMessage(const eudaq::LogMessage &msg) {
  QModelIndex pos = m_model.AddMessage(msg);
  if (pos.isValid())
    viewLog->scrollTo(pos);
}

void LogCollectorGUI::CheckRegistered(){
  static bool registered = false;
  if (!registered) {
    qRegisterMetaType<QModelIndex>("QModelIndex");
    qRegisterMetaType<eudaq::LogMessage>("eudaq::LogMessage");
    registered = true;
  }
}

void LogCollectorGUI::Exec(){
  StartLogCollector(); //TODO: Start it OnServer
  StartCommandReceiver();

  show();
  if(QApplication::instance())
    QApplication::instance()->exec(); 
  else
    std::cerr<<"ERROR: LogCollectorGUI::EXEC\n";

  while(IsActiveCommandReceiver() || IsActiveLogCollector()){
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
