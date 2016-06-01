#ifndef QDRAWNSTATUS_H
#define QDRAWNSTATUS_H

#include <QOpenGLWidget>

class QStatusTime : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit QStatusTime(QWidget *parent = 0);

    void setTime(double time);

protected:
    void paintGL();
    QSize minimumSizeHint();

private:
    double currentTime;
    QString drawnText;
};

#endif // QDRAWNSTATUS_H
