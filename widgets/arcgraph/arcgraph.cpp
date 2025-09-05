#include "arcgraph.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QDebug>
#include <QTimer>

ArcGraph::ArcGraph(QWidget *parent)
    : QWidget(parent),
      startAngle(90),
      angleLength(100)
{
    this->setMinimumSize(100, 100);
    setAttribute(Qt::WA_TranslucentBackground, true);
    // 定时刷新，保证圆弧持续显示
    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, [this]() {
        update();
    });
    refreshTimer->start(50);  // 50ms刷新一次，大约20帧
}

ArcGraph::~ArcGraph()
{
}

void ArcGraph::setStartAngle(int angle)
{
    startAngle = angle;
    this->update();
}

void ArcGraph::setAngleLength(int length)
{
    angleLength = length;
    this->update();
}

void ArcGraph::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);

    // 控件中心点和半径
    QPointF center(width() / 2.0, height() / 2.0);
    qreal radius = qMin(width(), height()) / 2.0 - 2;  // 留一点边距

    // ---- 绘制最外层圆 ----
    QRadialGradient gradient1(center, radius, center);
    gradient1.setColorAt(0, Qt::transparent);
    gradient1.setColorAt(0.5, Qt::transparent);
    gradient1.setColorAt(0.51, QColor("#00237f"));
    gradient1.setColorAt(0.58, QColor("#00237f"));
    gradient1.setColorAt(0.59, Qt::transparent);
    gradient1.setColorAt(1, Qt::transparent);
    painter.setBrush(gradient1);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, radius, radius);

    // ---- 绘制里层圆 ----
    QRadialGradient gradient2(center, radius, center);
    gradient2.setColorAt(0, Qt::transparent);
    gradient2.setColorAt(0.420, Qt::transparent);
    gradient2.setColorAt(0.421, QColor("#885881e3"));
    gradient2.setColorAt(0.430, QColor("#5881e3"));
    gradient2.setColorAt(0.440, QColor("#885881e3"));
    gradient2.setColorAt(0.441, Qt::transparent);
    gradient2.setColorAt(1, Qt::transparent);
    painter.setBrush(gradient2);
    painter.drawEllipse(center, radius, radius);

    // ---- 显示百分比数字 ----
    QFont font;
    font.setPixelSize(width() / 10);
    painter.setFont(font);
    painter.setPen(Qt::white);
    double percentage = static_cast<double>(angleLength) * 100.0 / 360.0;
    painter.drawText(rect(), Qt::AlignCenter, QString::number(percentage, 'f', 1) + "%");

    // ---- 发光弧 ----
    painter.save();
    painter.translate(center);

    QRectF arcRect(-radius, -radius, radius * 2, radius * 2);

    // 背景发光弧
    QRadialGradient gradient3(0, 0, radius);
    gradient3.setColorAt(0, Qt::transparent);
    gradient3.setColorAt(0.42, Qt::transparent);
    gradient3.setColorAt(0.51, QColor("#500194d3"));
    gradient3.setColorAt(0.55, QColor("#22c1f3f9"));
    gradient3.setColorAt(0.58, QColor("#500194d3"));
    gradient3.setColorAt(0.68, Qt::transparent);
    gradient3.setColorAt(1.0, Qt::transparent);
    painter.setBrush(gradient3);
    painter.setPen(Qt::NoPen);

    QPainterPath path1;
    path1.arcMoveTo(arcRect, startAngle);       // 移动到弧起点，避免白线
    path1.arcTo(arcRect, startAngle, -angleLength);
    painter.drawPath(path1);

    // 发光圆/弧
    QRadialGradient gradient4(0, 0, radius);
    gradient4.setColorAt(0, Qt::transparent);
    gradient4.setColorAt(0.49, Qt::transparent);
    gradient4.setColorAt(0.50, QColor("#4bf3f9"));
    gradient4.setColorAt(0.59, QColor("#4bf3f9"));
    gradient4.setColorAt(0.60, Qt::transparent);
    gradient4.setColorAt(1.0, Qt::transparent);
    painter.setBrush(gradient4);

    QPainterPath path2;
    path2.arcMoveTo(arcRect, startAngle);       // 同样移动到弧起点
    path2.arcTo(arcRect, startAngle, -angleLength);
    painter.drawPath(path2);

    painter.restore();
}
