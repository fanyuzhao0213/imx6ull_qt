#ifndef SMARTDEVICEMODULE_H
#define SMARTDEVICEMODULE_H

#include <QObject>
#include <QTimer>
#include <QString>

/**
 * @brief The smartDeviceModule class
 * 封装系统硬件外设（LED、BEEP、ALARM）的控制
 * 继承 QObject 以便使用 QTimer 和信号槽
 */
class smartDeviceModule : public QObject
{
    Q_OBJECT

public:
    explicit smartDeviceModule(QObject *parent = nullptr);
    ~smartDeviceModule();

    // ========================
    // LED 控制
    // ========================
    void setLedTrigger(const QString &mode);  // 设置 LED 触发模式，如 "none"、"heartbeat"
    void turnLedOn();                          // 点亮 LED
    void turnLedOff();                         // 熄灭 LED

    // ========================
    // ALARM 控制
    // ========================
    void turnAlarmOn();
    void turnAlarmOff();

    // ========================
    // BEEP 控制（可用于闹钟）
    // ========================
    void beepOn();    // BEEP 点亮（鸣叫）
    void beepOff();   // BEEP 熄灭（停止）

    // 闹钟控制
    void startAlarm(int times, int intervalMs = 500); // times: 滴滴次数, intervalMs: 间隔毫秒
    void stopAlarm();                                 // 停止闹钟
signals:
    void alarmStopped();  // 闹钟完成时发送

private slots:
    void toggleBeep();  // 定时器槽函数，切换 BEEP 状态（开/关）

private:
    QString ledPath;    // LED sysfs 路径
    QString alarmPath;  // ALARM sysfs 路径
    QString beepPath;   // BEEP sysfs 路径

    QTimer timer;       // 用于闹钟 BEEP 间隔控制
    int beepCount;      // 当前滴滴次数计数
    int beepTarget;     // 目标滴滴次数
    bool beepState;     // 当前 BEEP 状态：true=响，false=停
    int interval;       // 间隔时间（毫秒）

    // ========================
    // 工具函数
    // ========================
    bool writeSysFile(const QString &path, const QString &value);  // 写入 sysfs 文件
};

#endif // SMARTDEVICEMODULE_H
