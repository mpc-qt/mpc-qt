#ifndef QDRAWNSTATUS_H
#define QDRAWNSTATUS_H

#include <QWidget>

class StatusTime : public QWidget
{
    Q_OBJECT
public:
    explicit StatusTime(QWidget *parent = nullptr);
    virtual QSize minimumSizeHint() const;

    void setTime(double time);

protected:
    void paintEvent(QPaintEvent *event);

private:
    double currentTime = 0;
    QString drawnText;
};

#endif // QDRAWNSTATUS_H
