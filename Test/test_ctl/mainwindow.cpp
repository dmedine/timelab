#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stdio.h"

extern "C"{
#include "globals.h"
}

void MainWindow::stop(void){

  printf("stopping...\n");

  done = 0;

}

void MainWindow::start(void){

  printf("starting...\n");

  setup();

}

void MainWindow::set_val(int i){
  
  value = (tl_smp)i;
  
}

void MainWindow::go_bang(void){
  do_bang = 1;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow){

  ui->setupUi(this);
  QObject::connect(ui->btn_stop, SIGNAL(clicked()), this, SLOT(stop()));
  QObject::connect(ui->btn_start, SIGNAL(clicked()), this, SLOT(start()));

  QObject::connect(ui->btn_bang, SIGNAL(clicked()), this, SLOT(go_bang()));
  QObject::connect(ui->spn_val, SIGNAL(valueChanged(int)), this, SLOT(set_val(int)));
  

}

MainWindow::~MainWindow(){

  delete ui;

}
