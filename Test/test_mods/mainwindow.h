#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QMainWindow>
#include <QMessageBox>
#include <QComboBox>

extern "C"{
#include "tl_core.h"
}

//#include <QHostInfo> // why can't linux find this? don't need it now, but still...



namespace Ui {

class MainWindow;
}

// convenience class for avoiding code duplication
class SldrCtl {


 public:
  QSlider *sldr;
  QLineEdit *le;
  //double *val;
  tl_smp *val; 

  inline SldrCtl (QSlider *x, QLineEdit *y, tl_smp *z){
    sldr = x;
    le = y;
    val = z;
 
  };

  ~SldrCtl(){};
  
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void signalSldrChanged(SldrCtl *x, int i);

private slots:    

  //void audio_off(void);
  //void audio_on(void);
  
    void slotLFreq(int i);
    void slotRFreq(int i);
    void slotLAmp(int i);
    void slotRAmp(int i);
    void slotSldrChanged(SldrCtl *x, int i);

    void audio_on(void);
    void audio_off(void);
    //void amp_chgd(SldrLe &x);
    

private:
    
    SldrCtl *l_freq;
    SldrCtl *r_freq;
    SldrCtl *l_amp;
    SldrCtl *r_amp;

    Ui::MainWindow *ui; // window pointer
};


#endif // MAINWINDOW_H
