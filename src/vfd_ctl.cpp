
#include <QSerialPortInfo>

#include "vfd_ctl.hpp"

VFDCtrl::VFDCtrl(QObject* parent)
    : QObject(parent)
{
    connect(&m_alive_timer, SIGNAL(timeout()), this, SLOT(keep_alive()));
}

void VFDCtrl::set_target(float target)
{
    // m_uart.write()
    if (m_uart.isWritable())
    {
        m_uart.write();
    }
}


void VFDCtrl::keep_alive()
{

}

void VFDCtrl::update_uart()
{
    auto all_uart = QSerialPortInfo::availablePorts();

    Q_EMIT uart_list(all_uart);
}

void VFDCtrl::OpenPort(QSerialPortInfo port)
{
    m_alive_timer.stop();
    if (m_uart.isOpen())
        m_uart.close();
    m_uart.setPort(port);
    m_uart.open(QIODeviceBase::ReadWrite);
    m_alive_timer.start();
}
