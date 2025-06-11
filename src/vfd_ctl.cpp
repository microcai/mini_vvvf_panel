
#include <QDebug>
#include <QSerialPortInfo>

#include "vfd_ctl.hpp"

#include "qtcoro.hpp"

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

    m_alive_timer.setInterval(100);
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
    if (m_uart.isWritable())
    {
        static const unsigned char get_status_cmd[2] = {0x22, 0x22};

        m_uart.write((const char*)get_status_cmd, 2);
    }
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
    if (m_uart.open(QIODeviceBase::ReadWrite))
    {
        // start reading coroutine
        coro_start(serial_reader_thread());
    }
    m_alive_timer.start();
}

struct vfd_info_buffer
{
    char head;
    char vfd_state;
    uint16_t pwm_freq;
    uint8_t FreqAndTarget[5]; // 0.01 分辨率的当前频率和目标。20bit 每个字段
    uint8_t CRC;
};

std::pair<int, int> unpack_40bit(const uint8_t* FreqAndTarget)
{
    uint32_t num1, num2;

    int num1_converted, num2_converted;

    num1 =
        ((static_cast<uint32_t>(FreqAndTarget[0]) & 0xF0) << 12)
        | ((static_cast<uint32_t>(FreqAndTarget[1]) & 0xFF) << 8)
        | (static_cast<uint32_t>(FreqAndTarget[2]) & 0xFF);

    num1_converted = num1;
    if (num1 > 524287)
        num1_converted = (int) ( num1| 0xFFF00000);

    num2 =
        ((static_cast<uint32_t>(FreqAndTarget[0]) & 0xF) << 16)
        | ((static_cast<uint32_t>(FreqAndTarget[3]) & 0xFF) << 8)
        | (static_cast<uint32_t>(FreqAndTarget[4]) & 0xFF);
    num2_converted = num2;

    if (num2 > 524287)
        num2_converted = (int) ( num2| 0xFFF00000);

    return std::make_pair(num1_converted, num2_converted);
}

bool check_crc(const QByteArray& array)
{
    uint32_t sum = 0;
    for (int i=0; i < array.size() -1 ; i++)
    {
        uint8_t C = static_cast<uint8_t>(array[i]);
        sum += C;
    }
    return (sum & 0xFF) == (static_cast<uint8_t>(array[array.size()-1]));
}

ucoro::awaitable<void> VFDCtrl::serial_reader_thread()
{
    for (;;)
    {
        co_await qtcoro::WaitForQtSignal(&m_uart, SIGNAL(readyRead()));

        co_await qtcoro::coro_delay_ms(2);

        QByteArray read_buffer = m_uart.read(32);

        qDebug() << read_buffer.toHex();

        // calc CRC
        if (!check_crc(read_buffer))
            continue;

        //  TODO, 读取返回消息，并更新到界面上.

        switch (read_buffer[0] & 0xF0)
        {
            case 0x20: // status update
            {

                const vfd_info_buffer* info =  (const vfd_info_buffer*)read_buffer.begin();

                auto freq_and_target = unpack_40bit(info->FreqAndTarget);

                Q_EMIT vfd_info_update(freq_and_target.first/100.0f, freq_and_target.second/100.0f, info->pwm_freq);

            };

        }
    }
}
