/********************************************************************************
** Form generated from reading UI file 'euLog.ui'
**
** Created: Mon 28. Oct 18:56:24 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EULOG_H
#define UI_EULOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QSpacerItem>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_wndLog
{
public:
    QWidget *centralwidget;
    QVBoxLayout *vboxLayout;
    QHBoxLayout *hboxLayout;
    QVBoxLayout *vboxLayout1;
    QLabel *lblLevel;
    QComboBox *cmbLevel;
    QVBoxLayout *vboxLayout2;
    QLabel *lblFrom;
    QComboBox *cmbFrom;
    QVBoxLayout *vboxLayout3;
    QLabel *lblSearch;
    QLineEdit *txtSearch;
    QSpacerItem *spacerItem;
    QTreeView *viewLog;
    QMenuBar *menubar;

    void setupUi(QMainWindow *wndLog)
    {
        if (wndLog->objectName().isEmpty())
            wndLog->setObjectName(QString::fromUtf8("wndLog"));
        wndLog->resize(716, 436);
        const QIcon icon = QIcon(QString::fromUtf8("../../images/Icon_euLog.png"));
        wndLog->setWindowIcon(icon);
        centralwidget = new QWidget(wndLog);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        vboxLayout = new QVBoxLayout(centralwidget);
#ifndef Q_OS_MAC
        vboxLayout->setSpacing(6);
#endif
#ifndef Q_OS_MAC
        vboxLayout->setContentsMargins(9, 9, 9, 9);
#endif
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setSpacing(2);
#ifndef Q_OS_MAC
        hboxLayout->setContentsMargins(0, 0, 0, 0);
#endif
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setSpacing(0);
#ifndef Q_OS_MAC
        vboxLayout1->setContentsMargins(0, 0, 0, 0);
#endif
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        lblLevel = new QLabel(centralwidget);
        lblLevel->setObjectName(QString::fromUtf8("lblLevel"));

        vboxLayout1->addWidget(lblLevel);

        cmbLevel = new QComboBox(centralwidget);
        cmbLevel->setObjectName(QString::fromUtf8("cmbLevel"));

        vboxLayout1->addWidget(cmbLevel);


        hboxLayout->addLayout(vboxLayout1);

        vboxLayout2 = new QVBoxLayout();
        vboxLayout2->setSpacing(0);
        vboxLayout2->setContentsMargins(0, 0, 0, 0);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        lblFrom = new QLabel(centralwidget);
        lblFrom->setObjectName(QString::fromUtf8("lblFrom"));

        vboxLayout2->addWidget(lblFrom);

        cmbFrom = new QComboBox(centralwidget);
        cmbFrom->setObjectName(QString::fromUtf8("cmbFrom"));

        vboxLayout2->addWidget(cmbFrom);


        hboxLayout->addLayout(vboxLayout2);

        vboxLayout3 = new QVBoxLayout();
        vboxLayout3->setSpacing(0);
        vboxLayout3->setContentsMargins(0, 0, 0, 0);
        vboxLayout3->setObjectName(QString::fromUtf8("vboxLayout3"));
        lblSearch = new QLabel(centralwidget);
        lblSearch->setObjectName(QString::fromUtf8("lblSearch"));

        vboxLayout3->addWidget(lblSearch);

        txtSearch = new QLineEdit(centralwidget);
        txtSearch->setObjectName(QString::fromUtf8("txtSearch"));

        vboxLayout3->addWidget(txtSearch);


        hboxLayout->addLayout(vboxLayout3);

        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);


        vboxLayout->addLayout(hboxLayout);

        viewLog = new QTreeView(centralwidget);
        viewLog->setObjectName(QString::fromUtf8("viewLog"));
        viewLog->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        viewLog->setSelectionMode(QAbstractItemView::ExtendedSelection);
        viewLog->setRootIsDecorated(false);
        viewLog->setSortingEnabled(true);
        viewLog->setAllColumnsShowFocus(true);

        vboxLayout->addWidget(viewLog);

        wndLog->setCentralWidget(centralwidget);
        menubar = new QMenuBar(wndLog);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 716, 22));
        wndLog->setMenuBar(menubar);

        retranslateUi(wndLog);

        QMetaObject::connectSlotsByName(wndLog);
    } // setupUi

    void retranslateUi(QMainWindow *wndLog)
    {
        wndLog->setWindowTitle(QApplication::translate("wndLog", "EUDAQ Log Collector", 0, QApplication::UnicodeUTF8));
        lblLevel->setText(QApplication::translate("wndLog", "Level:", 0, QApplication::UnicodeUTF8));
        lblFrom->setText(QApplication::translate("wndLog", "From:", 0, QApplication::UnicodeUTF8));
        cmbFrom->clear();
        cmbFrom->insertItems(0, QStringList()
         << QApplication::translate("wndLog", "All", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("wndLog", "RunControl", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("wndLog", "DataCollector", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("wndLog", "LogCollector", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("wndLog", "Monitor.*", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("wndLog", "Producer.*", 0, QApplication::UnicodeUTF8)
        );
        lblSearch->setText(QApplication::translate("wndLog", "Search:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class wndLog: public Ui_wndLog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EULOG_H
