#pragma once

#include <QDialog>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QStandardItemModel>
#include "ui_BLEPickerDialog.h"

class BLEPickerDialog : public QDialog, public Ui::BLEPickerDialog
{
    Q_OBJECT
public:
    BLEPickerDialog(QWidget *parent = nullptr);
    ~BLEPickerDialog();

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &device);
    void onScanFinished();
    void onScanError(QBluetoothDeviceDiscoveryAgent::Error error);

private:
    QBluetoothDeviceDiscoveryAgent *discoveryAgent;
    QStandardItemModel *deviceModel;
};
