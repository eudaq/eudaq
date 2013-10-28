/********************************************************************************
** Form generated from reading UI file 'LogDialog.ui'
**
** Created: Mon 28. Oct 18:56:24 2013
**      by: Qt User Interface Compiler version 4.8.4
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGDIALOG_H
#define UI_LOGDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_dlgLogMessage
{
public:
    QVBoxLayout *verticalLayout;
    QTreeWidget *treeLogMessage;
    QDialogButtonBox *btnClose;

    void setupUi(QDialog *dlgLogMessage)
    {
        if (dlgLogMessage->objectName().isEmpty())
            dlgLogMessage->setObjectName(QString::fromUtf8("dlgLogMessage"));
        dlgLogMessage->resize(636, 304);
        verticalLayout = new QVBoxLayout(dlgLogMessage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        treeLogMessage = new QTreeWidget(dlgLogMessage);
        treeLogMessage->setObjectName(QString::fromUtf8("treeLogMessage"));

        verticalLayout->addWidget(treeLogMessage);

        btnClose = new QDialogButtonBox(dlgLogMessage);
        btnClose->setObjectName(QString::fromUtf8("btnClose"));
        btnClose->setOrientation(Qt::Horizontal);
        btnClose->setStandardButtons(QDialogButtonBox::Close);
        btnClose->setCenterButtons(true);

        verticalLayout->addWidget(btnClose);


        retranslateUi(dlgLogMessage);
        QObject::connect(btnClose, SIGNAL(accepted()), dlgLogMessage, SLOT(accept()));
        QObject::connect(btnClose, SIGNAL(rejected()), dlgLogMessage, SLOT(reject()));

        QMetaObject::connectSlotsByName(dlgLogMessage);
    } // setupUi

    void retranslateUi(QDialog *dlgLogMessage)
    {
        dlgLogMessage->setWindowTitle(QApplication::translate("dlgLogMessage", "Log Message", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeLogMessage->headerItem();
        ___qtreewidgetitem->setText(2, QApplication::translate("dlgLogMessage", "Short", 0, QApplication::UnicodeUTF8));
        ___qtreewidgetitem->setText(1, QApplication::translate("dlgLogMessage", "Full", 0, QApplication::UnicodeUTF8));
        ___qtreewidgetitem->setText(0, QApplication::translate("dlgLogMessage", "Name", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class dlgLogMessage: public Ui_dlgLogMessage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGDIALOG_H
