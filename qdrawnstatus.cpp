#include <QPainter>
#include "helpers.h"
#include "qdrawnstatus.h"

QStatusTime::QStatusTime(QWidget *parent) : QOpenGLWidget(parent),
    currentTime(-1)
{
    setTime(0);
    setMinimumSize(minimumSizeHint());
}

QSize QStatusTime::minimumSizeHint() const
{
    QSize sz = QFontMetrics(font()).size(0, drawnText);
    return sz;
}

void QStatusTime::setTime(double time)
{
    if (currentTime == time)
        return;
    currentTime = time;
    drawnText = Helpers::toDateFormat(time);
    update();
}

void QStatusTime::paintGL()
{
    QPainter p(this);
    QColor bgColor = parentWidget()->palette().color(QPalette::Active, QPalette::Window);
    QColor txColor = parentWidget()->palette().color(QPalette::Active, QPalette::WindowText);
    QRectF rc = QRectF(QPointF(0,0), QSizeF(size()));
    p.fillRect(rc, bgColor);
    p.setPen(txColor);
    p.drawText(rc, drawnText, QTextOption(Qt::AlignRight | Qt::AlignVCenter));
}

