#include <QApplication>
#include "euRun.hh"
#include "eudaq/OptionParser.hh"
#include "Colours.hh"

static const char * statuses[] = {
  "RUN",       "Run Number",
  "TIMESTAMP", "Timestamp",
  "EVENT",     "Events Built",
  "TRIG",      "Triggers",
  0
};

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

RunControlGUI::RunControlGUI(const std::string & listenaddress,
                             QRect geom,
                             QWidget *parent,
                             Qt::WindowFlags flags)
  : QMainWindow(parent, flags),
    eudaq::RunControl(listenaddress),
    m_delegate(&m_run)
{
  setupUi(this);
  if (!grpStatus->layout()) grpStatus->setLayout(new QGridLayout(grpStatus));
  QGridLayout * layout = dynamic_cast<QGridLayout *>(grpStatus->layout());
  int row = 0, col = 0;
  for (const char ** st = statuses; st[0] && st[1]; st += 2) {
    QLabel * lblname = new QLabel(grpStatus);
    lblname->setObjectName(QString("lbl_st_") + st[0]);
    lblname->setText((std::string(st[1]) + ": ").c_str());
    QLabel * lblvalue = new QLabel(grpStatus);
    lblvalue->setObjectName(QString("txt_st_") + st[0]);
    //lblvalue->setText("");
    layout->addWidget(lblname, row, col*2);
    layout->addWidget(lblvalue, row, col*2+1);
    m_status[st[0]] = std::make_pair(lblname, lblvalue);
    if (++col > 1) {
      ++row;
      col = 0;
    }
  }
  viewConn->setModel(&m_run);
  viewConn->setItemDelegate(&m_delegate);
  QDir dir("./conf/", "*.conf");
  for (size_t i = 0; i < dir.count(); ++i) {
    QString item = dir[i];
    item.chop(5);
    cmbConfig->addItem(item);
  }
  cmbConfig->setEditText("default");
  QSize fsize = frameGeometry().size();
  if (geom.x() == -1) geom.setX(x());
  if (geom.y() == -1) geom.setY(y());
  else geom.setY(geom.y() + MAGIC_NUMBER);
  if (geom.width() == -1) geom.setWidth(fsize.width());
  if (geom.height() == -1) geom.setHeight(fsize.height());
  //else geom.setHeight(geom.height() - MAGIC_NUMBER);
  move(geom.topLeft());
  resize(geom.size());
  connect(this, SIGNAL(StatusChanged(const QString &, const QString &)), this, SLOT(ChangeStatus(const QString &, const QString &)));
  
  //connect(this, SIGNAL(TrigNumberChanged(const QString &)), txtTriggers, SLOT(setText(const QString &)));
  //connect(this, SIGNAL(EventNumberChanged(const QString &)), txtEvents,  SLOT(setText(const QString &)));
  //connect(this, SIGNAL(TimestampChanged(const QString &)), txtTimestamp, SLOT(setText(const QString &)));
  //connect(&m_statustimer, SIGNAL(timeout()), this, SLOT(timer()));
  m_statustimer.start(500);
}
