#include <QPainter>
#include <QTimer>
#include "helpers.h"
#include "drawnstatus.h"

StatusTime::StatusTime(QWidget *parent) : QWidget(parent),
    currentTime(-1)
{
    setTime(0);
    setMinimumSize(minimumSizeHint());
}

QSize StatusTime::minimumSizeHint() const
{
    QSize sz = QFontMetrics(font()).size(0, drawnText);
    return sz;
}

void StatusTime::setTime(double time)
{
    if (currentTime == time)
        return;
    currentTime = time;
    drawnText = Helpers::toDateFormat(time);
    update();
}

void StatusTime::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    QColor bgColor = parentWidget()->palette().color(QPalette::Active, QPalette::Window);
    QColor txColor = parentWidget()->palette().color(QPalette::Active, QPalette::WindowText);
    QRectF rc = QRectF(QPointF(0,0), QSizeF(size()));
    p.fillRect(rc, bgColor);
    p.setPen(txColor);
    p.drawText(rc, drawnText, QTextOption(Qt::AlignRight | Qt::AlignVCenter));
}

