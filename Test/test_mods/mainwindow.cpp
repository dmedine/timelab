#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stdio.h"

extern "C"{
#include "globals.h"

}


void MainWindow::slotLFreq(int i){

  emit signalSldrChanged(l_freq, i);

}

void MainWindow::slotRFreq(int i){

  emit signalSldrChanged(r_freq, i);

}

void MainWindow::slotLAmp(int i){

  emit signalSldrChanged(l_amp, i);

}

void MainWindow::slotRAmp(int i){

  emit signalSldrChanged(r_amp, i);

}

void MainWindow::slotSldrChanged(SldrCtl *x, int i){

  QString str;
  str.setNum(i);
  //printf("%d\n", i);  
  x->le->setText(str);
  *x->val = (tl_smp)i;
}

void MainWindow::audio_on(void){
  tl_audio_on();
}

void MainWindow::audio_off(void){
  tl_audio_off();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){


  ui->setupUi(this);
  l_freq = new SldrCtl(ui->sldr_l_freq, ui->le_l_freq, &l_freq_val);
  r_freq = new SldrCtl(ui->sldr_r_freq, ui->le_r_freq, &r_freq_val);
  l_amp = new SldrCtl(ui->sldr_l_amp, ui->le_l_amp, &l_amp_val);
  r_amp = new SldrCtl(ui->sldr_r_amp, ui->le_r_amp, &r_amp_val);

  QObject::connect(this, SIGNAL(signalSldrChanged(SldrCtl *, int)), this, SLOT(slotSldrChanged(SldrCtl *, int)));
  QObject::connect(ui->sldr_l_freq, SIGNAL(valueChanged(int)), this, SLOT(slotLFreq(int)));
  QObject::connect(ui->sldr_r_freq, SIGNAL(valueChanged(int)), this, SLOT(slotRFreq(int)));
  QObject::connect(ui->sldr_l_amp, SIGNAL(valueChanged(int)), this, SLOT(slotLAmp(int)));
  QObject::connect(ui->sldr_r_amp, SIGNAL(valueChanged(int)), this, SLOT(slotRAmp(int)));

  QObject::connect(ui->btn_audio_on, SIGNAL(clicked()), this, SLOT(audio_on()));
  QObject::connect(ui->btn_audio_off, SIGNAL(clicked()), this, SLOT(audio_off()));

  setup();
}

MainWindow::~MainWindow(){

  // delete l_freq;
  delete ui;
  
}
