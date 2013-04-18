#include <QApplication>
#include <QMessageBox>
#include "euLog.hh"
#include "eudaq/OptionParser.hh"
#include "Colours.hh"

int main(int argc, char ** argv) {
  QApplication app(argc, argv);
  eudaq::OptionParser op("EUDAQ Log Collector", "1.0", "A Qt version of the Log Collector");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
      "The address of the RunControl application");
  eudaq::Option<std::string> addr (op, "a", "listen-address", "tcp://44002", "address",
      "The address on which to listen for Log connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
      "The initial level for displaying log messages");
  eudaq::Option<std::string> file (op, "f", "log-file", "", "filename",
      "A log file to load at startup");
  eudaq::Option<int>             x(op, "x", "left",   0, "pos");
  eudaq::Option<int>             y(op, "y", "top",    0, "pos");
  eudaq::Option<int>             w(op, "w", "width",  100, "pos");
  eudaq::Option<int>             h(op, "g", "height", 100, "pos", "The initial position of the window");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    QRect rect(x.Value(), y.Value(), w.Value(), h.Value());
    LogCollectorGUI gui(rctrl.Value(), addr.Value(), rect, file.Value(), eudaq::Status::String2Level(level.Value()));
    gui.show();
    return app.exec();
  } catch (...) {
    std::cout << "euLog exception handler" << std::endl;
    std::ostringstream err;
    int result = op.HandleMainException(err);
    if (err.str() != "") {
      QMessageBox::warning(0, "Exception", err.str().c_str());
    }
    return result;
  }
  return 0;
}

LogItemDelegate::LogItemDelegate(LogCollectorModel * model) : m_model(model) {}

void LogItemDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
  //std::cout << __FUNCTION__ << std::endl;
  //painter->save();
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  //painter->restore();
}

void LogCollectorGUI::AddSender(const std::string & type, const std::string & name) {
  bool foundtype = false;
  int count = cmbFrom->count();
  for (int i = 0; i <= count; ++i) {
    std::string curname, curtype = (i == count ? "" : cmbFrom->itemText(i).toStdString());
    size_t dot = curtype.find('.');
    if (dot != std::string::npos) {
      curname = curtype.substr(dot+1);
      curtype = curtype.substr(0, dot);
    }
    if (curtype == type) {
      if (curname == name || (curname == "*" && name == "")) return;
      if (!foundtype) {
        //std::cout << "Found type" << std::endl;
        if (curname == "" && name != "") {
          //std::cout << "Setting .*" << std::endl;
          cmbFrom->setItemText(i, (curtype + ".*").c_str());
        }
        foundtype = true;
      }
    } else {
      bool insertedtype = false;
      if (i == count && !foundtype) {
        std::string text = type;
        if (name != "") text += ".*";
        //std::cout << "Inserting type" << std::endl;
        cmbFrom->insertItem(i, text.c_str());
        insertedtype = true;
      }
      if (foundtype || (i == count && name != "")) {
        //std::cout << "Inserting name" << std::endl;
        cmbFrom->insertItem(i+insertedtype, (type + "." + name).c_str());
        return;
      }
    }
  }
}
