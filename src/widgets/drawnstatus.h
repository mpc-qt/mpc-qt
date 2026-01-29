#ifndef QDRAWNSTATUS_H
#define QDRAWNSTATUS_H

#include <QWidget>
#include "helpers.h"

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

protected:
    void updateTimeFormat();
    void updateText();
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    bool shortMode = false;
    bool remainingMode = false;
    bool percentMode = false;
    Helpers::TimeFormat timeFormat = Helpers::LongFormat;
    double currentTime = 0;
    double currentDuration = 0;
    QString drawnText;
    int lastTextLength = 0;
};

#endif // QDRAWNSTATUS_H
