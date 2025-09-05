/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 1990-2021. All rights reserved.
* @projectName   SlidePage
* @brief         slidepage.cpp
* @editor        ChatGPT 完整注释增强版
* @date          2025-09-05
*******************************************************************/
#include "slidepage.h"
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QCursor>
#include <QDebug>

SlidePage::SlidePage(QWidget *parent)
    : QWidget(parent),
      pageIndex(0),
      pageCount(0),
      draggingFlag(false)
{
    /* 基础设置 */
    this->setMinimumSize(400, 300);                // 默认最小大小
    this->setAttribute(Qt::WA_TranslucentBackground, true); // 背景透明

    /* =================== 1. 滚动区域 =================== */
    scrollArea = new QScrollArea(this);
    scrollArea->setAlignment(Qt::AlignCenter);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);   // 隐藏垂直滚动条
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏水平滚动条

    /* 主容器，承载所有页面 */
    mainWidget = new QWidget();
    mainWidget->setStyleSheet("background: transparent");
    scrollArea->setWidget(mainWidget);
    scrollArea->setStyleSheet("background: transparent");

    /* =================== 2. 底部指示器 =================== */
    bottomWidget = new QWidget(this);
    bottomWidget->setStyleSheet("background: transparent");
    bottomHBoxLayout = new QHBoxLayout(bottomWidget);
    bottomHBoxLayout->setContentsMargins(0, 0, 0, 0);
    bottomHBoxLayout->setAlignment(Qt::AlignCenter);

    /* =================== 3. 页面布局 =================== */
    hBoxLayout = new QHBoxLayout();
    hBoxLayout->setContentsMargins(0, 0, 0, 0);
    hBoxLayout->setSpacing(0);
    mainWidget->setLayout(hBoxLayout);

    /* =================== 4. QScroller 滑动控制 =================== */
    scroller = QScroller::scroller(scrollArea);                // 绑定 scrollArea
    scroller->grabGesture(scrollArea, QScroller::LeftMouseButtonGesture); // 左键模拟触摸手势

    // 配置滑动属性
    QScrollerProperties properties = scroller->scrollerProperties();
    properties.setScrollMetric(QScrollerProperties::SnapTime, 0.5);        // 滑动时间，值越大，时间越短
    properties.setScrollMetric(QScrollerProperties::MinimumVelocity, 1);   // 最小滑动速度
    scroller->setScrollerProperties(properties);

    /* =================== 5. 拖动检测定时器 =================== */
    timer = new QTimer(this);

    /* =================== 6. 信号槽连接 =================== */
    connect(scrollArea->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &SlidePage::hScrollBarValueChanged);

    connect(scroller, &QScroller::stateChanged,
            this, &SlidePage::onStateChanged);

    connect(timer, &QTimer::timeout,
            this, &SlidePage::onTimerTimeout);

    connect(this, &SlidePage::currentPageIndexChanged,
            this, &SlidePage::onCurrentPageIndexChanged);
}

SlidePage::~SlidePage() {}

/**
 * @brief 添加一个新页面
 */
void SlidePage::addPage(QWidget *w)
{
    // 1. 添加页面到主布局
    hBoxLayout->addWidget(w);
    pageCount++;

    // 2. 创建底部指示器
    QLabel *label = new QLabel();

    // 【关键修复 ①】固定大小为16x16，防止拉伸
    label->setFixedSize(16, 16);

    // 【关键修复 ②】让图片自动适配 QLabel
    label->setScaledContents(true);

    // 【关键修复 ③】透明背景，避免白色或黑色矩形边框
    label->setStyleSheet("background: transparent;");

    // 加载默认状态的灰色圆点
    label->setPixmap(QPixmap(":/src/sliderpage_off.png"));  // 确认此PNG为透明背景圆形

    // 居中对齐
    label->setAlignment(Qt::AlignCenter);

    // 保存到容器，方便后续切换状态
    pageIndicator.append(label);

    // 加入布局
    bottomHBoxLayout->addWidget(label);
}


/**
 * @brief 窗口大小变化时自动调整内部布局
 */
void SlidePage::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    scrollArea->resize(this->size());
    mainWidget->resize(this->width() * pageCount, this->height() - 20);

    // 更新初始指示器
    if (pageCount > 0)
        onCurrentPageIndexChanged(0);

    // 调整底部指示器位置
    bottomWidget->setGeometry(0, this->height() - 20, this->width(), 20);
}

/**
 * @brief 滚动条变化时计算当前页索引
 */
void SlidePage::hScrollBarValueChanged(int)
{
    // 当前滚动位置对应的页索引
    pageIndex = scrollArea->horizontalScrollBar()->value() / this->width();

    // 超过一半宽度时切换到下一页
    pageIndex = scrollArea->horizontalScrollBar()->value() >=
                (pageIndex * this->width() + this->width() * 0.5)
                ? pageIndex + 1 : pageIndex;
}

/**
 * @brief QScroller 状态变化槽
 */
void SlidePage::onStateChanged(QScroller::State state)
{
    static int pressedValue = 0;       // 按下位置
    static int releasedValue = 0;      // 松开位置
    static int currentPageIndex = 0;   // 按下时的页索引

    if (pageCount == 0) return; // 无页面直接返回

    /* ============ 松开状态：计算最终页并动画切换 ============ */
    if (state == QScroller::Inactive) {
        timer->stop();  // 停止拖动检测
        releasedValue = QCursor::pos().x(); // 记录松开位置

        if (pressedValue == releasedValue)
            return; // 没有实际滑动

        // 如果不是拖动状态，则根据滑动方向切换页
        if (!draggingFlag) {
            if (pressedValue - releasedValue > 20 && currentPageIndex == pageIndex)
                pageIndex++;  // 向左滑动，下一页
            else
                pageIndex--;  // 向右滑动，上一页
        }

        // 页索引边界处理
        if (pageIndex < 0) pageIndex = 0;
        if (pageIndex >= pageCount) pageIndex = pageCount - 1;

        // 动画平滑滚动到目标页
        QPropertyAnimation *animation = new QPropertyAnimation(scrollArea->horizontalScrollBar(), "value");
        animation->setDuration(200); // 动画持续时间
        animation->setStartValue(scrollArea->horizontalScrollBar()->value());
        animation->setEasingCurve(QEasingCurve::OutCurve);
        animation->setEndValue(pageIndex * this->width());
        animation->start();

        // 如果页索引发生变化，发送信号
        if (currentPageIndex != pageIndex)
            emit currentPageIndexChanged(pageIndex);

        // 复位
        pressedValue = 0;
        releasedValue = 0;
        draggingFlag = false;
    }

    /* ============ 按下状态：记录位置并启动拖动检测 ============ */
    if (state == QScroller::Pressed) {
        pressedValue = QCursor::pos().x();
        currentPageIndex = scrollArea->horizontalScrollBar()->value() / this->width();

        // 300ms 内未松手 -> 判定为拖动
        timer->start(300);
    }
}

/**
 * @brief 定时器超时，标记为拖动状态
 */
void SlidePage::onTimerTimeout()
{
    draggingFlag = true;
    timer->stop();
}

/**
 * @brief 当前页索引变化时，更新底部指示器
 */
void SlidePage::onCurrentPageIndexChanged(int index)
{
    for (int i = 0; i < pageIndicator.count(); ++i) {
        // 当前页为高亮蓝色，其余灰色
        pageIndicator[i]->setPixmap(QPixmap(
            i == index ? ":/src/sliderpage_on.png" : ":/src/sliderpage_off.png"
        ));
    }
}

/**
 * @brief 获取总页数
 */
int SlidePage::getPageCount() const
{
    return pageCount;
}

/**
 * @brief 获取当前页索引
 */
int SlidePage::getCurrentPageIndex() const
{
    return pageIndex;
}
