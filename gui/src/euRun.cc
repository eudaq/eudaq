#include <QApplication>
#include "euRun.hh"
#include "eudaq/OptionParser.hh"
#include "Colours.hh"

int main(int argc, char ** argv) {
  QApplication app(argc, argv);
  eudaq::OptionParser op("EUDAQ Run Control", "1.0", "A Qt version of the Run Control");
  eudaq::Option<std::string>  addr(op, "a", "listen-address", "tcp://7000", "address",
                                   "The address on which to listen for connections");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<int>             x(op, "x", "left",   -1, "pos");
  eudaq::Option<int>             y(op, "y", "top",    -1, "pos");
  eudaq::Option<int>             w(op, "w", "width",  -1, "pos");
  eudaq::Option<int>             h(op, "g", "height", -1, "pos", "The initial position of the window");
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

RunConnectionDelegate::RunConnectionDelegate(RunControlModel * model) : m_model(model) {}

void RunConnectionDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const {
  //std::cout << __FUNCTION__ << std::endl;
  //painter->save();
  int level = m_model->GetLevel(index);
  painter->fillRect(option.rect, QBrush(level_colours[level]));
  QItemDelegate::paint(painter, option, index);
  //painter->restore();
}
