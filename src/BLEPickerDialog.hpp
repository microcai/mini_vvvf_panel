#pragma once

#include <QDialog>
#include "ui_BLEPickerDialog.h"

class BLEPickerDialog : public QDialog, public Ui::BLEPickerDialog
{
    Q_OBJECT
public:
    BLEPickerDialog(QWidget *parent = nullptr);
    ~BLEPickerDialog();
};
