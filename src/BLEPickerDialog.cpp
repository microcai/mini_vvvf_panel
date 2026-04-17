#include "BLEPickerDialog.hpp"

BLEPickerDialog::BLEPickerDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    // 初始化设备模型
    deviceModel = new QStandardItemModel(this);
    deviceModel->setHorizontalHeaderLabels({"设备名称", "地址"});
    deviceList->setModel(deviceModel);

    // 初始化蓝牙扫描代理
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    discoveryAgent->setLowEnergyDiscoveryTimeout(5000);

    // 连接信号槽
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BLEPickerDialog::onDeviceDiscovered);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BLEPickerDialog::onScanFinished);
    connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &BLEPickerDialog::onScanError);

    // 开始扫描 BLE 设备
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

BLEPickerDialog::~BLEPickerDialog()
{
    if (discoveryAgent) {
        discoveryAgent->stop();
        delete discoveryAgent;
    }
    if (deviceModel) {
        delete deviceModel;
    }
}

void BLEPickerDialog::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    // 只添加 BLE 设备
    // if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        QStandardItem *nameItem = new QStandardItem(device.name());
        QStandardItem *addressItem = new QStandardItem(device.address().toString());
        QList<QStandardItem*> row;
        row << nameItem << addressItem;
        deviceModel->appendRow(row);
    // }
}

void BLEPickerDialog::onScanFinished()
{
    // 扫描完成后的处理
}

void BLEPickerDialog::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    // 错误处理
}
