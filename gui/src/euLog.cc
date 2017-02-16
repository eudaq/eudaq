#include <QApplication>
#include <QMessageBox>
#include "euLog.hh"
#include "Colours.hh"


namespace{
  auto dummy0 = eudaq::Factory<eudaq::LogCollector>::
    Register<LogCollectorGUI, const std::string&, const std::string&,
	     const std::string&, const int&>(eudaq::cstr2hash("LogCollectorGUI"));
  auto dummy1 = eudaq::Factory<eudaq::LogCollector>::
    Register<LogCollectorGUI, const std::string&, const std::string&,
	     const std::string&, const int&>(eudaq::cstr2hash("GuiLogCollector"));

}


LogItemDelegate::LogItemDelegate(LogCollectorModel *model) : m_model(model) {}

void LogItemDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const {
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
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
