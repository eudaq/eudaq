#include <QApplication>
#include "euProd.hh"

ProducerGUI::ProducerGUI(const std::string & name, const std::string &runcontrol)
  :QMainWindow(0, 0), eudaq::Producer(name, runcontrol){
  setupUi(this);
}

// void ProducerGUI::Exec(){
//   show();
//   if(QApplication::instance()){
//     std::thread qthread(&Producer::Exec, this); //TODO: is QCore?
//     qthread.detach();
//     QApplication::instance()->exec();
//     // Producer::Exec();
//   }
//   else
//     std::cerr<<"ERROR: ProducerGUI::EXEC\n";
// }

void ProducerGUI::on_btnTrigger_clicked() {
    QMessageBox::information(this, "EUDAQ Dummy Producer",
                             "This will generate a software Trigger.");
}
