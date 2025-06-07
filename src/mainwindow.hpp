
#pragma once

#include <QObject>
#include <QActionGroup>
#include <QMainWindow>
#include "awaitable.hpp"
#include "vfd_ctl.hpp"

#include "ui_mainwindow.h"
#include "mainwindow.moc"

class MainWindow : public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow();

public Q_SLOTS:
    void on_action_About_triggered();

    void on_action_About_Qt_triggered();

    void SelectPort(QAction*);
    void update_menuC(QList<QSerialPortInfo>);

Q_SIGNALS:
    void RequrestPort(QSerialPortInfo);

private:
    QTimer m_sendtimer;
    VFDCtrl vfd_ctrl;

    QMap<QSerialPortInfo, QAction*> uart_ports_menu;
    QActionGroup ports_group;
};

