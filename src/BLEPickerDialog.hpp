#pragma once

#include <QDialog>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QBluetoothSocket>
#include <QStandardItemModel>
#include "ui_BLEPickerDialog.h"

class BLEPickerDialog : public QDialog, public Ui::BLEPickerDialog
{
    Q_OBJECT
public:
    BLEPickerDialog(QWidget *parent = nullptr);
    ~BLEPickerDialog();
    
    QBluetoothDeviceInfo getSelectedDevice() const;

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void on_pushButtonConnect_clicked();

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QStandardItemModel *deviceModel;
    QBluetoothDeviceInfo selectedDevice;
};
