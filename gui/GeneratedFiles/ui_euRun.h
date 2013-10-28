/********************************************************************************
** Form generated from reading UI file 'euRun.ui'
**
** Created: Wed 23. Oct 16:25:23 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EURUN_H
#define UI_EURUN_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_wndRun
{
public:
    QWidget *centralwidget;
    QVBoxLayout *vboxLayout;
    QGroupBox *grpControl;
    QGridLayout *gridLayout;
    QLabel *lblConfig;
    QLabel *lblRunmsg;
    QPushButton *btnLog;
    QLineEdit *txtLogmsg;
    QLineEdit *txtRunmsg;
    QComboBox *cmbConfig;
    QPushButton *btnStart;
    QLabel *lblLogmsg;
    QPushButton *btnConfig;
    QPushButton *btnStop;
    QLabel *lblGeoID;
    QLineEdit *txtGeoID;
    QGroupBox *grpStatus;
    QGridLayout *gridLayout1;
    QGroupBox *grpConnections;
    QVBoxLayout *vboxLayout1;
    QTreeView *viewConn;
    QMenuBar *menubar;

    void setupUi(QMainWindow *wndRun)
    {
        if (wndRun->objectName().isEmpty())
            wndRun->setObjectName(QString::fromUtf8("wndRun"));
        wndRun->setEnabled(true);
        wndRun->resize(349, 372);
        QIcon icon;
        icon.addFile(QString::fromUtf8("../../images/Icon_euRun.png"), QSize(), QIcon::Normal, QIcon::Off);
        wndRun->setWindowIcon(icon);
        centralwidget = new QWidget(wndRun);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        vboxLayout = new QVBoxLayout(centralwidget);
        vboxLayout->setSpacing(4);
        vboxLayout->setContentsMargins(4, 4, 4, 4);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        grpControl = new QGroupBox(centralwidget);
        grpControl->setObjectName(QString::fromUtf8("grpControl"));
        gridLayout = new QGridLayout(grpControl);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(8);
        gridLayout->setVerticalSpacing(4);
        lblConfig = new QLabel(grpControl);
        lblConfig->setObjectName(QString::fromUtf8("lblConfig"));

        gridLayout->addWidget(lblConfig, 1, 0, 1, 1);

        lblRunmsg = new QLabel(grpControl);
        lblRunmsg->setObjectName(QString::fromUtf8("lblRunmsg"));

        gridLayout->addWidget(lblRunmsg, 2, 0, 1, 1);

        btnLog = new QPushButton(grpControl);
        btnLog->setObjectName(QString::fromUtf8("btnLog"));
        btnLog->setEnabled(false);

        gridLayout->addWidget(btnLog, 3, 2, 1, 1);

        txtLogmsg = new QLineEdit(grpControl);
        txtLogmsg->setObjectName(QString::fromUtf8("txtLogmsg"));

        gridLayout->addWidget(txtLogmsg, 3, 1, 1, 1);

        txtRunmsg = new QLineEdit(grpControl);
        txtRunmsg->setObjectName(QString::fromUtf8("txtRunmsg"));

        gridLayout->addWidget(txtRunmsg, 2, 1, 1, 1);

        cmbConfig = new QComboBox(grpControl);
        cmbConfig->setObjectName(QString::fromUtf8("cmbConfig"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(cmbConfig->sizePolicy().hasHeightForWidth());
        cmbConfig->setSizePolicy(sizePolicy);
        cmbConfig->setEditable(true);
        cmbConfig->setMaxVisibleItems(16);
        cmbConfig->setInsertPolicy(QComboBox::InsertAlphabetically);

        gridLayout->addWidget(cmbConfig, 1, 1, 1, 1);

        btnStart = new QPushButton(grpControl);
        btnStart->setObjectName(QString::fromUtf8("btnStart"));
        btnStart->setEnabled(false);

        gridLayout->addWidget(btnStart, 2, 2, 1, 1);

        lblLogmsg = new QLabel(grpControl);
        lblLogmsg->setObjectName(QString::fromUtf8("lblLogmsg"));

        gridLayout->addWidget(lblLogmsg, 3, 0, 1, 1);

        btnConfig = new QPushButton(grpControl);
        btnConfig->setObjectName(QString::fromUtf8("btnConfig"));
        btnConfig->setEnabled(false);
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(btnConfig->sizePolicy().hasHeightForWidth());
        btnConfig->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(btnConfig, 1, 2, 1, 1);

        btnStop = new QPushButton(grpControl);
        btnStop->setObjectName(QString::fromUtf8("btnStop"));
        btnStop->setEnabled(false);

        gridLayout->addWidget(btnStop, 4, 2, 1, 1);

        lblGeoID = new QLabel(grpControl);
        lblGeoID->setObjectName(QString::fromUtf8("lblGeoID"));

        gridLayout->addWidget(lblGeoID, 4, 0, 1, 1);

        txtGeoID = new QLineEdit(grpControl);
        txtGeoID->setObjectName(QString::fromUtf8("txtGeoID"));
        txtGeoID->setReadOnly(true);

        gridLayout->addWidget(txtGeoID, 4, 1, 1, 1);


        vboxLayout->addWidget(grpControl);

        grpStatus = new QGroupBox(centralwidget);
        grpStatus->setObjectName(QString::fromUtf8("grpStatus"));
        gridLayout1 = new QGridLayout(grpStatus);
        gridLayout1->setContentsMargins(0, 0, 0, 0);
        gridLayout1->setObjectName(QString::fromUtf8("gridLayout1"));
        gridLayout1->setHorizontalSpacing(8);
        gridLayout1->setVerticalSpacing(4);

        vboxLayout->addWidget(grpStatus);

        grpConnections = new QGroupBox(centralwidget);
        grpConnections->setObjectName(QString::fromUtf8("grpConnections"));
        vboxLayout1 = new QVBoxLayout(grpConnections);
        vboxLayout1->setSpacing(4);
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        viewConn = new QTreeView(grpConnections);
        viewConn->setObjectName(QString::fromUtf8("viewConn"));
        viewConn->setRootIsDecorated(false);
        viewConn->setSortingEnabled(true);

        vboxLayout1->addWidget(viewConn);


        vboxLayout->addWidget(grpConnections);

        wndRun->setCentralWidget(centralwidget);
        menubar = new QMenuBar(wndRun);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 349, 22));
        wndRun->setMenuBar(menubar);
        QWidget::setTabOrder(btnConfig, txtRunmsg);
        QWidget::setTabOrder(txtRunmsg, btnStart);
        QWidget::setTabOrder(btnStart, txtLogmsg);
        QWidget::setTabOrder(txtLogmsg, btnLog);

        retranslateUi(wndRun);
        QObject::connect(txtRunmsg, SIGNAL(returnPressed()), btnStart, SLOT(animateClick()));
        QObject::connect(txtLogmsg, SIGNAL(returnPressed()), btnLog, SLOT(animateClick()));

        QMetaObject::connectSlotsByName(wndRun);
    } // setupUi

    void retranslateUi(QMainWindow *wndRun)
    {
        wndRun->setWindowTitle(QApplication::translate("wndRun", "eudaq Run Control", 0, QApplication::UnicodeUTF8));
        grpControl->setTitle(QApplication::translate("wndRun", "Control", 0, QApplication::UnicodeUTF8));
        lblConfig->setText(QApplication::translate("wndRun", "Config: ", 0, QApplication::UnicodeUTF8));
        lblRunmsg->setText(QApplication::translate("wndRun", "Run: ", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnLog->setToolTip(QApplication::translate("wndRun", "Send a log message", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnLog->setText(QApplication::translate("wndRun", "Log", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        txtLogmsg->setToolTip(QApplication::translate("wndRun", "Send a message to the LogCollector", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        txtRunmsg->setToolTip(QApplication::translate("wndRun", "An optional message to add to the Run", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        cmbConfig->setToolTip(QApplication::translate("wndRun", "Select configuration to use", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_TOOLTIP
        btnStart->setToolTip(QApplication::translate("wndRun", "Start a run", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnStart->setText(QApplication::translate("wndRun", "Start", 0, QApplication::UnicodeUTF8));
        lblLogmsg->setText(QApplication::translate("wndRun", "Log: ", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnConfig->setToolTip(QApplication::translate("wndRun", "Configure using the selected configuration", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnConfig->setText(QApplication::translate("wndRun", "Config", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        btnStop->setToolTip(QApplication::translate("wndRun", "Stop a run", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        btnStop->setText(QApplication::translate("wndRun", "Stop", 0, QApplication::UnicodeUTF8));
        lblGeoID->setText(QApplication::translate("wndRun", "GeoID:", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        txtGeoID->setToolTip(QApplication::translate("wndRun", "Double-click to change GeoID", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        grpStatus->setTitle(QApplication::translate("wndRun", "Status", 0, QApplication::UnicodeUTF8));
        grpConnections->setTitle(QApplication::translate("wndRun", "Connections", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        viewConn->setToolTip(QApplication::translate("wndRun", "Processes connected to the Run Control", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
    } // retranslateUi

};

namespace Ui {
    class wndRun: public Ui_wndRun {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EURUN_H
