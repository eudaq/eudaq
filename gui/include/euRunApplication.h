#include <QApplication>
class euRunApplication : public QApplication {
public:
  euRunApplication(int &argc, char **argv);
  bool notify(QObject *receiver, QEvent *event);
};
