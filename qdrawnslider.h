#ifndef QDRAWNSLIDER_H
#define QDRAWNSLIDER_H
// A set of simplistic (okay, not really) multimedia widgets that mimics stock
// QSliders, but completely disregards their storage semantics.
//
// These widgets use doubles rather than integers to store their positional-
// related attributes.  This is important because timestamps are usually more
// accurate than a single second.

#include <QOpenGLWidget>
#include <QMouseEvent>

class QDrawnSlider : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit QDrawnSlider(QWidget *parent, QSize handle, QSize margin);
    void setValue(double v) { vValue= v; update(); }
    void setMaximum(double v) { vMaximum = v; update(); }
    void setMinimum(double v) { vMinimum = v; update(); }
    double value() { return vValue; }
    double maximum() { return vMaximum; }
    double minimum() { return vMinimum; }

signals:
    void sliderMoved(double v);

public slots:

protected:
    void setSliderGeometry(int handleWidth, int handleHeight,
                           int marginX, int marginY);
    virtual void drawGroove(QPainter *p) = 0;
    virtual void drawHandle(QPainter *p, double x) = 0;
    virtual void handleHover(double x);

    double valueToX(double value);
    double xToValue(double x);

    void updateLoopArea();

    QRectF drawnArea;
    QRectF grooveArea;
    QRectF sliderArea;
    QColor grooveBorder, grooveFill;
    QColor handleBorder, handleFill;
    QColor bgColor, loopColor, markColor;
    int handleWidth, handleHeight, marginX, marginY;

private:
    void paintGL();
    void resizeGL(int w, int h);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);

    bool isDragging;
    double xPosition;

    double vValue;
    double vMaximum;
    double vMinimum;
};

class QMediaSlider : public QDrawnSlider {
    Q_OBJECT
public:
    explicit QMediaSlider(QWidget *parent = 0);

    void clearTicks();
    void setTick(double value, QString text);
    void setLoopA(double a) { vLoopA = a; updateLoopArea(); }
    void setLoopB(double b) { vLoopB = b; updateLoopArea(); }
    double loopA() { return vLoopA; }
    double loopB() { return vLoopB; }
    bool isLoopEmpty() { return vLoopA < 0 || vLoopB < 0; }

signals:
    void hoverBegin();
    void hoverEnd();
    void hoverValue(double value, QString text, double x);

protected:
    void drawGroove(QPainter *p);
    void drawHandle(QPainter *p, double x);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void handleHover(double x);

    void updateLoopArea();

    QString valueToTickText(double value);
    QMap<double, QString> ticks;
    double vLoopA;
    double vLoopB;
    QRectF loopArea;
};

class QVolumeSlider : public QDrawnSlider {
    Q_OBJECT

public:
    explicit QVolumeSlider(QWidget *parent = 0);

protected:
    void drawGroove(QPainter *p);
    void drawHandle(QPainter *p, double x);
};
#endif // QDRAWNSLIDER_H
