#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include "helpers.h"
#include "tooltip.h"
#include "drawnstatus.h"

static constexpr char logModule[] =  "drawnstatus";

StatusTime::StatusTime(QWidget *parent) : QWidget(parent)
{
    setTime(0, 0);
    setMinimumSize(minimumSizeHint());
    setAttribute(Qt::WA_Hover);
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
    shortMode_ = shortened;
    lastTextLength = 0;
    lastDrawnText.clear();
    updateTimeFormat();
    updateText();
}

void StatusTime::setRemainingMode(bool isRemaining)
{
    remainingMode_ = isRemaining;
    lastTextLength = 0;
    lastDrawnText.clear();
    updateText();
}

void StatusTime::setPercentMode(bool isPercent)
{
    percentMode_ = isPercent;
    lastTextLength = 0;
    lastDrawnText.clear();
    updateText();
}

void StatusTime::updateTimeFormat()
{
    Helpers::TimeFormat format;
    if (int(currentDuration*1000 + 0.5) >= 3600000)
        format = shortMode_ ? Helpers::ShortFormat : Helpers::LongFormat;
    else
        format = shortMode_ ? Helpers::ShortHourFormat : Helpers::LongHourFormat;
    if (timeFormat == format)
        return;
    timeFormat = format;
}

void StatusTime::updateText()
{
    double time = currentTime;
    if (remainingMode_)
        time = currentTime - currentDuration;

    drawnText = Helpers::toDateFormatFixed(time, timeFormat);
    drawnText.append(" / ");
    drawnText.append(Helpers::toDateFormatFixed(currentDuration, timeFormat));

    if (percentMode_) {
        double durationNotNull = currentDuration == 0 ? 1 : currentDuration;
        drawnText.append(QString(tr(" (%1%)")).arg(QLocale().toString(currentTime / durationNotNull * 100,
                                                                'f',
                                                                1)));
    }

    if (hovered)
        updateHoverTooltip();

    if (drawnText == lastDrawnText)
        return;
    lastDrawnText = drawnText;
    int drawnTextLength = drawnText.length();
    if (drawnTextLength != lastTextLength) {
        setMinimumSize(minimumSizeHint());
        lastTextLength = drawnTextLength;
    }
    update();
}

void StatusTime::ensureHoverTooltip()
{
    if (hoverTooltip)
        return;
    QWidget *top = window();
    if (!top || top == this)
        return;
    hoverTooltip = new Tooltip(top);
}

void StatusTime::updatePalette()
{
    if (hoverTooltip)
        hoverTooltip->updatePalette();
}

void StatusTime::showContextMenu(const QPointF &p)
{
    QMenu *m = new QMenu(window());
    QAction *a;

    bool isTimeRemaingMode = remainingMode_;
    a = new QAction(m);
    a->setText(tr("Remaining time"));
    a->setCheckable(true);
    a->setChecked(remainingMode_);
    connect(a, &QAction::triggered,
            this, [this, isTimeRemaingMode]() {
        setRemainingMode(!isTimeRemaingMode);
    });
    m->addAction(a);

    bool isTimeShortMode = shortMode_;
    a = new QAction(m);
    a->setText(tr("High precision"));
    a->setCheckable(true);
    a->setChecked(!shortMode_);
    connect(a, &QAction::triggered,
            this, [this, isTimeShortMode]() {
        setShortMode(!isTimeShortMode);
    });
    m->addAction(a);

    bool isTimePercentageMode = percentMode_;
    a = new QAction(m);
    a->setText(tr("Show percentage"));
    a->setCheckable(true);
    a->setChecked(percentMode_);
    connect(a, &QAction::triggered,
            this, [this, isTimePercentageMode]() {
        setPercentMode(!isTimePercentageMode);
    });
    m->addAction(a);

    m->exec(mapToGlobal(p).toPoint());
    if (hoverTooltip)
        hoverTooltip->hide();
    delete m;
}

void StatusTime::updateHoverTooltip()
{
    if (!hoverTooltip)
        return;
    QWidget *top = window();
    if (!top)
        return;
    QString label = remainingMode_ ? tr("Played") : tr("Remaining");
    double hoverTime = remainingMode_ ? currentTime : currentTime - currentDuration;
    QString text = QString("%1: %2").arg(label,
                                         Helpers::toDateFormatFixed(hoverTime, timeFormat));
    QString textTemplate = QString("%1: %2").arg(label,
                                                 Helpers::toDateFormatFixed(currentDuration, timeFormat));
    QPoint topLeft = mapTo(top, QPoint(0, 0));
    QPoint where(topLeft.x() + width() / 2, topLeft.y() - 2);
    hoverTooltip->show(text, where, top->width(), textTemplate);
}

bool StatusTime::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
        hovered = true;
        ensureHoverTooltip();
        updateHoverTooltip();
        break;
    case QEvent::HoverLeave:
        hovered = false;
        if (hoverTooltip)
            hoverTooltip->hide();
        break;
    case QEvent::ToolTip:
        event->accept();
        return true;
    default:
        break;
    }
    return QWidget::event(event);
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

void StatusTime::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange)
        setVisible(isEnabled());
}

void StatusTime::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit doubleClicked();
    QWidget::mouseDoubleClickEvent(event);
}
