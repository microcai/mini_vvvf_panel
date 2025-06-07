
#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>

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

Q_SIGNALS:
    void uart_list(QList<QSerialPortInfo>);

private:
    QTimer  m_alive_timer;

    QSerialPort m_uart;

};
