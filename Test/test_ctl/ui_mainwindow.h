/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *btn_bang;
    QSpinBox *spn_val;
    QLineEdit *ln_display;
    QPushButton *btn_stop;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QPushButton *btn_start;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(355, 182);
        MainWindow->setMaximumSize(QSize(500, 500));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        btn_bang = new QPushButton(centralwidget);
        btn_bang->setObjectName(QString::fromUtf8("btn_bang"));
        btn_bang->setGeometry(QRect(20, 40, 92, 27));
        spn_val = new QSpinBox(centralwidget);
        spn_val->setObjectName(QString::fromUtf8("spn_val"));
        spn_val->setGeometry(QRect(120, 40, 55, 25));
        ln_display = new QLineEdit(centralwidget);
        ln_display->setObjectName(QString::fromUtf8("ln_display"));
        ln_display->setEnabled(true);
        ln_display->setGeometry(QRect(180, 40, 113, 25));
        ln_display->setDragEnabled(false);
        ln_display->setReadOnly(true);
        btn_stop = new QPushButton(centralwidget);
        btn_stop->setObjectName(QString::fromUtf8("btn_stop"));
        btn_stop->setGeometry(QRect(190, 70, 92, 27));
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(40, 20, 59, 15));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(120, 20, 59, 15));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(200, 20, 71, 16));
        btn_start = new QPushButton(centralwidget);
        btn_start->setObjectName(QString::fromUtf8("btn_start"));
        btn_start->setGeometry(QRect(60, 70, 92, 27));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 355, 23));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        btn_bang->setText(QApplication::translate("MainWindow", "Bang1", 0, QApplication::UnicodeUTF8));
        btn_stop->setText(QApplication::translate("MainWindow", "Stop", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "Bang Me", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "Spin Me", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("MainWindow", "Who Is Me?", 0, QApplication::UnicodeUTF8));
        btn_start->setText(QApplication::translate("MainWindow", "Start", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
