
#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include "awaitable.hpp"

class VFDCtrl : public QObject
{
    Q_OBJECT

public:
    VFDCtrl(QObject* parent = nullptr);

public Q_SLOTS:
    void set_target(float );

    void keep_alive();

    void update_uart();

    void OpenPort(QSerialPortInfo);

    void setBaudRate(int baud_rate);

Q_SIGNALS:
    void uart_list(QList<QSerialPortInfo>);

    void vfd_info_update(float cur_freq, float request_target, int pwm_freq);

protected:
    ucoro::awaitable<void> serial_reader_thread();

private:
    QTimer  m_alive_timer;

    QSerialPort m_uart;
    int m_baud_rate = 115200;

};
