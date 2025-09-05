/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 1990-2021. All rights reserved.
* @projectName   SlidePage
* @brief         slidepage.h
* @author        Deng Zhimao
* @editor        ChatGPT 完整注释增强版
* @date          2025-09-05
*******************************************************************/
#ifndef SLIDEPAGE_H
#define SLIDEPAGE_H

#include <QWidget>
#include <QHBoxLayout>
#include <QScroller>
#include <QScrollArea>
#include <QTimer>
#include <QLabel>
#include <QVector>

/**
 * @brief SlidePage
 *
 * 这是一个支持 **左右滑动切换页面** 的自定义控件，
 * 类似于手机上的启动引导页或图片轮播控件。
 * 特性：
 *  1. 支持添加任意 QWidget 页面。
 *  2. 底部带小圆点分页指示器。
 *  3. 支持鼠标拖动/触摸滑动。
 *  4. 松开后自动滑动到最近页面，并带有动画效果。
 */
class SlidePage : public QWidget
{
    Q_OBJECT

public:
    explicit SlidePage(QWidget *parent = nullptr);  // 构造函数
    ~SlidePage();                                   // 析构函数

    /**
     * @brief 添加新的页面
     * @param page 需要添加的 QWidget 页面
     */
    void addPage(QWidget *page);

    /**
     * @brief 获取总页数
     */
    int getPageCount() const;

    /**
     * @brief 获取当前显示的页面索引
     */
    int getCurrentPageIndex() const;

signals:
    /**
     * @brief 当前显示页面发生变化时发出
     * @param index 当前页索引
     */
    void currentPageIndexChanged(int index);

protected:
    /**
     * @brief 重载 resizeEvent
     * 当控件大小变化时，调整内部 scrollArea 和 mainWidget 尺寸
     */
    void resizeEvent(QResizeEvent *event) override;

private slots:
    /**
     * @brief 滚动条值变化槽函数
     * 用于计算当前页索引
     */
    void hScrollBarValueChanged(int value);

    /**
     * @brief QScroller 状态变化槽函数
     * @param state QScroller 当前状态
     */
    void onStateChanged(QScroller::State state);

    /**
     * @brief 拖动检测定时器槽函数
     * 300ms 内未松手 -> 判定为拖动
     */
    void onTimerTimeout();

    /**
     * @brief 当前页面索引变化后，更新底部小圆点显示
     * @param index 当前页面索引
     */
    void onCurrentPageIndexChanged(int index);

private:
    /* ============= 核心界面结构 ============= */
    QScrollArea *scrollArea;       // 滚动容器，承载所有页面
    QWidget *mainWidget;           // 主容器，实际存放所有子页面
    QHBoxLayout *hBoxLayout;       // 水平布局，所有子页面横向排列

    /* ============= 底部指示器 ============= */
    QWidget *bottomWidget;              // 底部区域容器
    QHBoxLayout *bottomHBoxLayout;      // 底部水平布局
    QVector<QLabel *> pageIndicator;    // 底部指示器圆点

    /* ============= 滑动相关 ============= */
    QScroller *scroller;          // Qt 内置滑动控制器
    QTimer *timer;                // 拖动检测定时器（区分滑动和拖动）

    /* ============= 页面状态 ============= */
    int pageIndex;                // 当前页面索引
    int pageCount;                // 总页面数量
    bool draggingFlag;            // 拖动标志位，true 表示当前为拖动
};

#endif // SLIDEPAGE_H
