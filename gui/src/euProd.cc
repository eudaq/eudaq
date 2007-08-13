#include <QApplication>
#include "euProd.hh"

int main(int argc, char ** argv) {
  QApplication app(argc, argv);
  QMainWindow *window = new ProducerGUI();
  window->show();
  return app.exec();
}
