#ifndef QMEDIASLIDER_H
#define QMEDIASLIDER_H

#include <QSlider>
#include <QMouseEvent>

class QMediaSlider : public QSlider
{
    Q_OBJECT
public:
    explicit QMediaSlider(QWidget *parent = 0);

    void clearTicks();
    void setTick(int value, QString text);

signals:
    void hoverValue(int value, QString text, int local_x);
    void hoverBegin();
    void hoverEnd();

public slots:

private:
    void paintEvent(QPaintEvent *pe);
    QString valueToTickname(int value);

    void resizeEvent(QResizeEvent *re);
    void mousePressEvent(QMouseEvent* me);
    void mouseMoveEvent(QMouseEvent *me);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);

    int valueToX(int value);
    double xToValue(int x);

    QMap<int, QString> ticks;
    QRect drawnGroove;
};

#endif // QMEDIASLIDER_H
