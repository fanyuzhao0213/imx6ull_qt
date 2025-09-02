#include "smartdevicemodule.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>

smartDeviceModule::smartDeviceModule(QObject *parent)
    : QObject(parent), beepCount(0), beepTarget(0), beepState(false), interval(500)
{
    // 设置各外设 sysfs 路径
    ledPath   = "/sys/class/leds/sys-led/";
    alarmPath = "/sys/class/alarm/alarm0/";  // 如果系统有 ALARM 外设
    beepPath  = "/sys/class/leds/beep/";

    // 初始化 LED 为手动控制
    setLedTrigger("none");

    // 定时器用于控制 BEEP 闹钟节奏
    connect(&timer, &QTimer::timeout, this, &smartDeviceModule::toggleBeep);
}

smartDeviceModule::~smartDeviceModule()
{
    // 析构时关闭所有外设
    turnLedOff();
    turnAlarmOff();
    beepOff();
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
    writeSysFile(ledPath + "trigger", mode);
}

void smartDeviceModule::turnLedOn()
{
    writeSysFile(ledPath + "brightness", "1");
}

void smartDeviceModule::turnLedOff()
{
    writeSysFile(ledPath + "brightness", "0");
}

// ========================
// ALARM 控制
// ========================
void smartDeviceModule::turnAlarmOn()
{
    writeSysFile(alarmPath + "enable", "1");
}

void smartDeviceModule::turnAlarmOff()
{
    writeSysFile(alarmPath + "enable", "0");
}

// ========================
// BEEP 控制
// ========================
void smartDeviceModule::beepOn()
{
    writeSysFile(beepPath + "brightness", "1");
}

void smartDeviceModule::beepOff()
{
    writeSysFile(beepPath + "brightness", "0");
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
