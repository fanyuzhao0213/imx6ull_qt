#include "smartdevicemodule.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

smartDeviceModule::smartDeviceModule(QObject *parent)
    : QObject(parent), beepCount(0), beepTarget(0), beepState(false), interval(500)
{
    // 初始化 LED 为手动控制
    setLedTrigger("none");

    // 定时器用于控制 BEEP 闹钟节奏
    connect(&timer, &QTimer::timeout, this, &smartDeviceModule::toggleBeep);
    // 定时器用于ap3216C read data
    connect(&readAp3216c_timer, &QTimer::timeout, this, &smartDeviceModule::timer_timeout);
}

smartDeviceModule::~smartDeviceModule()
{
    // 析构时关闭所有外设
    turnLedOff();
    turnAlarmOff();
    beepOff();
    if (readAp3216c_timer.isActive())
        readAp3216c_timer.stop();
}

/**
 * @brief writeSysFile
 * 写入 sysfs 文件的统一接口
 * @param path 文件路径
 * @param value 写入值
 * @return true 成功 / false 失败
 */
bool smartDeviceModule::writeSysFile(const QString &path, const QString &value)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Cannot open" << path;
        return false;
    }

    QTextStream out(&file);
    out << value;  // 写入文件
    file.close();
    return true;
}

// ========================
// LED 控制
// ========================
void smartDeviceModule::setLedTrigger(const QString &mode)
{
    QString path = QString(ledPath) + "trigger";
    writeSysFile(path, mode);
}

void smartDeviceModule::turnLedOn()
{
    QString path = QString(ledPath) + "brightness";
    writeSysFile(path, "1");
}

void smartDeviceModule::turnLedOff()
{
    QString path = QString(ledPath) + "brightness";
    writeSysFile(path, "0");
}

// ========================
// ALARM 控制
// ========================
void smartDeviceModule::turnAlarmOn()
{
    writeSysFile(QString(alarmPath) + "enable", "1");
}

void smartDeviceModule::turnAlarmOff()
{
    writeSysFile(QString(alarmPath) + "enable", "0");
}

// ========================
// BEEP 控制
// ========================
void smartDeviceModule::beepOn()
{
    writeSysFile(QString(beepPath) + "brightness", "1");
}

void smartDeviceModule::beepOff()
{
    writeSysFile(QString(beepPath) + "brightness", "0");
}

// ========================
// 闹钟控制
// ========================
void smartDeviceModule::startAlarm(int times, int intervalMs)
{
    if (times <= 0) return;

    beepTarget = times * 2; // 每次滴滴包含 BEEP_ON + BEEP_OFF 两个状态
    beepCount = 0;
    beepState = false;
    interval = intervalMs;

    timer.start(interval);
}

void smartDeviceModule::stopAlarm()
{
    timer.stop();
    beepOff();
    beepState = false;
    beepCount = 0;

    emit alarmStopped(); // 发信号通知外部（MainWindow）
}

void smartDeviceModule::toggleBeep()
{
    if (beepCount >= beepTarget) {
        stopAlarm();  // 完成目标次数自动停止
        return;
    }

    if (beepState) {
        beepOff();
        beepState = false;
    } else {
        beepOn();
        beepState = true;
    }

    beepCount++;
}


/*AP3216C*/
/**
 * @brief 启动或停止传感器数据采集
 * @param start true=启动，false=停止
 */
void smartDeviceModule::setCapture(bool start)
{
    if (start)
        readAp3216c_timer.start(1000);  // 1秒更新一次
    else
        readAp3216c_timer.stop();
}

/* 模拟读取ALS数据 */
QString smartDeviceModule::readAlsData()
{
    return readSensorValue(AP3216C_ALS_PATH);
}

/* 模拟读取PS数据 */
QString smartDeviceModule::readPsData()
{
    return readSensorValue(AP3216C_PS_PATH);
}

/* 模拟读取IR数据 */
QString smartDeviceModule::readIrData()
{
    return readSensorValue(AP3216C_IR_PATH);
}

/* 定时器回调：读取传感器并发射信号 */
void smartDeviceModule::timer_timeout()
{

    Ap3216cData data;
    data.als = readAlsData();
    data.ps  = readPsData();
    data.ir  = readIrData();
    emit ap3216cDataUpdated(data);  // 发射结构体数据
}


// 通用文件读取
QString smartDeviceModule::readSensorValue(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Open file error:" << filePath;
        return "open file error!";
    }

    QByteArray buf = file.readAll();  // 读取整个文件内容
    file.close();

    if (buf.isEmpty()) {
        qDebug() << "Read file error or file empty:" << filePath;
        return "read data error!";
    }

    QString value = QString(buf).trimmed();  // 去掉换行符和空白
    return value;
}
