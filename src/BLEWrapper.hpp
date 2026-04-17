#pragma once

#include <QObject>
#include <QByteArray>
#include <QBluetoothDeviceInfo>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyCharacteristic>

class BLEWrapper : public QObject
{
    Q_OBJECT

public:
    explicit BLEWrapper(QObject* parent = nullptr);
    ~BLEWrapper();

    bool open(const QBluetoothDeviceInfo& device);
    void close();
    bool isOpen() const;
    bool isWritable() const;
    qint64 write(const char* data, qint64 size);
    QByteArray read(qint64 maxSize);

    bool waitForBytesWritten(int msecs);
    bool waitForReadyRead(int msecs);

    Q_SIGNALS:
    void readyRead();
    void connected();
    void disconnected();
    void errorOccurred(const QString& error);

private Q_SLOTS:
    void onDeviceConnected();
    void onDeviceDisconnected();
    void onServiceDiscovered(const QBluetoothUuid &serviceUuid);
    void onServiceScanDone();
    void onServiceStateChanged(QLowEnergyService::ServiceState state);
    void onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value);
    void onServiceError(QLowEnergyService::ServiceError error);
    void onControllerError(QLowEnergyController::Error error);

private:
    void selectServiceAndDiscover();
    void selectCharacteristic();
    void cleanup();

    QLowEnergyController* m_bleController;
    QLowEnergyService* m_bleService;
    QLowEnergyCharacteristic m_writeCharacteristic;
    QLowEnergyCharacteristic m_notifyCharacteristic;
    QByteArray m_readBuffer;
    QBluetoothDeviceInfo m_currentDevice;
    bool m_isOpen;
    bool m_isConnected;
};
