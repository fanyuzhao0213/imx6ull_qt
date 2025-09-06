#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QFile>
#include <QLabel>
#include <QToolButton>
#include <QMovie>


#include "serialmodule.h"
#include "smartdevicemodule.h"
#include "musicmodule.h"
#include "widgets/arcgraph/arcgraph.h"
#include "widgets/glowtext/glowtext.h"
#include "slidepage/slidepage.h"            //滑动页面



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void mainwindow_init();
    void photopage_init();
    void musicpage_init();

    void loadPhotosToSlidePage();       // 加载图片到滑动页面

private slots:
    void on_ledBtn_clicked(bool checked);
    void on_alarmBtn_clicked(bool checked);
    void on_fanBtn_clicked(bool checked);
    void on_smartBtn_clicked(bool checked);
    void on_musicBtn_clicked(bool checked);
    void on_photoBtn_clicked(bool checked);
    void on_uartBtn_clicked(bool checked);

    void onAlarmStopped(); // 闹钟停止后的处理
    void onAp3216cDataChanged(const Ap3216cData &data);
    void on6AxisDataChanged(const Sensor6AxisData &data);
    void ap3216c_style_init();

    void onSerialDataReceived(const QByteArray &data);  // 串口数据接收槽
    void onSerialError(const QString &error);           // 串口错误槽
    void on_openBtn_clicked(bool checked);

    void on_clear_rev_Btn_clicked();

    void on_clear_send_Btn_clicked();

    void on_sendBtn_clicked();

    /**
        * @brief 将十六进制字符串转换为字节数组
    */
    QByteArray hexStringToByteArray(const QString &hexString);
    void on_Btn_1_clicked();
    void on_Btn_2_clicked();
    void on_Btn_6_clicked();
    void on_Btn_7_clicked();

    void on_toolButton_xiazai_clicked();

    void on_toolButton_mode_clicked();

    void on_toolButton_last_clicked();

    void on_toolButton_bofang_clicked();

    void on_toolButton_next_clicked();

    void on_toolButton_sound_clicked();

    void on_toolButton_xihuan_clicked();

    void on_toolButton_ci_clicked();


    void onSongInfoUpdated(const QString &title,
                           const QString &artist,
                           const QString &composer,
                           const QPixmap &cover);
    void onDurationChanged(qint64 duration);
    void onPositionChanged(qint64 position);

    void sliderPressed();
    void sliderReleased();
    void sliderMoved(int value);

private:
    Ui::MainWindow *ui;
    bool m_sliderPressed = false; //音乐播放进度条拖动标记
    // ================= 初始化函数 =================
    void setButtonIcon(QToolButton *btn, const QString &iconPath);
    void initButtons();     ///< 初始化按钮属性（Checkable/默认状态）
	void initSerial();     ///< 初始化串口界面
    void initPortList();  // 初始化串口列表

    QMovie *movie;          //gif效果

    // MainWindow 成员变量
    MusicPlayer *musicPlayer;

    smartDeviceModule *deviceModule;
    serialModule *g_serialModule;
};
#endif // MAINWINDOW_H
