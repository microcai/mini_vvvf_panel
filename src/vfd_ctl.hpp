#pragma once

#include <QObject>
#include <QTimer>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include "awaitable.hpp"

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

private slots:
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceScanDone();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError error);
    void onControllerError(QLowEnergyController::Error error);

private:
    QTimer  m_alive_timer;

    QSerialPort m_uart;
    QLowEnergyController* m_bleController;
    QLowEnergyService* m_bleService;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QLowEnergyCharacteristic m_notifyCharacteristic;
    int m_baud_rate = 115200;
    bool m_isBLE = false;
    QBluetoothDeviceInfo m_currentDevice;

    void writeData(const char* data, int size);
};
