
#include <QSerialPortInfo>

#include "vfd_ctl.hpp"

static uint8_t calc_crc(const void* data, int len)
{
	uint8_t sum = 0;
	for (int i = 0; i < len; i ++)
	{
		sum += reinterpret_cast<const uint8_t*>(data)[i];
	}
	return sum;
}

VFDCtrl::VFDCtrl(QObject* parent)
    : QObject(parent)
{
    connect(&m_alive_timer, SIGNAL(timeout()), this, SLOT(keep_alive()));
}

uint32_t convert_to_buma(int n_24bit)
{
    if (n_24bit >= 0)
        return n_24bit;
    // uint32_t new_positive = 1 - n_24bit;
    return (uint32_t) n_24bit;
}

void VFDCtrl::set_target(float target)
{
    int32_t scaled_target = target* 100.0f;
    uint32_t target_converted = convert_to_buma(scaled_target);
	struct {
		uint8_t header;
		uint8_t target_high_and_type; // 目标的最高 4位 & 目标类型
		uint8_t target_mid8; // 目标 中间的8位
		uint8_t target_low8; // 目标 最低 8位
		uint8_t checksum;
	} target_command = {
        0x53,
        static_cast<uint8_t>((target_converted >> 16) & 0xF),
        static_cast<uint8_t>(target_converted >> 8 & 0xFF),
        static_cast<uint8_t>(target_converted & 0xFF),
    };

    target_command.checksum = calc_crc(&target_command, sizeof(target_command) - 1);

    // m_uart.write()
    if (m_uart.isWritable())
    {

        m_uart.write((const char* )&target_command, sizeof target_command);
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
    m_uart.setBaudRate(115200);
    m_uart.setParity(QSerialPort::NoParity);
    m_uart.setStopBits(QSerialPort::OneStop);
    m_uart.open(QIODeviceBase::ReadWrite);
    m_alive_timer.start();
}
