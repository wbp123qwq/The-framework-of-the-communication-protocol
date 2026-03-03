/********************************************************************************
** Form generated from reading UI file 'udp.ui'
**
** Created by: Qt User Interface Compiler version 6.5.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_UDP_H
#define UI_UDP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_UDP
{
public:

    void setupUi(QWidget *UDP)
    {
        if (UDP->objectName().isEmpty())
            UDP->setObjectName("UDP");
        UDP->resize(552, 518);

        retranslateUi(UDP);

        QMetaObject::connectSlotsByName(UDP);
    } // setupUi

    void retranslateUi(QWidget *UDP)
    {
        UDP->setWindowTitle(QCoreApplication::translate("UDP", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class UDP: public Ui_UDP {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_UDP_H
