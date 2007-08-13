#include "ui_euProd.h"
#include <QMainWindow>
#include <QMessageBox>

class ProducerGUI : public QMainWindow, public Ui::wndProd {
  Q_OBJECT
  Q_CLASSINFO("Author", "Emlyn Corrin")
public:
  ProducerGUI(QWidget *parent = 0, Qt::WindowFlags flags = 0)
    : QMainWindow(parent, flags)
  {
    setupUi(this);
  }
private slots:
  void on_btnTrigger_clicked() {
    QMessageBox::information(this, "EUDAQ Dummy Producer",
                             "This will generate a software Trigger.");
  }
};
