
#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPort>

class VFDCtrl : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void set_target(float );

    void keep_alive();


private:
    QTimer  m_alive_timer;

    QSerialPort m_uart;

};
