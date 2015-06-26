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
#include <QtGui/QSlider>
#include <QtGui/QStatusBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QPushButton *btn_audio_on;
    QPushButton *btn_audio_off;
    QSlider *sldr_r_freq;
    QSlider *sldr_r_amp;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QSlider *sldr_l_freq;
    QSlider *sldr_l_amp;
    QLabel *label_5;
    QLabel *label_6;
    QLineEdit *le_l_freq;
    QLineEdit *le_l_amp;
    QLineEdit *le_r_freq;
    QLineEdit *le_r_amp;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(338, 360);
        MainWindow->setMaximumSize(QSize(500, 500));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        btn_audio_on = new QPushButton(centralwidget);
        btn_audio_on->setObjectName(QString::fromUtf8("btn_audio_on"));
        btn_audio_on->setGeometry(QRect(10, 20, 92, 27));
        btn_audio_off = new QPushButton(centralwidget);
        btn_audio_off->setObjectName(QString::fromUtf8("btn_audio_off"));
        btn_audio_off->setGeometry(QRect(10, 50, 92, 27));
        sldr_r_freq = new QSlider(centralwidget);
        sldr_r_freq->setObjectName(QString::fromUtf8("sldr_r_freq"));
        sldr_r_freq->setGeometry(QRect(90, 230, 160, 21));
        sldr_r_freq->setMaximum(1000);
        sldr_r_freq->setOrientation(Qt::Horizontal);
        sldr_r_amp = new QSlider(centralwidget);
        sldr_r_amp->setObjectName(QString::fromUtf8("sldr_r_amp"));
        sldr_r_amp->setGeometry(QRect(90, 260, 160, 21));
        sldr_r_amp->setOrientation(Qt::Horizontal);
        label = new QLabel(centralwidget);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(20, 230, 71, 16));
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(20, 260, 71, 16));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setGeometry(QRect(20, 130, 71, 16));
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setGeometry(QRect(20, 160, 71, 16));
        sldr_l_freq = new QSlider(centralwidget);
        sldr_l_freq->setObjectName(QString::fromUtf8("sldr_l_freq"));
        sldr_l_freq->setGeometry(QRect(90, 130, 160, 21));
        sldr_l_freq->setMaximum(1000);
        sldr_l_freq->setOrientation(Qt::Horizontal);
        sldr_l_amp = new QSlider(centralwidget);
        sldr_l_amp->setObjectName(QString::fromUtf8("sldr_l_amp"));
        sldr_l_amp->setGeometry(QRect(90, 160, 160, 21));
        sldr_l_amp->setMaximum(99);
        sldr_l_amp->setOrientation(Qt::Horizontal);
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setGeometry(QRect(90, 100, 81, 16));
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setGeometry(QRect(90, 200, 101, 16));
        le_l_freq = new QLineEdit(centralwidget);
        le_l_freq->setObjectName(QString::fromUtf8("le_l_freq"));
        le_l_freq->setGeometry(QRect(270, 130, 51, 25));
        le_l_amp = new QLineEdit(centralwidget);
        le_l_amp->setObjectName(QString::fromUtf8("le_l_amp"));
        le_l_amp->setGeometry(QRect(270, 160, 51, 25));
        le_l_amp->setReadOnly(true);
        le_r_freq = new QLineEdit(centralwidget);
        le_r_freq->setObjectName(QString::fromUtf8("le_r_freq"));
        le_r_freq->setGeometry(QRect(270, 230, 51, 25));
        le_r_freq->setReadOnly(true);
        le_r_amp = new QLineEdit(centralwidget);
        le_r_amp->setObjectName(QString::fromUtf8("le_r_amp"));
        le_r_amp->setGeometry(QRect(270, 260, 51, 25));
        le_r_amp->setReadOnly(true);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 338, 23));
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
        btn_audio_on->setText(QApplication::translate("MainWindow", "Audio On", 0, QApplication::UnicodeUTF8));
        btn_audio_off->setText(QApplication::translate("MainWindow", "Audio Off", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "Frequency", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("MainWindow", "Amplitude", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("MainWindow", "Frequency", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("MainWindow", "Amplitude", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("MainWindow", "Left Channel", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("MainWindow", "Right Channel", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
