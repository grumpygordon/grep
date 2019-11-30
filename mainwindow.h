#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "background_thread.h"
#include "ui_mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::grep *ui;
    background_thread bg_thread;
};
#endif // MAINWINDOW_H
