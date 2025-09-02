#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QFile>
#include <QLabel>
#include <QToolButton>


#include "smartdevicemodule.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_ledBtn_clicked(bool checked);

    void on_alarmBtn_clicked(bool checked);

    void on_fanBtn_clicked(bool checked);

    void on_smartBtn_clicked(bool checked);

    void on_musicBtn_clicked(bool checked);

    void on_photoBtn_clicked(bool checked);

    void onAlarmStopped(); // 闹钟停止后的处理
private:
    Ui::MainWindow *ui;
    // ================= 初始化函数 =================
    void setButtonIcon(QToolButton *btn, const QString &iconPath);
    void initButtons();     ///< 初始化按钮属性（Checkable/默认状态）

    smartDeviceModule *deviceModule;
};
#endif // MAINWINDOW_H
