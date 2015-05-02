#include "qmediaslider.h"
#include <QPainter>

QMediaSlider::QMediaSlider(QWidget *parent) :
    QSlider(parent)
{
    setOrientation(Qt::Horizontal);
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setStyleSheet("QSlider { height: 12px; }");
}

void QMediaSlider::clearTicks()
{
    ticks.clear();
}

void QMediaSlider::setTick(int value, QString text)
{
    ticks.insert(value, text);
}

void QMediaSlider::paintEvent(QPaintEvent *pe)
{
    (void)pe;

    QColor grooveBorder, grooveFill, handleBorder, handleFill;
    QPainter p(this);

    QPalette::ColorGroup cg = isEnabled() ? QPalette::Normal : QPalette::Disabled;
    grooveBorder = palette().color(cg,QPalette::Shadow);
    grooveFill = palette().color(cg,QPalette::Base);
    handleBorder = palette().color(cg,QPalette::Dark);
    handleFill = palette().color(cg,QPalette::Button);

    p.setPen(grooveBorder);
    p.setBrush(grooveFill);
    p.drawRect(drawnGroove);

    int pos;
    for(QMap<int, QString>::const_iterator i = ticks.constBegin();
        i != ticks.constEnd(); i++) {
        pos = valueToX(i.key());
        if (pos != drawnGroove.left() && pos != drawnGroove.right())
            p.drawLine(QPoint(pos,drawnGroove.top() + 1),
                       QPoint(pos, drawnGroove.bottom() - 1));
    }

    pos = valueToX(value());
    QRect slider(pos-5,0,10,height()-1);

    p.setBrush(Qt::NoBrush);
    p.setPen(handleBorder);
    p.drawRect(slider);

    p.setPen(handleFill);
    slider.adjust(1,1,-1,-1);
    p.drawRect(slider);

    p.setPen(handleBorder);
    slider.adjust(1,1,-1,-1);
    p.drawRect(slider);
}

QString QMediaSlider::valueToTickname(int value)
{
    QString last_text;
    int last_tick = -1;
    for (QMap<int, QString>::const_iterator i = ticks.constBegin();
         i != ticks.constEnd(); i++) {
        last_tick = i.key();
        if (last_tick > value)
            break;
        last_text = i.value();
    }
    return last_text;
}

void QMediaSlider::resizeEvent(QResizeEvent *re)
{
    (void)re;
    drawnGroove = QRect(0,0,width()-1, height()-1);
    drawnGroove.adjust(5,3,-5,-3);
}

void QMediaSlider::mousePressEvent(QMouseEvent *me)
{
    setValue(xToValue(me->localPos().x()));
    sliderMoved(value());
}

void QMediaSlider::mouseMoveEvent(QMouseEvent *me)
{
    int mouse_value = xToValue(me->localPos().x());
    if (me->buttons() & Qt::LeftButton) {
        setValue(mouse_value);
        sliderMoved(value());
    }
    hoverValue(mouse_value, valueToTickname(mouse_value), me->localPos().x());
}

void QMediaSlider::enterEvent(QEvent *event)
{
    (void)event;
    hoverBegin();
}

void QMediaSlider::leaveEvent(QEvent *event)
{
    (void)event;
    hoverEnd();
}

int QMediaSlider::valueToX(int value)
{
    return drawnGroove.left() + (((value-minimum()) * drawnGroove.width())
                                 / (maximum() - minimum()));
}

double QMediaSlider::xToValue(int x)
{
    double val = ((x-drawnGroove.left()) * (maximum() - minimum()))
            / drawnGroove.width();
    if (val < minimum()) return minimum();
    if (val > maximum()) return maximum();
    return val;
}
