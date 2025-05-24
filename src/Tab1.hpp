
#pragma once

#include <QWidget>
#include "ui_Tab1.h"
#include "qtcoro.hpp"

class Tab1 : public QWidget, Ui_Tab1
{
    Q_OBJECT

public:
    explicit Tab1(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

Q_SIGNALS:
    void set_target(float);

public Q_SLOTS:
    void on_targetSlider_valueChanged(int);
    void on_stopButton_pressed();

private:
    qreal freq1, freq2, freq;
};

