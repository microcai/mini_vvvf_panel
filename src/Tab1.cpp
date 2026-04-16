
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

void Tab1::on_targetSlider_sliderPressed()
{
    is_slider_dragging = true;
}

void Tab1::on_targetSlider_sliderReleased()
{
    is_slider_dragging = false;
}


void Tab1::update_vfd_info(float cur_freq, float request_target, int pwm_freq, char vfd_state)
{
    this->output_freq->display(cur_freq);
    this->carrier->display(pwm_freq);

    // 检查 vfd_state 第三位是否为 1（表示频率已追上）
    bool freq_caught_up = (vfd_state & 0x04) != 0;

    // 只有当频率已追上或者用户没有在拖拽滑块时，才更新滑块位置
    if (freq_caught_up || !is_slider_dragging)
    {
        const QSignalBlocker blocker(this->targetSlider);
        this->target_freq->display( request_target );
        this->targetSlider->setValue(request_target * 100.0);
    }
    else
    {
        // 当频率未追上且用户正在拖拽滑块时，只更新显示，不更新滑块位置
        this->target_freq->display( request_target );
    }

}