#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    /* 获取屏幕的分辨率，Qt官方建议使用这
     * 种方法获取屏幕分辨率，防上多屏设备导致对应不上
     * 注意，这是获取整个桌面系统的分辨率
     */
    QList <QScreen *> list_screen =  QGuiApplication::screens();

    // 打印屏幕分辨率信息
    if (!list_screen.isEmpty()) {
        QRect screenRect = list_screen.at(0)->geometry();
        qDebug() << "Screen resolution:"
                 << screenRect.width() << "x" << screenRect.height();
    }

    /* 如果是ARM平台，直接设置大小为屏幕的大小 */
//#if __arm__
 #if 1
    /* 重设大小 */
    this->resize(list_screen.at(0)->geometry().width(),
                 list_screen.at(0)->geometry().height());
    /* 默认是出厂系统的LED心跳的触发方式,想要控制LED，
     * 需要改变LED的触发方式，改为none，即无 */
    system("echo none > /sys/class/leds/sys-led/trigger");
#else
    /* 否则则设置主窗体大小为1024*600 */
    this->resize(1024, 600);
#endif
    ui->setupUi(this);

    deviceModule = new smartDeviceModule();

    // // 闹钟停止后的处理
    connect(deviceModule, &smartDeviceModule::alarmStopped,
            this, &MainWindow::onAlarmStopped);
    /*btn init*/
    initButtons();
}

MainWindow::~MainWindow()
{

    delete ui;
}

void MainWindow::initButtons()
{
    // 设置 checkable 属性
    ui->smartBtn->setCheckable(true);
    ui->musicBtn->setCheckable(true);
    ui->photoBtn->setCheckable(true);
    ui->ledBtn->setCheckable(true);
    ui->fanBtn->setCheckable(true);
    ui->alarmBtn->setCheckable(true);

    ui->ledBtn->setAutoRaise(true);   // 去掉按钮边框，看起来像图标
    ui->fanBtn->setAutoRaise(true);   // 去掉按钮边框，看起来像图标
    ui->alarmBtn->setAutoRaise(true);   // 去掉按钮边框，看起来像图标

    // 默认未选中
    ui->smartBtn->setChecked(false);
    ui->musicBtn->setChecked(false);
    ui->photoBtn->setChecked(false);
    ui->ledBtn->setChecked(false);
    ui->fanBtn->setChecked(false);
    ui->alarmBtn->setChecked(false);

    //icon
    setButtonIcon(ui->ledBtn, ":/src/smart/light_off.png");
    setButtonIcon(ui->alarmBtn, ":/src/smart/alarm_off.png");
    setButtonIcon(ui->fanBtn, ":/src/smart/fan_off.png");
}




void MainWindow::on_ledBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->label_led->setText("LED_ON");
        deviceModule->turnLedOn();
        setButtonIcon(ui->ledBtn, ":/src/smart/light_on.png");
    }else {
        ui->label_led->setText("LED_OFF");
        deviceModule->turnLedOff();
        setButtonIcon(ui->ledBtn, ":/src/smart/light_off.png");
    }
}

void MainWindow::on_alarmBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->label_alarm->setText("ALARM_ON");
        deviceModule->startAlarm(5,500);        // 5 次滴滴，每次间隔 500ms
        setButtonIcon(ui->alarmBtn, ":/src/smart/alarm_on.png");
    }else {
        ui->label_alarm->setText("ALARM_OFF");
        deviceModule->stopAlarm();
        setButtonIcon(ui->alarmBtn, ":/src/smart/alarm_off.png");
    }
}

void MainWindow::on_fanBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->label_fan->setText("FAN_ON");
        deviceModule->beepOn();
        setButtonIcon(ui->fanBtn, ":/src/smart/fan_on.png");
    }else {
        ui->label_fan->setText("FAN_OFF");
        deviceModule->beepOff();
        setButtonIcon(ui->fanBtn, ":/src/smart/fan_off.png");
    }
}

void MainWindow::on_smartBtn_clicked(bool checked)
{

}

void MainWindow::on_musicBtn_clicked(bool checked)
{

}

void MainWindow::on_photoBtn_clicked(bool checked)
{

}

// 闹钟停止后的处理
void MainWindow::onAlarmStopped()
{
    ui->label_alarm->setText("ALARM_OFF");
    setButtonIcon(ui->alarmBtn, ":/src/smart/alarm_off.png");
}

// 切换按钮图标
void MainWindow::setButtonIcon(QToolButton *btn, const QString &iconPath)
{
    QIcon icon(iconPath);
    if (icon.isNull()) {
        qDebug() << "❌ 图标加载失败:" << iconPath;
    } else {
        qDebug() << "✅ 图标加载成功:" << iconPath;
    }
    btn->setIcon(icon);
    btn->setIconSize(QSize(200, 160));  // 保持和 QSS 一致的大小
}

