#ifndef SMARTDEVICEMODULE_H
#define SMARTDEVICEMODULE_H

#include <QObject>
#include <QTimer>
#include <QString>

/*
传感器	典型范围	单位/意义
ALS	0 ~ 65535	环境光强，ADC值，可换算 Lux
PS	0 / 1 或 0~1023	接近状态，1表示物体靠近
IR	0 ~ 1023	红外光强度，辅助检测距离
*/
// ====== AP3216C sysfs 文件路径宏 ======
#define AP3216C_BASE_PATH     "/sys/class/misc/ap3216c/"
#define AP3216C_IR_PATH       AP3216C_BASE_PATH "ir"
#define AP3216C_PS_PATH       AP3216C_BASE_PATH "ps"
#define AP3216C_ALS_PATH      AP3216C_BASE_PATH "als"

// 设置各外设 sysfs 路径
#define	ledPath    "/sys/class/leds/sys-led/"
#define	alarmPath  "/sys/class/alarm/alarm0/" // 如果系统有 ALARM 外设
#define beepPath   "/sys/class/leds/beep/"

/* 传感器数据结构体 */
struct Ap3216cData {
    QString als;  // 光照强度
    QString ps;   // 距离
    QString ir;   // 红外
};

struct Sensor6AxisData {
    QString ax;
    QString ay;
    QString az;
    QString gx;
    QString gy;
    QString gz;
};

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

	/* ap3216c 启停采集 */
    void setCapture(bool start);
    QString readSensorValue(const QString &filePath);
    void timer_timeout();
signals:
    void alarmStopped();  									// 闹钟完成时发送
    void ap3216cDataUpdated(const Ap3216cData &data);	    /* 当传感器数据更新时发射 */

private slots:
    void toggleBeep();  // 定时器槽函数，切换 BEEP 状态（开/关）

private:

    QTimer timer;       // 用于闹钟 BEEP 间隔控制
    int beepCount;      // 当前滴滴次数计数
    int beepTarget;     // 目标滴滴次数
    bool beepState;     // 当前 BEEP 状态：true=响，false=停
    int interval;       // 间隔时间（毫秒）

    QTimer readAp3216c_timer;

    /* 模拟读取硬件数据 */
    QString readAlsData();
    QString readPsData();
    QString readIrData();
    // ========================
    // 工具函数
    // ========================
    bool writeSysFile(const QString &path, const QString &value);  // 写入 sysfs 文件
};

#endif // SMARTDEVICEMODULE_H
