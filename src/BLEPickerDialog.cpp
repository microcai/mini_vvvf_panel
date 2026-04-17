#include "BLEPickerDialog.hpp"
#include <QDebug>
#include <QMessageBox>

BLEPickerDialog::BLEPickerDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);

    // 初始化设备模型
    deviceModel = new QStandardItemModel(this);
    deviceModel->setHorizontalHeaderLabels({"设备名称", "地址"});
    deviceList->setModel(deviceModel);
    
    // 设置列表为只读模式
    deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 设置选择模式为单选
    deviceList->setSelectionMode(QAbstractItemView::SingleSelection);

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
        discoveryAgent->deleteLater();
    }
    if (deviceModel) {
        deviceModel->deleteLater();
    }
}

QBluetoothDeviceInfo BLEPickerDialog::getSelectedDevice() const
{
    return selectedDevice;
}

void BLEPickerDialog::onDeviceDiscovered(const QBluetoothDeviceInfo &device)
{
    // 只添加 BLE 设备
    if (device.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
        QStandardItem *nameItem = new QStandardItem(device.name());
        QStandardItem *addressItem = new QStandardItem(device.address().toString());
        // 设置item为不可编辑
        nameItem->setEditable(false);
        addressItem->setEditable(false);
        QList<QStandardItem*> row;
        row << nameItem << addressItem;
        deviceModel->appendRow(row);
    }
}

void BLEPickerDialog::onScanFinished()
{
    // 扫描完成后的处理
    qDebug() << "Scan finished";
}

void BLEPickerDialog::onScanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    // 错误处理
    qDebug() << "Scan error:" << error;
}

void BLEPickerDialog::on_pushButtonConnect_clicked()
{
    QModelIndex currentIndex = deviceList->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::warning(this, "警告", "请先选择一个蓝牙设备");
        return;
    }
    
    int row = currentIndex.row();
    QString address = deviceModel->item(row, 1)->text();
    QString name = deviceModel->item(row, 0)->text();
    
    // 创建设备信息
    selectedDevice = QBluetoothDeviceInfo(QBluetoothAddress(address), name, 0);
    accept();
}
