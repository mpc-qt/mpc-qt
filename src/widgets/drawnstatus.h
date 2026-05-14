#ifndef QDRAWNSTATUS_H
#define QDRAWNSTATUS_H

#include <QWidget>
#include "helpers.h"

class Tooltip;

class StatusTime : public QWidget
{
    Q_OBJECT
public:
    explicit StatusTime(QWidget *parent = nullptr);
    QSize minimumSizeHint() const override;

    void setTime(double time, double duration);
    void setShortMode(bool shortened);
    void setRemainingMode(bool isRemaining);
    void setPercentMode(bool isPercent);
    void updatePalette();

signals:
    void doubleClicked();

protected:
    void updateTimeFormat();
    void updateText();
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

private:
    void ensureHoverTooltip();
    void updateHoverTooltip();

    bool shortMode = false;
    bool remainingMode = false;
    bool percentMode = false;
    bool hovered = false;
    Helpers::TimeFormat timeFormat = Helpers::LongFormat;
    double currentTime = 0;
    double currentDuration = 0;
    QString drawnText;
    QString lastDrawnText;
    int lastTextLength = 0;
    Tooltip *hoverTooltip = nullptr;
};

#endif // QDRAWNSTATUS_H
