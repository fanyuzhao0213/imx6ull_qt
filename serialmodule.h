#ifndef SERIALMODULE_H
#define SERIALMODULE_H

#include <QObject>
#include <cstdint>
#include <QSerialPort>
#include <QSerialPortInfo>

/**
 * @brief 串口管理类 - 针对固定串口的简化版本
 * 
 * 特点：
 * - 固定串口设备，不需要动态扫描
 * - 根据用户选择的波特率、停止位等参数打开串口
 * - 支持HEX/文本模式接收
 * - 发送/接收数据统计
 * - 错误与状态信号反馈
 */

class serialModule : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 串口配置结构体
     */
    struct SerialConfig {
        int baudRate = QSerialPort::Baud115200;          ///< 波特率，默认115200
        QSerialPort::DataBits dataBits = QSerialPort::Data8;  ///< 数据位，默认8
        QSerialPort::Parity parity = QSerialPort::NoParity;   ///< 校验位，默认无
        QSerialPort::StopBits stopBits = QSerialPort::OneStop; ///< 停止位，默认1
        QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl; ///< 流控，默认无
        bool dtrEnabled = false; ///< DTR控制
        bool rtsEnabled = false; ///< RTS控制
    };

    // CRC16/Modbus 计算
    uint16_t crc16(const uint8_t *data, uint16_t len);

    // 校验 CRC 是否正确
    bool checkCrc(const uint8_t *frame, uint16_t frame_len);

    explicit serialModule(QObject *parent = nullptr);
    ~serialModule();

    // ================= 串口操作接口 =================

    /**
     * @brief 打开串口，使用完整配置
     * @param portName 串口设备路径（如 /dev/ttyAMA0）
     * @param config 串口配置参数
     * @return 打开成功返回true，失败返回false
     */
    bool openSerial(const QString &portName, const SerialConfig &config);

    /**
     * @brief 打开串口，只设置波特率，其他为默认
     * @param portName 串口设备路径
     * @param baudRate 波特率
     * @return 打开成功返回true，失败返回false
     */
    bool openSerial(const QString &portName, int baudRate);

    /**
     * @brief 关闭串口
     */
    void closeSerial();

    /**
     * @brief 判断串口是否已打开
     * @return true已打开，false未打开
     */
    bool isOpen() const;

    /**
     * @brief 获取串口当前状态
     * @return 状态字符串
     */
    QString portStatus() const;

    // ================= 数据发送接口 =================

    /**
     * @brief 发送字节数组
     */
    void sendData(const QByteArray &data);

    /**
     * @brief 发送文本字符串
     */
    void sendData(const QString &text);

    /**
     * @brief 发送原始数据指针
     */
    void sendData(const char *data, int size);

    /**
     * @brief 清空串口收发缓冲区
     */
    void clearBuffers();

    // ================= 配置接口 =================

    /**
     * @brief 设置HEX接收模式
     * @param enabled true为HEX模式，false为文本模式
     */
    void setHexModeEnabled(bool enabled);

    /**
     * @brief 获取当前串口配置
     */
    SerialConfig currentConfig() const;

	// 添加到 public 接口
	/**
	 * @brief 获取系统可用串口列表
	 * @return 串口设备名称列表
	 */
	QStringList availablePorts() const;
    QByteArray packCommand(quint16 cmd, const QByteArray &payload);  //发送数据打包函数
signals:
    // ================= 数据接收信号 =================
    void dataReceived(const QByteArray &data);
    void dataReceivedText(const QString &text);
    void dataReceivedHex(const QByteArray &data);

    // ================= 状态与错误 =================
    void errorOccurred(const QString &errorMsg);
    void statusMessage(const QString &message);

    // ================= 串口连接状态 =================
    void serialOpened();
    void serialClosed();

    // ================= 统计信号 =================
    void bytesSent(qint64 bytes);
    void bytesReceived(qint64 bytes);
    void dataSent(const QByteArray &data, bool isHex);

private slots:
    /**
     * @brief 读取串口数据
     */
    void readSerialData();

    /**
     * @brief 串口错误处理
     */
    void handleError(QSerialPort::SerialPortError error);

private:
    void setupSerialPort(); ///< 初始化串口对象

    QSerialPort *m_serial = nullptr;   ///< 串口对象
    SerialConfig m_currentConfig;      ///< 当前串口配置
    QString m_currentPort;             ///< 当前串口路径
    qint64 m_totalBytesSent = 0;       ///< 总发送字节数
    qint64 m_totalBytesReceived = 0;   ///< 总接收字节数
    bool m_hexMode = false;            ///< HEX模式标志
};

#endif
