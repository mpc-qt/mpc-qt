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

protected:
    void updateTimeFormat();
    void updateText();
    void paintEvent(QPaintEvent *event) override;

private:
    bool shortMode = false;
    Helpers::TimeFormat timeFormat = Helpers::LongFormat;
    double currentTime = 0;
    double currentDuration = 0;
    QString drawnText;
    int lastTextLength = 0;
};

#endif // QDRAWNSTATUS_H
