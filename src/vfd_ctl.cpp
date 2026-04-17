#include <QDebug>
#include <QSerialPortInfo>

#include "vfd_ctl.hpp"

#include "qtcoro.hpp"

struct vfd_info_buffer
{
    char head;
    char vfd_state; // 位状态。1位为运行状态，2位为 pwm 载波调制是否同步，3 位 指示变频器的当前频率是否已经追上设定频率
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
    if (array.size() == 0)
        return false;
    uint32_t sum = 0;
    for (int i=0; i < array.size() -1 ; i++)
    {
        uint8_t C = static_cast<uint8_t>(array[i]);
        sum += C;
    }
    return (sum & 0xFF) == (static_cast<uint8_t>(array[array.size()-1]));
}

static uint8_t calc_crc(const void* data, int len)
{
	uint8_t sum = 0;
	for (int i = 0; i < len; i ++)
	{
		sum += reinterpret_cast<const uint8_t*>(data)[i];
	}
	return sum;
}

uint32_t convert_to_buma(int n_24bit)
{
    if (n_24bit >= 0)
        return n_24bit;
    // uint32_t new_positive = 1 - n_24bit;
    return (uint32_t) n_24bit;
}

VFDCtrl::VFDCtrl(QObject* parent)
    : QObject(parent), m_bleController(nullptr), m_bleService(nullptr)
{
    connect(&m_alive_timer, SIGNAL(timeout()), this, SLOT(keep_alive()));

    m_alive_timer.setInterval(100);
}

VFDCtrl::~VFDCtrl()
{
    if (m_bleService) {
        m_bleService->deleteLater();
    }
    if (m_bleController) {
        m_bleController->disconnectFromDevice();
        m_bleController->deleteLater();
    }
}

void VFDCtrl::writeData(const char* data, int size)
{
    if (m_isBLE && m_bleService && m_writeCharacteristic.isValid())
    {
        m_bleService->writeCharacteristic(m_writeCharacteristic, QByteArray(data, size));
    }
    else if (!m_isBLE && m_uart.isWritable())
    {
        m_uart.write(data, size);
    }
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

    writeData((const char* )&target_command, sizeof target_command);
}


void VFDCtrl::keep_alive()
{
    static const unsigned char get_status_cmd[2] = {0x22, 0x22};
    writeData((const char*)get_status_cmd, 2);
}

void VFDCtrl::update_uart()
{
    auto all_uart = QSerialPortInfo::availablePorts();

    Q_EMIT uart_list(all_uart);
}



void VFDCtrl::OpenPort(QSerialPortInfo port)
{
    m_alive_timer.stop();
    
    // 关闭蓝牙连接
    if (m_bleService) {
        m_bleService->deleteLater();
        m_bleService = nullptr;
    }
    if (m_bleController) {
        m_bleController->disconnectFromDevice();
        m_bleController->deleteLater();
        m_bleController = nullptr;
    }
    
    if (m_uart.isOpen())
        m_uart.close();
    m_uart.setPort(port);
    m_uart.setDataBits(QSerialPort::Data8);
    m_uart.setBaudRate(m_baud_rate);
    m_uart.setParity(QSerialPort::NoParity);
    m_uart.setStopBits(QSerialPort::OneStop);
    
    m_isBLE = false;
    
    if (m_uart.open(QIODeviceBase::ReadWrite))
    {
        // start reading coroutine
        coro_start(serial_reader_thread());
    }
    m_alive_timer.start();
}

void VFDCtrl::OpenBLE(const QBluetoothDeviceInfo& device)
{
    m_alive_timer.stop();
    
    // 关闭串口连接
    if (m_uart.isOpen())
        m_uart.close();
    
    // 关闭旧的蓝牙连接
    if (m_bleService) {
        m_bleService->deleteLater();
        m_bleService = nullptr;
    }
    if (m_bleController) {
        m_bleController->disconnectFromDevice();
        m_bleController->deleteLater();
        m_bleController = nullptr;
    }
    
    m_currentDevice = device;
    m_isBLE = true;
    
    // 创建BLE控制器
    m_bleController = QLowEnergyController::createCentral(device, this);
    
    // 连接BLE控制器的信号槽
    connect(m_bleController, &QLowEnergyController::connected, this, &VFDCtrl::onDeviceConnected);
    connect(m_bleController, &QLowEnergyController::disconnected, this, &VFDCtrl::onDeviceDisconnected);
    connect(m_bleController, &QLowEnergyController::serviceDiscovered, this, &VFDCtrl::onServiceDiscovered);
    connect(m_bleController, &QLowEnergyController::discoveryFinished, this, &VFDCtrl::onServiceScanDone);
    connect(m_bleController, &QLowEnergyController::errorOccurred, this, &VFDCtrl::onControllerError);
    
    // 连接到设备
    m_bleController->connectToDevice();
    qDebug() << "开始连接到BLE设备...";
}

void VFDCtrl::onDeviceConnected()
{
    qDebug() << "BLE设备已连接";
    // 连接成功后，开始服务发现
    m_bleController->discoverServices();
}

void VFDCtrl::onDeviceDisconnected()
{
    qDebug() << "BLE设备已断开连接";
    m_alive_timer.stop();
    m_isBLE = false;
}

void VFDCtrl::onServiceDiscovered(const QBluetoothUuid &serviceUuid)
{
    qDebug() << "发现服务:" << serviceUuid.toString();
}

void VFDCtrl::onServiceScanDone()
{
    qDebug() << "服务扫描完成";
    
    // 获取所有发现的服务
    QList<QBluetoothUuid> services = m_bleController->services();
    qDebug() << "共发现" << services.size() << "个服务";
    
    // 筛选自定义服务（排除标准BLE服务，标准服务以000018xx开头）
    QList<QBluetoothUuid> customServices;
    for (const QBluetoothUuid &serviceUuid : services) {
        QString uuidStr = serviceUuid.toString().toLower();
        qDebug() << "  服务UUID:" << uuidStr;
        
        // 排除标准BLE服务（000018xx-0000-1000-8000-00805f9b34fb）
        if (!uuidStr.startsWith("{000018") && !uuidStr.startsWith("000018")) {
            customServices.append(serviceUuid);
            qDebug() << "    -> 自定义服务";
        } else {
            qDebug() << "    -> 标准BLE服务，跳过";
        }
    }
    
    // 使用第一个自定义服务
    if (!customServices.isEmpty()) {
        qDebug() << "使用第一个自定义服务:" << customServices.first().toString();
        m_bleService = m_bleController->createServiceObject(customServices.first(), this);
        if (m_bleService) {
            connect(m_bleService, &QLowEnergyService::stateChanged, this, &VFDCtrl::onServiceStateChanged);
            connect(m_bleService, &QLowEnergyService::characteristicRead, this, &VFDCtrl::onCharacteristicRead);
            connect(m_bleService, &QLowEnergyService::characteristicWritten, this, &VFDCtrl::onCharacteristicWritten);
            connect(m_bleService, &QLowEnergyService::characteristicChanged, this, &VFDCtrl::onCharacteristicChanged);
            connect(m_bleService, &QLowEnergyService::errorOccurred, this, &VFDCtrl::onServiceError);
            
            // 发现服务详情
            m_bleService->discoverDetails();
        }
    } else {
        qDebug() << "未找到自定义服务，使用第一个可用服务";
        if (!services.isEmpty()) {
            m_bleService = m_bleController->createServiceObject(services.first(), this);
            if (m_bleService) {
                connect(m_bleService, &QLowEnergyService::stateChanged, this, &VFDCtrl::onServiceStateChanged);
                connect(m_bleService, &QLowEnergyService::characteristicRead, this, &VFDCtrl::onCharacteristicRead);
                connect(m_bleService, &QLowEnergyService::characteristicWritten, this, &VFDCtrl::onCharacteristicWritten);
                connect(m_bleService, &QLowEnergyService::characteristicChanged, this, &VFDCtrl::onCharacteristicChanged);
                connect(m_bleService, &QLowEnergyService::errorOccurred, this, &VFDCtrl::onServiceError);
                
                // 发现服务详情
                m_bleService->discoverDetails();
            }
        }
    }
}

void VFDCtrl::onServiceStateChanged(QLowEnergyService::ServiceState state)
{
    if (state == QLowEnergyService::RemoteServiceDiscovered) {
        qDebug() << "服务详情已发现";
        
        // 查找可写特征和通知特征
        QList<QLowEnergyCharacteristic> characteristics = m_bleService->characteristics();
        qDebug() << "发现" << characteristics.size() << "个特征";
        
        for (const QLowEnergyCharacteristic &characteristic : characteristics) {
            qDebug() << "  特征UUID:" << characteristic.uuid().toString();
            qDebug() << "    属性:" << characteristic.properties();
            
            auto props = characteristic.properties();
            bool hasWrite = props & QLowEnergyCharacteristic::Write;
            bool hasNotify = props & QLowEnergyCharacteristic::Notify;
            
            // 选择写特征：优先选择支持Write但不支持Notify的特征（用于写命令）
            if (hasWrite && !hasNotify) {
                m_writeCharacteristic = characteristic;
                qDebug() << "    设置为写特征（只写）";
            }
            // 如果没有找到只写的特征，才使用既可写又可通知的特征
            else if (hasWrite && !m_writeCharacteristic.isValid()) {
                m_writeCharacteristic = characteristic;
                qDebug() << "    设置为写特征（可写可通知）";
            }
            
            // 选择通知特征：使用第一个支持Notify的特征（用于读数据）
            if (hasNotify && !m_notifyCharacteristic.isValid()) {
                m_notifyCharacteristic = characteristic;
                qDebug() << "    设置为通知特征";
                
                // 启用通知
                const QLowEnergyDescriptor notificationDescriptor = characteristic.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
                if (notificationDescriptor.isValid()) {
                    m_bleService->writeDescriptor(notificationDescriptor, QByteArray::fromHex("0100"));
                    qDebug() << "    已启用通知";
                }
            }
        }
        
        // 如果找到了可写特征，开始数据传输
        if (m_writeCharacteristic.isValid()) {
            qDebug() << "BLE连接准备完成，开始数据传输";
            qDebug() << "  写特征:" << m_writeCharacteristic.uuid().toString();
            qDebug() << "  通知特征:" << m_notifyCharacteristic.uuid().toString();
            coro_start(ble_reader_thread());
            m_alive_timer.start();
        } else {
            qDebug() << "未找到可写特征，无法进行数据传输";
        }
    }
}

void VFDCtrl::onCharacteristicRead(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qDebug() << "读取特征:" << characteristic.uuid().toString() << "值:" << value.toHex();
}

void VFDCtrl::onCharacteristicWritten(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qDebug() << "写入特征:" << characteristic.uuid().toString() << "值:" << value.toHex();
}

void VFDCtrl::onCharacteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &value)
{
    qDebug() << "特征变化:" << characteristic.uuid().toString() << "值:" << value.toHex();
    
    // 处理接收到的数据
    QByteArray read_buffer = value;
    
    // calc CRC
    if (!check_crc(read_buffer))
        return;

    // 读取返回消息，并更新到界面上
    switch (read_buffer[0] & 0xF0)
    {
        case 0x20: // status update
        {
            const vfd_info_buffer* info = (const vfd_info_buffer*)read_buffer.constData();
            auto freq_and_target = unpack_40bit(info->FreqAndTarget);
            Q_EMIT vfd_info_update(freq_and_target.first/100.0f, freq_and_target.second/100.0f, info->pwm_freq, info->vfd_state);
        }
    }
}

void VFDCtrl::onServiceError(QLowEnergyService::ServiceError error)
{
    qDebug() << "服务错误:" << error;
}

void VFDCtrl::onControllerError(QLowEnergyController::Error error)
{
    qDebug() << "控制器错误:" << error;
}

void VFDCtrl::setBaudRate(int baud_rate)
{
    m_baud_rate = baud_rate;
    if (!m_isBLE && m_uart.isOpen())
    {
        m_uart.setBaudRate(m_baud_rate);
    }
    // 蓝牙连接不需要设置波特率
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

                const vfd_info_buffer* info =  (const vfd_info_buffer*)read_buffer.constData();

                auto freq_and_target = unpack_40bit(info->FreqAndTarget);

                Q_EMIT vfd_info_update(freq_and_target.first/100.0f, freq_and_target.second/100.0f, info->pwm_freq, info->vfd_state);

            };

        }
    }
}

ucoro::awaitable<void> VFDCtrl::ble_reader_thread()
{
    for (;;)
    {
        co_await qtcoro::coro_delay_ms(100);
        
        // BLE数据通过onCharacteristicChanged槽函数接收，这里保持协程运行即可
        if (!m_isBLE || !m_bleService) {
            co_return;
        }
    }
}
