#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QComboBox>
#include <QStackedWidget>
#include <QMessageBox>
#include <QDateTime>
#include <QTextCursor>
#include <QTextDocument>
#include <QRandomGenerator>
#include <QMenu>
#include <QDir>
#include <QFileInfoList>




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

    mainwindow_init();          //主窗口初始化

    ui->stackedWidget->setCurrentWidget(ui->page_music);

    deviceModule = new smartDeviceModule();
    g_serialModule = new serialModule(this);

    initSerial();
    // 连接串口信号
    connect(g_serialModule, &serialModule::dataReceived,
            this, &MainWindow::onSerialDataReceived);

    connect(g_serialModule, &serialModule::errorOccurred,
            this, &MainWindow::onSerialError);

    // 初始化串口列表
    initPortList();

    //start ap3216c
    deviceModule->setCapture(false);

    // // 闹钟停止后的处理
    connect(deviceModule, &smartDeviceModule::alarmStopped,
            this, &MainWindow::onAlarmStopped);
    // 连接信号槽：当传感器数据更新时触发
    connect(deviceModule, &smartDeviceModule::ap3216cDataUpdated,
            this, &MainWindow::onAp3216cDataChanged);

    // ================== 模拟数据定时器 ==================
    QTimer *testTimer = new QTimer(this);

    connect(testTimer, &QTimer::timeout, this, [=]() {
        Ap3216cData fakeData;
        // 生成 0~65535 的 ALS 随机值
        fakeData.als = QString::number(QRandomGenerator::global()->bounded(65536));
        // 生成 0~1023 的 PS 随机值
        fakeData.ps = QString::number(QRandomGenerator::global()->bounded(1024));
        // 生成 0~1023 的 IR 随机值
        fakeData.ir = QString::number(QRandomGenerator::global()->bounded(1024));
        // 调用你原本的槽函数，模拟设备信号
        onAp3216cDataChanged(fakeData);

        // ===== 6轴传感器虚拟数据 =====
        Sensor6AxisData sensor6;
        sensor6.ax = QString::number(QRandomGenerator::global()->bounded(-2000, 2000) / 1000.0, 'f', 2); // -2~2 g
        sensor6.ay = QString::number(QRandomGenerator::global()->bounded(-2000, 2000) / 1000.0, 'f', 2);
        sensor6.az = QString::number(QRandomGenerator::global()->bounded(-2000, 2000) / 1000.0, 'f', 2);

        sensor6.gx = QString::number(QRandomGenerator::global()->bounded(-250, 250), 'f', 1); // -250~250 °/s
        sensor6.gy = QString::number(QRandomGenerator::global()->bounded(-250, 250), 'f', 1);
        sensor6.gz = QString::number(QRandomGenerator::global()->bounded(-250, 250), 'f', 1);

        on6AxisDataChanged(sensor6); // 更新 6 轴 UI
    });

    testTimer->start(1000); // 每 1 秒生成一次虚拟数据
    /*btn init*/
    initButtons();
    ap3216c_style_init();

    // 加载 GIF 动画
    QMovie *movie = new QMovie(":/src/gif/1.gif"); // GIF 文件路径
    movie->setScaledSize(QSize(1180, 765));                 // 缩放 GIF
    movie->setSpeed(100);                                 // 播放速度 100%
    ui->label_idle_gif->setMovie(movie);
    movie->start();                                       // 循环播放

    //photo page加载
    photopage_init();
    //music page加载
    musicpage_init();


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadPhotosToSlidePage()
{
    // 2️⃣ 手动生成资源文件列表（支持 1.png, 2.png ...）
    QStringList fileList = {
        ":/src/picture/1.png",
        ":/src/picture/2.png",
        ":/src/picture/3.png",
        ":/src/picture/4.png",
        ":/src/picture/5.png",
        ":/src/picture/6.png",
        ":/src/picture/7.png",
        ":/src/picture/8.png",
        ":/src/picture/exit.png",
        ":/src/picture/last.png",
        ":/src/picture/next.png"
    };

    if (fileList.isEmpty()) {
        qDebug() << "未找到任何照片";
        return;
    }

    qDebug() << "检测到照片数量:" << fileList.count();

    ui->label_photo->setText(
        QString("这是一个相册,检测到照片数量: %1 张, 滑动切换照片!").arg(fileList.count())
    );

    // 3️⃣ 创建 SlidePage
    SlidePage *slidePage = new SlidePage(ui->widget_photo);
    slidePage->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 4️⃣ 遍历资源，每页显示 1 张
    for (const QString &path : fileList) {
        QWidget *page = new QWidget();
        QVBoxLayout *vLayout = new QVBoxLayout(page);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(0);
        vLayout->setAlignment(Qt::AlignCenter);

        QLabel *photoLabel = new QLabel();
        photoLabel->setAlignment(Qt::AlignCenter);
        photoLabel->setStyleSheet("background-color: lightgray; border-radius: 0px;");
        photoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        // 加载图片并自适应 slidePage 的大小
        QPixmap pix(path);
        if (!pix.isNull()) {
            photoLabel->setPixmap(pix.scaled(ui->widget_photo->size(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation));
        } else {
            photoLabel->setText("加载失败");
        }

        vLayout->addWidget(photoLabel);
        slidePage->addPage(page);
    }

    // 5️⃣ 监听当前页变化
    connect(slidePage, &SlidePage::currentPageIndexChanged, this, [=](int index){
        qDebug() << "当前滑动到第" << index + 1 << "页";
    });

    // 6️⃣ 添加到 widget_photo 布局
    if (!ui->widget_photo->layout())
        ui->widget_photo->setLayout(new QVBoxLayout());
    ui->widget_photo->layout()->setContentsMargins(0, 0, 0, 0);
    ui->widget_photo->layout()->addWidget(slidePage);
}



void MainWindow::mainwindow_init()
{
    this->setWindowTitle("FYZ-IMX6ULL-智能设备");                   // 设置窗口标题
    this->setWindowIcon(QIcon(":/src/tittle.png"));                 // 设置窗口图标

    /*menubar设置*/
    // 1. 创建菜单
    this->menuBar()->setFont(QFont("Microsoft YaHei", 8, QFont::Bold));

    QMenu *menu1 = new QMenu("菜单1", this);     // 设置标题
    menu1->setIcon(QIcon(":/src/mario.png"));   // 设置图标
    QMenu *menu2 = new QMenu("菜单2", this);
    menu2->setIcon(QIcon(":/src/mushroom_life.png"));
    QMenu *menu3 = new QMenu("菜单3", this);
    menu3->setIcon(QIcon(":/src/qingwa.png"));

    // 3. 添加菜单到菜单栏
    this->menuBar()->addMenu(menu1);
    this->menuBar()->addMenu(menu2);
    this->menuBar()->addMenu(menu3);

    // 4. 可选：给每个菜单添加动作
    QAction *action1 = new QAction(QIcon(":/images/icon1.png"), "操作1", this);
    QAction *action2 = new QAction(QIcon(":/images/icon2.png"), "操作2", this);
    QAction *action3 = new QAction(QIcon(":/images/icon3.png"), "操作3", this);
    menu1->addAction(action1);
    menu2->addAction(action2);
    menu3->addAction(action3);

    // 5. 可选：连接槽函数
    connect(action1, &QAction::triggered, this, [](){ qDebug() << "操作1触发"; });
    connect(action2, &QAction::triggered, this, [](){ qDebug() << "操作2触发"; });
    connect(action3, &QAction::triggered, this, [](){ qDebug() << "操作3触发"; });
}

void MainWindow::photopage_init()
{
    loadPhotosToSlidePage();       // 加载图片到滑动页面

    QPixmap pix(":/src/picture/exit.png");
    ui->toolButton_photo->setIcon(QIcon(pix));
    ui->toolButton_photo->setIconSize(ui->toolButton_photo->size());
    ui->toolButton_photo->setStyleSheet("border: none; border-radius: 10px;");
}

void MainWindow::musicpage_init()
{
    ui->toolButton_select->setText("Select Songs");
    ui->toolButton_search->setText("Search");
    ui->toolButton_more->setText("More");
    ui->toolButton_hudong->setText("Interation");

    // -------- pushButton_name --------
    ui->toolButton_name->setIcon(QIcon(":/src/music/name.png"));
    ui->toolButton_name->setIconSize(QSize(150, 125)); // 图标大小
    ui->toolButton_name->setText("Name of song");
    ui->toolButton_name->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // 文字在下


    // -------- toolButton_type --------
    ui->toolButton_type->setIcon(QIcon(":/src/music/type.png"));
    ui->toolButton_type->setIconSize(QSize(150, 125));
    ui->toolButton_type->setText("Type");
    ui->toolButton_type->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // 文字在下

    // -------- toolButton_toplist --------
    ui->toolButton_toplist->setIcon(QIcon(":/src/music/top.png"));
    ui->toolButton_toplist->setIconSize(QSize(150, 125));
    ui->toolButton_toplist->setText("TOP list");
    ui->toolButton_toplist->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // 文字在下
    // -------- toolButton_new --------
    ui->toolButton_new->setIcon(QIcon(":/src/music/new.png"));
    ui->toolButton_new->setIconSize(QSize(150, 125));
    ui->toolButton_new->setText("NEW");
    ui->toolButton_new->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // 文字在下

    // -------- toolButton_stars --------
    ui->toolButton_stars->setIcon(QIcon(":/src/music/stars.png"));
    ui->toolButton_stars->setIconSize(QSize(150, 125));
    ui->toolButton_stars->setText("Stars");
    ui->toolButton_stars->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); // 文字在下

    //toolButton_exit
    ui->toolButton_exit->setIcon(QIcon(":/src/music/guanbi.png"));
    ui->toolButton_exit->setIconSize(QSize(40, 40)); // 图标大小

    //toolButton_bofang
    ui->toolButton_bofang->setIcon(QIcon(":/src/music/bofang.png"));
    ui->toolButton_bofang->setIconSize(QSize(40, 40)); // 图标大小

    //toolButton_sound
    ui->toolButton_sound->setIcon(QIcon(":/src/music/sound.png"));
    ui->toolButton_sound->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_ci,
    ui->toolButton_ci->setIcon(QIcon(":/src/music/geci.png"));
    ui->toolButton_ci->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_last,
    ui->toolButton_last->setIcon(QIcon(":/src/music/shangyiqu.png"));
    ui->toolButton_last->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_mode,
    ui->toolButton_mode->setIcon(QIcon(":/src/music/shunxu.png"));
    ui->toolButton_mode->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_next,
    ui->toolButton_next->setIcon(QIcon(":/src/music/xiayiqu.png"));
    ui->toolButton_next->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_xiazai,
    ui->toolButton_xiazai->setIcon(QIcon(":/src/music/xiazai.png"));
    ui->toolButton_xiazai->setIconSize(QSize(40, 40)); // 图标大小

    //#toolButton_xihuan,
    ui->toolButton_xihuan->setIcon(QIcon(":/src/music/xihuan.png"));
    ui->toolButton_xihuan->setIconSize(QSize(40, 40)); // 图标大小

    //初始播放时间显示为空
    ui->label_silder->setText("");
    // 创建播放器对象
    musicPlayer = new MusicPlayer(this);
    // 从配置文件加载播放列表（磁盘 + 资源）
    musicPlayer->loadFromConfig(":/src/music/music.ini");
    musicPlayer->setVolume(50);
    musicPlayer->play();
    ui->toolButton_bofang->setIcon(QIcon(":/src/music/zanting.png"));

    // ---------------- 信号槽连接 ----------------
    connect(musicPlayer, &MusicPlayer::durationChanged, this, &MainWindow::onDurationChanged);
    connect(musicPlayer, &MusicPlayer::positionChanged, this, &MainWindow::onPositionChanged);
    connect(musicPlayer, &MusicPlayer::songInfoUpdated, this, &MainWindow::onSongInfoUpdated);

    connect(ui->horizontalSlider_music, &QSlider::sliderPressed, this, &MainWindow::sliderPressed);
    connect(ui->horizontalSlider_music, &QSlider::sliderReleased, this, &MainWindow::sliderReleased);
    connect(ui->horizontalSlider_music, &QSlider::sliderMoved, this, &MainWindow::sliderMoved);
}


void MainWindow::initButtons()
{
    // 设置 checkable 属性
    ui->smartBtn->setCheckable(true);
    ui->musicBtn->setCheckable(true);
    ui->photoBtn->setCheckable(true);
    ui->uartBtn->setCheckable(true);
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

    /*uart*/
    ui->openBtn->setCheckable(true);
}

void MainWindow::initSerial() ///< 初始化串口界面
{

    ui->comboBox_databit->setCurrentIndex(3);
    ui->comboBox_parity->setCurrentIndex(0);
    ui->comboBox_baund->setCurrentIndex(3);
    ui->comboBox_stop->setCurrentIndex(0);

    connect(ui->lineEdit_uart_send, &QLineEdit::selectionChanged, this, [=]() {
        qDebug() << "lineEdit clicked!";
        QGuiApplication::inputMethod()->show();  // 手动显示键盘
    });

    ui->lineEdit_uart_send->setAttribute(Qt::WA_InputMethodEnabled, true);
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
    if(checked)
    {
        ui->smartBtn->setChecked(true);
        ui->musicBtn->setChecked(false);
        ui->photoBtn->setChecked(false);
        ui->uartBtn->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->smart_page);
    }else
    {
        ui->smartBtn->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_idle);
    }
}

void MainWindow::on_musicBtn_clicked(bool checked)
{
    if(checked)
    {

    }else
    {

    }
}

void MainWindow::on_photoBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->smartBtn->setChecked(false);
        ui->musicBtn->setChecked(false);
        ui->photoBtn->setChecked(true);
        ui->uartBtn->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_photo);
    }else
    {
        ui->photoBtn->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_idle);
    }
}


void MainWindow::on_uartBtn_clicked(bool checked)
{
    if(checked)
    {
        ui->smartBtn->setChecked(false);
        ui->musicBtn->setChecked(false);
        ui->photoBtn->setChecked(false);
        ui->uartBtn->setChecked(true);
        ui->stackedWidget->setCurrentWidget(ui->page_serial);
    }else
    {
        ui->uartBtn->setChecked(false);
        ui->stackedWidget->setCurrentWidget(ui->page_idle);
    }
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

void MainWindow::onAp3216cDataChanged(const Ap3216cData &data)
{
    bool ok;

    // ================== ALS (环境光传感器) ==================
    uint alsValue = data.als.toUInt(&ok);
    if(!ok) alsValue = 0;
    double alsPercent = static_cast<double>(alsValue) * 100.0 / 65535.0;

    ui->lcdNumber_als->display(data.als);
    ui->label_als->setText(QString("环境光强度: %1 lux\n百分比: %2%")
                           .arg(alsValue)
                           .arg(QString::number(alsPercent, 'f', 1)));

    // ================== PS (接近传感器) ==================
    uint psValue = data.ps.toUInt(&ok);
    if(!ok) psValue = 0;
    double psPercent = static_cast<double>(psValue) * 100.0 / 1023.0;

    ui->lcdNumber_ps->display(data.ps);
    ui->label_ps->setText(QString("接近传感器值: %1\n百分比: %2%")
                          .arg(psValue)
                          .arg(QString::number(psPercent, 'f', 1)));

    // ================== IR (红外传感器) ==================
    uint irValue = data.ir.toUInt(&ok);
    if(!ok) irValue = 0;
    double irPercent = static_cast<double>(irValue) * 100.0 / 1023.0;

    ui->lcdNumber_ir->display(data.ir);
    ui->label_ir->setText(QString("红外传感器值: %1\n百分比: %2%")
                          .arg(irValue)
                          .arg(QString::number(irPercent, 'f', 1)));
}

void MainWindow::on6AxisDataChanged(const Sensor6AxisData &data)
{
    bool ok;

    // ================== 加速度 (a/g) ==================
    double ax = data.ax.toDouble(&ok); if(!ok) ax = 0;
    double ay = data.ay.toDouble(&ok); if(!ok) ay = 0;
    double az = data.az.toDouble(&ok); if(!ok) az = 0;

    // 百分比表示 (-2g~+2g -> 0~100%)
    double axPercent = (ax + 2.0) * 100.0 / 4.0;
    double ayPercent = (ay + 2.0) * 100.0 / 4.0;
    double azPercent = (az + 2.0) * 100.0 / 4.0;

    // 更新 LCD 显示
    ui->lcdNumber_ax->display(ax);
    ui->lcdNumber_ay->display(ay);
    ui->lcdNumber_az->display(az);

    // 更新 label 显示
    ui->label_ax->setText(QString("AX: %1 g\n百分比: %2%")
                          .arg(ax, 0, 'f', 2)
                          .arg(QString::number(axPercent, 'f', 1)));
    ui->label_ay->setText(QString("AY: %1 g\n百分比: %2%")
                          .arg(ay, 0, 'f', 2)
                          .arg(QString::number(ayPercent, 'f', 1)));
    ui->label_az->setText(QString("AZ: %1 g\n百分比: %2%")
                          .arg(az, 0, 'f', 2)
                          .arg(QString::number(azPercent, 'f', 1)));

    // ================== 陀螺仪 (°/s) ==================
    double gx = data.gx.toDouble(&ok); if(!ok) gx = 0;
    double gy = data.gy.toDouble(&ok); if(!ok) gy = 0;
    double gz = data.gz.toDouble(&ok); if(!ok) gz = 0;

    // 百分比表示 (-250~+250 °/s -> 0~100%)
    double gxPercent = (gx + 250.0) * 100.0 / 500.0;
    double gyPercent = (gy + 250.0) * 100.0 / 500.0;
    double gzPercent = (gz + 250.0) * 100.0 / 500.0;

    // 更新 LCD 显示
    ui->lcdNumber_gx->display(gx);
    ui->lcdNumber_gy->display(gy);
    ui->lcdNumber_gz->display(gz);

    // 更新 label 显示
    ui->label_gx->setText(QString("GX: %1 °/s\n百分比: %2%")
                          .arg(gx, 0, 'f', 1)
                          .arg(QString::number(gxPercent, 'f', 1)));
    ui->label_gy->setText(QString("GY: %1 °/s\n百分比: %2%")
                          .arg(gy, 0, 'f', 1)
                          .arg(QString::number(gyPercent, 'f', 1)));
    ui->label_gz->setText(QString("GZ: %1 °/s\n百分比: %2%")
                          .arg(gz, 0, 'f', 1)
                          .arg(QString::number(gzPercent, 'f', 1)));
}


void MainWindow::ap3216c_style_init()
{

}


/*初始化串口*/
/**
 * @brief 初始化串口下拉框
 */
void MainWindow::initPortList()
{
    ui->comboBox_port->clear();  // 先清空

    // 获取可用串口列表
    QStringList ports = g_serialModule->availablePorts();
    if (ports.isEmpty()) {
        ui->comboBox_port->addItem("无可用串口");
        return;
    }

    // 添加到下拉框
    for (const QString &portName : ports) {
        ui->comboBox_port->addItem(portName);
    }
}

/**
 * @brief 串口接收数据槽
 */
/**
 * @brief 串口接收数据槽
 */
void MainWindow::onSerialDataReceived(const QByteArray &data)
{
    // 获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    QString displayText;

    if (!ui->checkBox_rev->isChecked()) {
        // 复选框选中时，显示ASCII格式
        QString asciiData;
        for (char c : data) {
            if (c >= 32 && c <= 126) { // 可打印ASCII字符
                asciiData.append(c);
            } else {
                asciiData.append('.');
            }
        }
        displayText = QString("[%1] ASCII: %2").arg(timestamp).arg(asciiData);
    } else {
        // 复选框未选中时，显示十六进制格式
        QString hexData = data.toHex(' ').toUpper();
        displayText = QString("[%1] HEX: %2").arg(timestamp).arg(hexData);
    }

    // 追加到文本浏览器
    ui->textBrowser_uart_rev->append(displayText);

    // 自动滚动到底部
    QTextCursor cursor = ui->textBrowser_uart_rev->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textBrowser_uart_rev->setTextCursor(cursor);
}

/**
 * @brief 串口错误处理槽
 */
void MainWindow::onSerialError(const QString &error)
{
    qDebug() << "串口错误:" << error;
}

void MainWindow::on_openBtn_clicked(bool checked)
{
    Q_UNUSED(checked); // 告诉编译器这个参数不用

    QString portName = ui->comboBox_port->currentText();
    int baudRate = ui->comboBox_baund->currentText().toInt();

    if (portName.isEmpty() || portName == "无可用串口") {
        QMessageBox::warning(this, "错误", "请选择有效串口！");
        return;
    }

    // 打开串口
    if (g_serialModule->openSerial(portName, baudRate)) {
        ui->openBtn->setText("关闭串口");
        QMessageBox::information(this, "提示", QString("串口 [%1] 打开成功！").arg(portName));
    } else {
        QMessageBox::critical(this, "错误", QString("串口 [%1] 打开失败！").arg(portName));
    }
}

void MainWindow::on_clear_rev_Btn_clicked()
{
    ui->textBrowser_uart_rev->clear();
}

void MainWindow::on_clear_send_Btn_clicked()
{
    ui->textBrowser_send->clear();
}

void MainWindow::on_sendBtn_clicked()
{
    QString displayText; // 用于 QTextBrowser 显示

    // 获取要发送的文本
    QString textToSend = ui->lineEdit_uart_send->text().trimmed();
    if (textToSend.isEmpty()) {
        QMessageBox::warning(this, "提示", "发送内容不能为空!");
        return;
    }

    // 检查串口是否打开
    if (!g_serialModule->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开!");
        return;
    }

    try {
        if (ui->checkBox_send->isChecked()) {
            // HEX模式发送
            QByteArray hexData = hexStringToByteArray(textToSend);
            if (hexData.isEmpty()) {
                QMessageBox::warning(this, "错误", "HEX格式错误! 请输入有效的十六进制数据");
                return;
            }
            g_serialModule->sendData(hexData);

            // 显示十六进制格式
            displayText = hexData.toHex(' ').toUpper(); // 空格分隔，每个字节两位
        } else {
            // ASCII模式发送
            g_serialModule->sendData(textToSend);

            displayText = textToSend; // 直接显示文本
        }

        // 在 QTextBrowser 中追加显示
        ui->textBrowser_send->append(QString("[%1] %2")
                                     .arg(QTime::currentTime().toString("HH:mm:ss"))
                                     .arg(displayText));

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "错误", QString("发送数据失败: %1").arg(e.what()));
    }
}


/**
 * @brief 将十六进制字符串转换为字节数组
 */
QByteArray MainWindow::hexStringToByteArray(const QString &hexString)
{
    QByteArray byteArray;
    QString cleanString = hexString.trimmed().remove(' ').remove('\n').remove('\r').remove('\t');

    // 检查长度是否为偶数
    if (cleanString.length() % 2 != 0) {
        return QByteArray();
    }

    bool ok;
    for (int i = 0; i < cleanString.length(); i += 2) {
        QString byteString = cleanString.mid(i, 2);
        uchar byteValue = byteString.toUShort(&ok, 16);
        if (!ok) {
            return QByteArray();
        }
        byteArray.append(byteValue);
    }

    return byteArray;
}


void MainWindow::on_Btn_1_clicked() {
    // 判断串口是否打开
    if (!g_serialModule->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开，无法发送数据！");
        return;
    }

    QByteArray payload("\x01\x00", 2);           // payload 固定为 01 00
    QByteArray data = g_serialModule->packCommand(0x0101, payload);
    g_serialModule->sendData(data);

    // 获取当前日期和时间
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    ui->textBrowser_send->append(QString("[%1] %2")
                                 .arg(dateTimeStr)
                                 .arg(QString(data.toHex(' ').toUpper())));

}

void MainWindow::on_Btn_2_clicked() {
    // 判断串口是否打开
    if (!g_serialModule->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开，无法发送数据！");
        return;
    }

    QByteArray payload("\x01\x00", 2);
    QByteArray data = g_serialModule->packCommand(0x0102, payload);
    g_serialModule->sendData(data);
    // 获取当前日期和时间
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    ui->textBrowser_send->append(QString("[%1] %2")
                                 .arg(dateTimeStr)
                                 .arg(QString(data.toHex(' ').toUpper())));

}

void MainWindow::on_Btn_6_clicked() {

    // 判断串口是否打开
    if (!g_serialModule->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开，无法发送数据！");
        return;
    }

    QByteArray payload("\x01\x00", 2);
    QByteArray data = g_serialModule->packCommand(0x0601, payload);
    g_serialModule->sendData(data);
    // 获取当前日期和时间
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    ui->textBrowser_send->append(QString("[%1] %2")
                                 .arg(dateTimeStr)
                                 .arg(QString(data.toHex(' ').toUpper())));

}

void MainWindow::on_Btn_7_clicked() {
    // 判断串口是否打开
    if (!g_serialModule->isOpen()) {
        QMessageBox::warning(this, "错误", "串口未打开，无法发送数据！");
        return;
    }

    QByteArray payload("\x01\x00", 2);
    QByteArray data = g_serialModule->packCommand(0x0602, payload);
    g_serialModule->sendData(data);
    // 获取当前日期和时间
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    ui->textBrowser_send->append(QString("[%1] %2")
                                 .arg(dateTimeStr)
                                 .arg(QString(data.toHex(' ').toUpper())));

}



/*Music 界面相关按钮操作*/
// ------------------- 下载按钮 -------------------
void MainWindow::on_toolButton_xiazai_clicked()
{
    qDebug() << "[UI] 点击下载按钮";
    // TODO: 弹出下载对话框或执行下载逻辑
}

// ------------------- 播放模式切换 -------------------
void MainWindow::on_toolButton_mode_clicked()
{
    static uint8_t mode = 0;
    mode = (mode + 1) % 3;
    qDebug() << "mode: " << mode <<endl;
    // 0 = 顺序播放
    // 1 = 单曲循环
    // 2 = 随机播放
    switch (mode) {
    case 0:
        ui->toolButton_mode->setIcon(QIcon(":/src/music/shunxu.png"));
        break;
    case 1:
        ui->toolButton_mode->setIcon(QIcon(":/src/music/danqu.png"));
        break;
    case 2:
        ui->toolButton_mode->setIcon(QIcon(":/src/music/suiji.png"));
        break;
    default:
        ui->toolButton_mode->setIcon(QIcon(":/src/music/shunxu.png"));
        break;
    }

    if (musicPlayer) {
        musicPlayer->togglePlaybackMode();  // 交给 MusicPlayer 处理
    }
}

// ------------------- 上一首 -------------------
void MainWindow::on_toolButton_last_clicked()
{
    qDebug() << "[UI] 点击上一首按钮";
    musicPlayer->previous();
}

// ------------------- 播放/暂停 -------------------
void MainWindow::on_toolButton_bofang_clicked()
{
    static bool checked;
    if(checked)
    {
        checked = 0;
        ui->toolButton_bofang->setIcon(QIcon(":/src/music/zanting.png"));
    }else{
        checked = 1;
        ui->toolButton_bofang->setIcon(QIcon(":/src/music/bofang.png"));
    }

    if (musicPlayer) {
        musicPlayer->togglePlay();  // 交给 MusicPlayer 处理
    }
}

// ------------------- 下一首 -------------------
void MainWindow::on_toolButton_next_clicked()
{
    qDebug() << "[UI] 点击下一首按钮";
    musicPlayer->next();
}

// ------------------- 音量按钮 -------------------
void MainWindow::on_toolButton_sound_clicked()
{
    if (!musicPlayer) return;

    static int previousVolume = 50;
    int currentVolume = musicPlayer->getVolume();

    if (currentVolume > 0) {
        previousVolume = currentVolume;
        musicPlayer->setVolume(0);  // 静音
        ui->toolButton_sound->setIcon(QIcon(":/src/music/jingyin.png"));
        qDebug() << "[UI] 静音，保存音量:" << previousVolume;
    } else {
        ui->toolButton_sound->setIcon(QIcon(":/src/music/sound.png"));
        musicPlayer->setVolume(previousVolume);  // 恢复音量
        qDebug() << "[UI] 恢复音量:" << previousVolume;
    }
}

// ------------------- 喜欢按钮 -------------------
void MainWindow::on_toolButton_xihuan_clicked()
{
    qDebug() << "[UI] 点击喜欢按钮";
    // TODO: 添加当前播放歌曲到收藏列表
}

// ------------------- 歌词显示按钮 -------------------
void MainWindow::on_toolButton_ci_clicked()
{
    qDebug() << "[UI] 点击歌词按钮";
    // TODO: 显示歌词面板或切换歌词显示状态
}

// -------------------- music槽函数实现 --------------------
// durationChanged 信号槽：总时长变化
void MainWindow::onDurationChanged(qint64 duration)
{
    // 设置Slider最大值为总时长（毫秒）
    ui->horizontalSlider_music->setMaximum(static_cast<int>(duration));

    // 初始化显示当前时间 / 总时间
    QTime totalTime(0, 0, 0);
    totalTime = totalTime.addMSecs(duration);

    QTime currentTime(0, 0, 0);
    currentTime = currentTime.addMSecs(0); // 当前时间为0

    // 在一个label上显示：当前时间 / 总时间
    ui->label_silder ->setText(QString("%1 / %2")
        .arg(currentTime.toString("mm:ss"))
        .arg(totalTime.toString("mm:ss")));
}

// positionChanged 信号槽：播放位置变化
void MainWindow::onPositionChanged(qint64 position)
{
    // 如果用户没有拖动Slider，则更新Slider的值
    if (!m_sliderPressed)
        ui->horizontalSlider_music->setValue(static_cast<int>(position));

    // 获取总时长
    int totalDuration = ui->horizontalSlider_music->maximum();

    QTime currentTime(0, 0, 0);
    currentTime = currentTime.addMSecs(position);

    QTime totalTime(0, 0, 0);
    totalTime = totalTime.addMSecs(totalDuration);

    // 更新label显示：当前时间 / 总时间
    ui->label_silder->setText(QString("%1 / %2")
        .arg(currentTime.toString("mm:ss"))
        .arg(totalTime.toString("mm:ss")));
}

// Slider 拖动按下
void MainWindow::sliderPressed()
{
    m_sliderPressed = true; // 标记正在拖动
}

// Slider 拖动释放
void MainWindow::sliderReleased()
{
    m_sliderPressed = false; // 标记结束拖动
    // 更新播放器位置
    musicPlayer->seek(ui->horizontalSlider_music->value());
}


// Slider 拖动中
void MainWindow::sliderMoved(int value)
{
    // 拖动时动态更新label显示
    QTime currentTime(0, 0, 0);
    currentTime = currentTime.addMSecs(value);

    int totalDuration = ui->horizontalSlider_music->maximum();
    QTime totalTime(0, 0, 0);
    totalTime = totalTime.addMSecs(totalDuration);

    ui->label_silder->setText(QString("%1 / %2")
        .arg(currentTime.toString("mm:ss"))
        .arg(totalTime.toString("mm:ss")));
}
void MainWindow::onSongInfoUpdated(const QString &title,
                                   const QString &artist,
                                   const QString &composer,
                                   const QPixmap &cover)
{
    // 歌手和歌名为空时显示“无”
    QString artistText = artist.isEmpty() ? "<span style='color:#00ffff;'>演唱：</span><span style='color:#ffffff;'>无</span>"
                                          : QString("<span style='color:#00ffff;'>演唱：</span><span style='color:#ffffff;'>%1</span>").arg(artist);
    QString titleText  = title.isEmpty() ? "<span style='color:#00ffff;'>歌名：</span><span style='color:#ffffff;'>无</span>"
                                         : QString("<span style='color:#00ffff;'>歌名：</span><span style='color:#ffffff;'>%1</span>").arg(title);

    ui->label_author->setText(artistText);
    ui->label_name->setText(titleText);

    if (!cover.isNull())
        ui->toolButton_name->setIcon(QIcon(cover));
}
