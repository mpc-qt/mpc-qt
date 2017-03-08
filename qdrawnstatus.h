#ifndef QDRAWNSTATUS_H
#define QDRAWNSTATUS_H

#include <QWidget>

class QStatusTime : public QWidget
{
    Q_OBJECT
public:
    explicit QStatusTime(QWidget *parent = 0);
    virtual QSize minimumSizeHint() const;

    void setTime(double time);

protected:
    void paintEvent(QPaintEvent *event);

private:
    double currentTime;
    QString drawnText;
};

#endif // QDRAWNSTATUS_H
