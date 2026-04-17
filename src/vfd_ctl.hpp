#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QBluetoothDeviceInfo>
#include "awaitable.hpp"

class BLEWrapper;

class VFDCtrl : public QObject
{
    Q_OBJECT

public:
    VFDCtrl(QObject* parent = nullptr);
    ~VFDCtrl();

public Q_SLOTS:
    void set_target(float );

    void keep_alive();

    void update_uart();

    void OpenPort(QSerialPortInfo);
    void OpenBLE(const QBluetoothDeviceInfo& device);

    void setBaudRate(int baud_rate);

Q_SIGNALS:
    void uart_list(QList<QSerialPortInfo>);

    void vfd_info_update(float cur_freq, float request_target, int pwm_freq, char vfd_state);

protected:
    ucoro::awaitable<void> serial_reader_thread();
    ucoro::awaitable<void> ble_reader_thread();

private:
    QTimer  m_alive_timer;

    QSerialPort m_uart;
    BLEWrapper* m_ble;
    int m_baud_rate = 115200;
    bool m_isBLE = false;

    void writeData(const char* data, int size);
};
