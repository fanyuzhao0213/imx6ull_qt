#include "serialmodule.h"

/**
 * @brief 构造函数
 * @param parent 父对象
 */
serialModule::serialModule(QObject *parent) : QObject(parent)
{
    // 创建串口对象
    m_serial = new QSerialPort(this);

    // 初始化串口信号与槽
    setupSerialPort();
}

/**
 * @brief 析构函数
 */
serialModule::~serialModule()
{
    closeSerial();  // 析构时关闭串口
}

/**
 * @brief 获取系统可用串口列表
 * @return 串口名列表
 */
QStringList serialModule::availablePorts() const
{
    QStringList portList;
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        portList.append(info.portName());  // 只取串口名称，如 "COM3"、"/dev/ttyUSB0"
    }
    return portList;
}

QByteArray serialModule::packCommand(quint16 cmd, const QByteArray &payload)
{
    QByteArray packet;

    // 1. 包头 AA AA
    packet.append(0xAA);
    packet.append(0xAA);

    // 2. 长度 = 控制指令2字节 + payload长度
    quint16 len = 2 + payload.size();
    packet.append(static_cast<char>((len >> 8) & 0xFF)); // 高字节
    packet.append(static_cast<char>(len & 0xFF));        // 低字节

    // 3. 控制指令 2 字节
    packet.append(static_cast<char>((cmd >> 8) & 0xFF)); // 高字节
    packet.append(static_cast<char>(cmd & 0xFF));        // 低字节

    // 4. 可选扩展数据
    if (!payload.isEmpty()) {
        packet.append(payload);
    }

    // 5. 计算 CRC16
    // CRC计算从“长度字段开始”，长度=len字节
    quint16 crc = crc16(reinterpret_cast<const uint8_t*>(packet.constData() + 2), len);
    packet.append(static_cast<char>(crc & 0xFF));        // CRC低字节
    packet.append(static_cast<char>((crc >> 8) & 0xFF)); // CRC高字节

    return packet;
}

/**
 * @brief 初始化串口信号连接
 */
void serialModule::setupSerialPort()
{
    // 当串口有数据可读时触发 readSerialData
    connect(m_serial, &QSerialPort::readyRead, this, &serialModule::readSerialData);

    // 当串口发生错误时触发 handleError
    connect(m_serial, &QSerialPort::errorOccurred, this, &serialModule::handleError);
}

/* ================= CRC16/Modbus 算法 ================= */
/**
 * @brief CRC16(Modbus标准算法)
 * @param data 数据指针
 * @param len  数据长度
 * @return CRC16 结果
 */
uint16_t serialModule::crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for(uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for(uint8_t j = 0; j < 8; j++) {
            if(crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    return crc;
}

/* ================= 校验 CRC ================= */
/**
 * @brief 校验一帧数据的CRC
 * @param frame 数据帧
 * @param frame_len 数据帧长度
 * @return true=校验通过，false=失败
 */
bool serialModule::checkCrc(const uint8_t *frame, uint16_t frame_len)
{
    if(frame_len < 5) return false; // 最小帧长 = Header(2)+Length(2)+Command(1)

    uint16_t length = (frame[2] << 8) | frame[3];  // 获取帧长度
    if(length + 2 != frame_len - 2) return false;  // 长度不匹配

    uint16_t crc_calc = crc16(&frame[2], length - 2); // 计算CRC
    uint16_t crc_recv = frame[frame_len-2] | (frame[frame_len-1]<<8); // 接收到的CRC

    return crc_calc == crc_recv;
}

// ================= 串口操作 =================

/**
 * @brief 打开串口（详细配置版）
 * @param portName 串口名
 * @param config 串口配置参数
 * @return true=打开成功 false=失败
 */
bool serialModule::openSerial(const QString &portName, const SerialConfig &config)
{
    // 如果串口已打开，先关闭
    if (m_serial->isOpen()) {
        m_serial->close();
    }

    m_serial->setPortName(portName);

    // 尝试打开串口
    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("打开串口失败: %1").arg(portName));
        return false;
    }

    // 设置串口参数
    m_serial->setBaudRate(config.baudRate);
    m_serial->setDataBits(config.dataBits);
    m_serial->setParity(config.parity);
    m_serial->setStopBits(config.stopBits);
    m_serial->setFlowControl(config.flowControl);

    // 控制信号（DTR、RTS）
    m_serial->setDataTerminalReady(config.dtrEnabled);
    m_serial->setRequestToSend(config.rtsEnabled);

    // 保存当前串口信息
    m_currentPort = portName;
    m_currentConfig = config;

    emit serialOpened();  // 通知UI
    emit statusMessage(QString("串口已打开: %1").arg(portName));
    return true;
}

/**
 * @brief 打开串口（快速配置版，只设置波特率）
 * @param portName 串口名
 * @param baudRate 波特率
 */
bool serialModule::openSerial(const QString &portName, int baudRate)
{
    SerialConfig cfg;
    cfg.baudRate = baudRate;
    return openSerial(portName, cfg);
}

/**
 * @brief 关闭串口
 */
void serialModule::closeSerial()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        emit serialClosed();
        emit statusMessage(QString("串口已关闭"));
    }
}

/**
 * @brief 串口是否已打开
 */
bool serialModule::isOpen() const
{
    return m_serial->isOpen();
}

/**
 * @brief 获取串口状态
 */
QString serialModule::portStatus() const
{
    return m_serial->isOpen() ? "已打开" : "已关闭";
}

// ================= 数据发送 =================

/**
 * @brief 发送二进制数据
 */
void serialModule::sendData(const QByteArray &data)
{
    if (!m_serial->isOpen()) return;

    qint64 sent = m_serial->write(data); // 写入数据
    m_totalBytesSent += sent;            // 更新统计
    emit bytesSent(sent);
    emit dataSent(data, m_hexMode);
}

/**
 * @brief 发送字符串数据
 */
void serialModule::sendData(const QString &text)
{
    sendData(text.toUtf8());
}

/**
 * @brief 发送原始数据指针
 */
void serialModule::sendData(const char *data, int size)
{
    sendData(QByteArray(data, size));
}

/**
 * @brief 清空串口缓冲区
 */
void serialModule::clearBuffers()
{
    if (m_serial->isOpen()) {
        m_serial->clear(QSerialPort::AllDirections);
    }
}

// ================= 配置接口 =================

void serialModule::setHexModeEnabled(bool enabled)
{
    m_hexMode = enabled;
}

serialModule::SerialConfig serialModule::currentConfig() const
{
    return m_currentConfig;
}

// ================= 私有槽函数 =================

/**
 * @brief 串口接收槽函数
 */
void serialModule::readSerialData()
{
    if (!m_serial->isOpen()) return;

    QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    m_totalBytesReceived += data.size();
    emit bytesReceived(data.size());   // 通知接收字节数
    emit dataReceived(data);           // 通知原始数据

    if (m_hexMode) {
        emit dataReceivedHex(data);    // 16进制格式
    } else {
        emit dataReceivedText(QString::fromUtf8(data)); // 文本格式
    }
}

/**
 * @brief 串口错误处理
 */
void serialModule::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    emit errorOccurred(m_serial->errorString());
}


