#ifndef QDRAWNSLIDER_H
#define QDRAWNSLIDER_H
// A set of simplistic (okay, not really) multimedia widgets that mimics stock
// QSliders, but completely disregards their storage semantics.
//
// These widgets use doubles rather than integers to store their positional-
// related attributes.  This is important because timestamps are usually more
// accurate than a single second.

#include <QWidget>
#include <QImage>
#include <QMouseEvent>

class DrawnSlider : public QWidget {
    Q_OBJECT

public:
    explicit DrawnSlider(QWidget *parent, QSize handle, QSize margin);
    void setValue(double v);
    void setMaximum(double v);
    void setMinimum(double v);
    double value();
    double maximum();
    double minimum();

signals:
    void sliderMoved(double v);

protected:
    void setSliderGeometry(int handleWidth, int handleHeight,
                           int marginX, int marginY);
    virtual void makeBackground() = 0;
    virtual void makeHandle() = 0;
    virtual void handleHover(double x);

    double valueToX(double value);
    double xToValue(double x);

    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);

    bool redrawPics = true;
    QImage backgroundPic;
    QImage handlePics[16];

    QRectF drawnArea;
    QRectF grooveArea;
    QRectF sliderArea;
    QColor grooveBorder, grooveFill;
    QColor handleBorder, handleFill;
    QColor bgColor, loopColor, markColor;
    int handleWidth, handleHeight, marginX, marginY;

private:
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);

    bool isDragging = false;
    double xPosition = 0.0;

    double vValue = 0.0;
    double vMaximum = 0.0;
    double vMinimum = 0.0;
};

class MediaSlider : public DrawnSlider {
    Q_OBJECT
public:
    explicit MediaSlider(QWidget *parent = 0);

    void clearTicks();
    void setTick(double value, QString text);
    void setLoopA(double a);
    void setLoopB(double b);
    double loopA();
    double loopB();
    bool isLoopEmpty();

signals:
    void hoverBegin();
    void hoverEnd();
    void hoverValue(double value, QString text, double x);

protected:
    void resizeEvent(QResizeEvent *event);
    void makeBackground();
    void makeHandle();
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void handleHover(double x);

    void updateLoopArea();

    QString valueToTickText(double value);
    QMap<double, QString> ticks;
    double vLoopA = -1;
    double vLoopB = -1;
    QRectF loopArea = { -1, -1, 0, 0};
};

class VolumeSlider : public DrawnSlider {
    Q_OBJECT

public:
    explicit VolumeSlider(QWidget *parent = 0);

protected:
    void makeBackground();
    void makeHandle();
};
#endif // QDRAWNSLIDER_H
