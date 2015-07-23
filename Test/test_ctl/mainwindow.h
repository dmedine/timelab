#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QMessageBox>
#include <QComboBox>
//#include <QHostInfo> // why can't linux find this? don't need it now, but still...



namespace Ui {

class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:    

    void stop(void);
    void start(void);
    void set_val(int i);
    void go_bang(void);

private:

    Ui::MainWindow *ui; // window pointer
    tl_procession *procession; // gateway to timelab scheduler
};


#endif // MAINWINDOW_H
