
#include <numbers>
#include <iostream>
#include "Tab1.hpp"
#include <QMessageBox>
#include "qtcoro.hpp"

Tab1::Tab1(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    setupUi(this);
}

void Tab1::on_targetSlider_valueChanged(int v)
{
    this->target_freq->display( v / 100.0 );

    Q_EMIT set_target(v / 100.0);
}

void Tab1::on_stopButton_pressed()
{
    this->target_freq->display(0);
}


void Tab1::update_vfd_info(float cur_freq, float request_target, int pwm_freq)
{
    this->output_freq->display(cur_freq);
    this->carrier->display(pwm_freq);

    const QSignalBlocker blocker(this->targetSlider);

    this->target_freq->display( request_target );

    this->targetSlider->setValue(request_target * 100.0);

}