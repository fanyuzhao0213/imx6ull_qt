#ifndef ARCGRAPH_H
#define ARCGRAPH_H

#include <QWidget>

class ArcGraph : public QWidget
{
    Q_OBJECT

public:
    explicit ArcGraph(QWidget *parent = nullptr);
    ~ArcGraph();

    // 设置起始角度（度数）
    void setStartAngle(int angle);

    // 设置弧长（度数）
    void setAngleLength(int length);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int startAngle;    // 起始角度
    int angleLength;   // 弧长
};

#endif // ARCGRAPH_H
