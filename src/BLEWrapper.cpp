#include <QDebug>
#include <QBluetoothUuid>
#include "BLEWrapper.hpp"

BLEWrapper::BLEWrapper(QObject* parent)
    : QObject(parent)
    , m_bleController(nullptr)
    , m_bleService(nullptr)
    , m_isOpen(false)
    , m_isConnected(false)
{
}

BLEWrapper::~BLEWrapper()
{
    close();
}

bool BLEWrapper::open(const QBluetoothDeviceInfo& device)
{
    if (m_isOpen) {
        close();
    }

    m_currentDevice = device;
    m_isOpen = false;
    m_isConnected = false;

    m_bleController = QLowEnergyController::createCentral(device, this);
    if (!m_bleController) {
        qDebug() << "BLEWrapper: 创建控制器失败";
        return false;
    }

    connect(m_bleController, &QLowEnergyController::connected, this, &BLEWrapper::onDeviceConnected);
    connect(m_bleController, &QLowEnergyController::disconnected, this, &BLEWrapper::onDeviceDisconnected);
    connect(m_bleController, &QLowEnergyController::serviceDiscovered, this, &BLEWrapper::onServiceDiscovered);
    connect(m_bleController, &QLowEnergyController::discoveryFinished, this, &BLEWrapper::onServiceScanDone);
    connect(m_bleController, &QLowEnergyController::errorOccurred, this, &BLEWrapper::onControllerError);

    m_bleController->connectToDevice();
    qDebug() << "BLEWrapper: 开始连接设备...";
    return true;
}

void BLEWrapper::close()
{
    cleanup();
    m_isOpen = false;
    m_isConnected = false;
}

bool BLEWrapper::isOpen() const
{
    return m_isOpen;
}

bool BLEWrapper::isWritable() const
{
    return m_isOpen && m_writeCharacteristic.isValid();
}

qint64 BLEWrapper::write(const char* data, qint64 size)
{
    if (!m_isOpen || !m_bleService || !m_writeCharacteristic.isValid()) {
        return -1;
    }

    m_bleService->writeCharacteristic(m_writeCharacteristic, QByteArray(data, size));
    return size;
}

QByteArray BLEWrapper::read(qint64 maxSize)
{
    QByteArray result;
    if (!m_isOpen) {
        return result;
    }

    result = m_readBuffer.left(maxSize);
    m_readBuffer.remove(0, result.size());
    return result;
}

bool BLEWrapper::waitForBytesWritten(int msecs)
{
    Q_UNUSED(msecs);
    return true;
}

bool BLEWrapper::waitForReadyRead(int msecs)
{
    Q_UNUSED(msecs);
    return !m_readBuffer.isEmpty();
}

void BLEWrapper::onDeviceConnected()
{
    qDebug() << "BLEWrapper: 设备已连接";
    m_isConnected = true;
    m_bleController->discoverServices();
}

void BLEWrapper::onDeviceDisconnected()
{
    qDebug() << "BLEWrapper: 设备已断开";
    m_isConnected = false;
    m_isOpen = false;
    Q_EMIT disconnected();
}

void BLEWrapper::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "BLEWrapper: 发现服务:" << serviceUuid.toString();
}

void BLEWrapper::onServiceScanDone()
{
    qDebug() << "BLEWrapper: 服务扫描完成";
    selectServiceAndDiscover();
}

void BLEWrapper::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        qDebug() << "BLEWrapper: 服务详情已发现";
        selectCharacteristic();
    }
}

void BLEWrapper::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    Q_UNUSED(characteristic);
    m_readBuffer.append(value);
    Q_EMIT readyRead();
}

void BLEWrapper::onServiceError(QLowEnergyService::ServiceError error)
{
    qDebug() << "BLEWrapper: 服务错误:" << error;
    Q_EMIT errorOccurred(QString("服务错误: %1").arg(error));
}

void BLEWrapper::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "BLEWrapper: 控制器错误:" << error;
    Q_EMIT errorOccurred(QString("控制器错误: %1").arg(error));
}

void BLEWrapper::selectServiceAndDiscover()
{
    QList<QBluetoothUuid> services = m_bleController->services();
    qDebug() << "BLEWrapper: 共发现" << services.size() << "个服务";

    QList<QBluetoothUuid> customServices;
    for (const QBluetoothUuid &serviceUuid : services) {
        QString uuidStr = serviceUuid.toString().toLower();
        qDebug() << "  服务UUID:" << uuidStr;

        if (!uuidStr.startsWith("{000018") && !uuidStr.startsWith("000018")) {
            customServices.append(serviceUuid);
            qDebug() << "    -> 自定义服务";
        } else {
            qDebug() << "    -> 标准BLE服务，跳过";
        }
    }

    if (!customServices.isEmpty()) {
        qDebug() << "BLEWrapper: 使用第一个自定义服务:" << customServices.first().toString();
        m_bleService = m_bleController->createServiceObject(customServices.first(), this);
        if (m_bleService) {
            connect(m_bleService, &QLowEnergyService::stateChanged, this, &BLEWrapper::onServiceStateChanged);
            connect(m_bleService, &QLowEnergyService::characteristicChanged, this, &BLEWrapper::onCharacteristicChanged);
            connect(m_bleService, &QLowEnergyService::errorOccurred, this, &BLEWrapper::onServiceError);
            m_bleService->discoverDetails();
        }
    } else {
        qDebug() << "BLEWrapper: 未找到自定义服务，使用第一个可用服务";
        if (!services.isEmpty()) {
            m_bleService = m_bleController->createServiceObject(services.first(), this);
            if (m_bleService) {
                connect(m_bleService, &QLowEnergyService::stateChanged, this, &BLEWrapper::onServiceStateChanged);
                connect(m_bleService, &QLowEnergyService::characteristicChanged, this, &BLEWrapper::onCharacteristicChanged);
                connect(m_bleService, &QLowEnergyService::errorOccurred, this, &BLEWrapper::onServiceError);
                m_bleService->discoverDetails();
            }
        }
    }
}

void BLEWrapper::selectCharacteristic()
{
    QList<QLowEnergyCharacteristic> characteristics = m_bleService->characteristics();
    qDebug() << "BLEWrapper: 发现" << characteristics.size() << "个特征";

    for (const QLowEnergyCharacteristic &characteristic : characteristics) {
        qDebug() << "  特征UUID:" << characteristic.uuid().toString();
        qDebug() << "    属性:" << characteristic.properties();

        auto props = characteristic.properties();
        bool hasWrite = props & QLowEnergyCharacteristic::Write;
        bool hasNotify = props & QLowEnergyCharacteristic::Notify;

        if (hasWrite && !hasNotify) {
            m_writeCharacteristic = characteristic;
            qDebug() << "    设置为写特征（只写）";
        } else if (hasWrite && !m_writeCharacteristic.isValid()) {
            m_writeCharacteristic = characteristic;
            qDebug() << "    设置为写特征（可写可通知）";
        }

        if (hasNotify && !m_notifyCharacteristic.isValid()) {
            m_notifyCharacteristic = characteristic;
            qDebug() << "    设置为通知特征";

            const QLowEnergyDescriptor notificationDescriptor = characteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
            if (notificationDescriptor.isValid()) {
                m_bleService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
                qDebug() << "    已启用通知";
            }
        }
    }

    if (m_writeCharacteristic.isValid()) {
        qDebug() << "BLEWrapper: 连接准备完成";
        qDebug() << "  写特征:" << m_writeCharacteristic.uuid().toString();
        qDebug() << "  通知特征:" << m_notifyCharacteristic.uuid().toString();
        m_isOpen = true;
        Q_EMIT connected();
    } else {
        qDebug() << "BLEWrapper: 未找到可写特征";
        Q_EMIT errorOccurred("未找到可写特征");
    }
}

void BLEWrapper::cleanup()
{
    if (m_bleService) {
        m_bleService->deleteLater();
        m_bleService = nullptr;
    }
    if (m_bleController) {
        m_bleController->disconnectFromDevice();
        m_bleController->deleteLater();
        m_bleController = nullptr;
    }
    m_writeCharacteristic = QLowEnergyCharacteristic();
    m_notifyCharacteristic = QLowEnergyCharacteristic();
    m_readBuffer.clear();
}
