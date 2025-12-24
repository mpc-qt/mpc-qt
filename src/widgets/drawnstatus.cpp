#include <QPainter>
#include "helpers.h"
#include "drawnstatus.h"

StatusTime::StatusTime(QWidget *parent) : QWidget(parent)
{
    setTime(0, 0);
    setMinimumSize(minimumSizeHint());
}

QSize StatusTime::minimumSizeHint() const
{
    QSize sz = QFontMetrics(font()).size(0, drawnText);
    return sz;
}

void StatusTime::setTime(double time, double duration)
{
    if (currentDuration != duration) {
        currentDuration = duration;
        updateTimeFormat();
    } else if (currentTime == time)
        return;
    currentTime = time;
    updateText();
}

void StatusTime::setShortMode(bool shortened)
{
    if (shortMode == shortened)
        return;
    shortMode = shortened;
    lastTextLength = 0;
    updateTimeFormat();
    updateText();
}

void StatusTime::updateTimeFormat()
{
    Helpers::TimeFormat format;
    if (int(currentDuration*1000 + 0.5) >= 3600000)
        format = shortMode ? Helpers::ShortFormat : Helpers::LongFormat;
    else
        format = shortMode ? Helpers::ShortHourFormat : Helpers::LongHourFormat;
    if (timeFormat == format)
        return;
    timeFormat = format;
}

void StatusTime::updateText()
{
    drawnText = Helpers::toDateFormatFixed(currentTime, timeFormat);
    int drawnTextLength = drawnText.length();
    if (drawnTextLength != lastTextLength) {
        setMinimumSize(minimumSizeHint());
        lastTextLength = drawnTextLength;
    }
    update();
}

void StatusTime::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    QColor bgColor = parentWidget()->palette().color(QPalette::Active, QPalette::Window);
    QColor txColor = parentWidget()->palette().color(QPalette::Active, QPalette::WindowText);
    QRectF rc = QRectF(QPointF(0,0), QSizeF(size())).adjusted(-0.5,-0.5,0.5,0.5);
    p.fillRect(rc, bgColor);
    p.setPen(txColor);
    p.drawText(rc, drawnText, QTextOption(Qt::AlignRight | Qt::AlignVCenter));
}

