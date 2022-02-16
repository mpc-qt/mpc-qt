// A simple(?) set of sliders related to controlling multimedia players.
#include <QPainter>
#include <QOpenGLContext>
#include <QTimer>
#include <cmath>
#include "drawnslider.h"



// dr: drawrect electric floating boogaloo
// As the doc says, QPainter draws the right and bottom edges one pixel below
// and to the right of the expected location.  So provide a helper function
// that draws an adjusted rect to keep the main math clean.  As it turns out,
// we can avoid a whole host of bugs by using doubles rather than ints.
static void dr(QPainter *p, QRectF r) {
    QRectF r2(r.left() + 0.5, r.top() + 0.5, r.width() - 1, r.height() - 1);
    p->drawRect(r2);
}



DrawnSlider::DrawnSlider(QWidget *parent, QSize handle, QSize margin) :
    QWidget(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setSliderGeometry(handle.width(), handle.height(),
                      margin.width(), margin.height());
}

void DrawnSlider::setValue(double v)
{
    vValue = qBound(vMinimum, v, vMaximum);
    xPosition = valueToX(vValue);
    update();
}

void DrawnSlider::setMaximum(double v)
{
    vMaximum = v;
    if (vValue > v)
        setValue(v);
    else
        update();
}

void DrawnSlider::setMinimum(double v)
{
    vMinimum = v;
    if (vValue < v)
        setValue(v);
    else
        update();
}

double DrawnSlider::value()
{
    return vValue;
}

double DrawnSlider::maximum()
{
    return vMaximum;
}

double DrawnSlider::minimum()
{
    return vMinimum;
}

void DrawnSlider::setHighContrast(bool enabled)
{
    highContrast = enabled;
    redrawPics = true;
    update();
}

void DrawnSlider::setSliderGeometry(int handleWidth, int handleHeight,
                                     int marginX, int marginY)
{
    this->handleWidth = handleWidth;
    this->handleHeight = handleHeight;
    this->marginX = marginX;
    this->marginY = marginY;
    setMinimumHeight(handleHeight);
    setMaximumHeight(handleHeight);
}

void DrawnSlider::handleHover(double x)
{
    // do nothing by default
    (void)x;
}

double DrawnSlider::valueToX(double value)
{
    double stride = sliderArea.right() - sliderArea.left();
    double x = sliderArea.left() + (((value-minimum()) * stride)
                                 / std::max(1.0, maximum() - minimum()));
    return qBound(x, sliderArea.left(), sliderArea.right());
}

double DrawnSlider::xToValue(double x)
{
    double stride = std::max(1.0, sliderArea.right() - sliderArea.left());
    double val = ((x-sliderArea.left()) * (maximum() - minimum()))
            / stride;
    return qBound(val, minimum(), maximum());
}

void DrawnSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPalette pal;
    pal = reinterpret_cast<QWidget*>(parentWidget())->palette();
    if (highContrast) {
        grooveBorder = pal.color(QPalette::Normal, QPalette::Text);
        grooveFill   = pal.color(QPalette::Normal, QPalette::Window);
        handleBorder = pal.color(QPalette::Normal, QPalette::Text);
        handleFill   = pal.color(QPalette::Normal, QPalette::Window);
        bgColor      = pal.color(QPalette::Normal, QPalette::Window);
        loopColor    = pal.color(QPalette::Normal, QPalette::Highlight);
        markColor    = pal.color(QPalette::Normal, QPalette::Text);
    } else {
        grooveBorder = pal.color(QPalette::Normal, QPalette::Shadow);
        grooveFill   = pal.color(QPalette::Normal, QPalette::Base);
        handleBorder = pal.color(QPalette::Normal, QPalette::Button);
        handleFill   = pal.color(QPalette::Normal, QPalette::Button);
        bgColor      = pal.color(QPalette::Normal, QPalette::Window);
        loopColor    = pal.color(QPalette::Normal, QPalette::Highlight);
        markColor    = pal.color(QPalette::Normal, QPalette::Button);

        // is this a light theme?
        if (pal.color(QPalette::Normal, QPalette::Button).value() >
                pal.color(QPalette::Normal, QPalette::Text).value()) {
            handleBorder = handleBorder.darker();
            markColor = markColor.darker();
        } else {
            handleBorder = handleBorder.lighter();
            markColor = markColor.lighter();
        }
    }

    if (redrawPics) {
        makeBackground();
        makeHandle();
        redrawPics = false;
    }

    QPainter p(this);
    int pr = devicePixelRatio();
    p.scale(1.0/pr, 1.0/pr);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.drawImage(0, 0, backgroundPic);
    p.setOpacity(isEnabled() ? 1.0 : 0.333);

    if (minimum() != maximum()) {
        double px;
        double x = isDragging ? xPosition : valueToX(value());
        x -= handleWidth/2.0;
        x *= pr;
        int index = int(modf(x, &px) * 16.0)&15;
        p.drawImage(QPointF(px - 0.5, (height() - handleHeight)/2), handlePics[index]);
    }
}

void DrawnSlider::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    /*
        MEDIA SLIDER CASE

                  <---- SLIDER AREA
  ^     +------------------------
        |
        |< marginX>
        |
        +------------------+-----   ^     ^
        |
        |                  |     marginY
        |
        |         +---------------  v
        |         |
height  |         |   GROOVE              hH
        |         |
        |         +---------------
        |               DRAWN
        |                  |
        |                AREA
        +------------------+-----         v
        |
        |
        |
  v     +------------------------


        VOLUME SLIDER CASE

                  <---- SLIDER AREA
  ^     +------------------------
        |
        |<  hW/2  >
        |
        +------------------+-----     ^
        |
        |                  |
        |              DRAWN AREA
        |                  |
        |
height  |         +          ----    hH
        |                ----
        |            ----  |
        |        ----
        |    ----          |
        |----
        +------------------+----  ^   v
        |
        |                      padding
        |
  v     +-----------------------  v

    Therefore, "drawn area" is actually the control area, minus not needed
    padding.

    Therefore, "groove area" is actually the drawn area, minus any margin.

    Therefore, "slider area" is actually the drawn area, minus the handle
    area.

    Therefore, a volume slider ignores the groove area as it has no groove.
    However, it should provide 'margins' equal to half its handle size, as
    these relate to where the middle of the handle is.
    */

    grooveArea = QRectF(0, 0, width(), height());
    drawnArea = grooveArea;
    grooveArea.adjust(marginX, marginY, -marginX, -marginY);
    sliderArea = grooveArea;
    sliderArea.adjust(0, 0, -(handleWidth&1), 0);
    redrawPics = true;
}

void DrawnSlider::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        isDragging = true;
        xPosition = ev->localPos().x();
        setValue(xToValue(ev->localPos().x()));
        emit sliderMoved(value());
    }
}

void DrawnSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    if (isDragging && ev->button() == Qt::LeftButton) {
        isDragging = false;
        update();
    }
}

void DrawnSlider::mouseMoveEvent(QMouseEvent *ev)
{
    if (isDragging && ev->buttons() & Qt::LeftButton) {
        double mouseValue = xToValue(ev->localPos().x());
        if (value() != mouseValue) {
            setValue(mouseValue);
            emit sliderMoved(value());
        }
    }
    handleHover(ev->localPos().x());
    // Forward mouse move events also to the parent widget
    ev->ignore();
}

MediaSlider::MediaSlider(QWidget *parent) :
    DrawnSlider(parent, QSize(11, 12), QSize(5, 3))
{
}

void MediaSlider::clearTicks()
{
    ticks.clear();
    vLoopA = vLoopB = -1;
    loopArea = { -1, -1, 0, 0 };
    redrawPics = true;
}

void MediaSlider::setTick(double value, QString text)
{
    ticks.insert(value, text);
}

void MediaSlider::setLoopA(double a)
{
    vLoopA = a; updateLoopArea();
}

void MediaSlider::setLoopB(double b)
{
    vLoopB = b; updateLoopArea();
}

double MediaSlider::loopA()
{
    return vLoopA;
}

double MediaSlider::loopB() {
    return vLoopB;
}

bool MediaSlider::isLoopEmpty()
{
    return vLoopA < 0 || vLoopB < 0;
}

void MediaSlider::resizeEvent(QResizeEvent *event)
{
    DrawnSlider::resizeEvent(event);
    updateLoopArea();
}

void MediaSlider::makeBackground()
{
    int pr = devicePixelRatio();
    int pw = width() * pr;
    int ph = width() * pr;
    backgroundPic = QImage(pw, ph, QImage::Format_RGB32);
    backgroundPic.fill(bgColor);
    QPainter p(&backgroundPic);
    p.scale(pr, pr);

    // Draw inside area
    p.setPen(Qt::NoPen);
    p.setBrush(grooveFill);
    dr(&p, grooveArea);

    // Draw any highlighted area
    p.setPen(loopColor);
    p.setBrush(loopColor);
    if (vLoopA >= 0 && vLoopB >= 0) {
        p.setBrush(loopColor);
        dr(&p, loopArea);
    } else if (vLoopA >= 0) {
        double pos = valueToX(vLoopA);
        p.drawLine(QPointF(pos + 0.5, grooveArea.top() + 1.5),
                   QPointF(pos + 0.5, grooveArea.bottom() - 1.5));
    } else if (vLoopB >= 0) {
        double pos = valueToX(vLoopB);
        p.drawLine(QPointF(pos + 0.5, grooveArea.top() + 1.5),
                    QPointF(pos + 0.5, grooveArea.bottom() - 1.5));

    }

    // Draw chapter marks
    p.setPen(markColor);
    for (auto i = ticks.constBegin(); i != ticks.constEnd(); i++) {
        double pos = valueToX(i.key());
        // Don't draw over the edge of the groove twice when disabled, so the
        // affected groove sides don't appear dark.
        if (isEnabled() || (pos > grooveArea.left() + 1.0 &&
                            pos < grooveArea.right() - 1.0)) {
            p.drawLine(QPointF(pos + 0.5, grooveArea.top() + 0.5),
                       QPointF(pos + 0.5, grooveArea.bottom() - 0.5));
        }
    }

    // Draw outside groove
    p.setPen(grooveBorder);
    p.setBrush(Qt::NoBrush);
    dr(&p, grooveArea);


}

void MediaSlider::makeHandle()
{
    int pr = devicePixelRatio();
    int pw = handleWidth * pr;
    int ph = handleHeight * pr;

    for (int i = 0; i < 16; i++) {
        QImage handlePic(pw+1, ph, QImage::Format_ARGB32_Premultiplied);
        handlePic.fill(0);
        QPainter p(&handlePic);
        p.setRenderHint(QPainter::Antialiasing);
        p.setTransform(QTransform().translate(i/16.0,0).scale(pr,pr));
        QRectF slider(0, 0, handleWidth, handleHeight);
        slider.adjust(1.5, 1.5, -1.5, -1.5);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(handleBorder, 4, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        dr(&p, slider);
        p.setPen(QPen(handleFill, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        dr(&p, slider);
        handlePics[i] = handlePic;
    }
}

void MediaSlider::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
    emit hoverBegin();
}

void MediaSlider::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    emit hoverEnd();
}

void MediaSlider::handleHover(double x)
{
    double valueOfX = xToValue(x);

    emit hoverValue(valueOfX, valueToTickText(valueOfX), x);
}

void MediaSlider::updateLoopArea()
{
    double left = valueToX(vLoopA);
    double right = valueToX(vLoopB);
    loopArea = {left, grooveArea.top() + 1, right - left, grooveArea.height() - 2};
    update();
}

QString MediaSlider::valueToTickText(double value)
{
    QString last_text;

    double last_tick = -1;
    for (auto i = ticks.constBegin(); i != ticks.constEnd(); i++) {
        last_tick = i.key();
        if (last_tick > value)
            break;
        last_text = i.value();
    }
    return last_text;
}



VolumeSlider::VolumeSlider(QWidget *parent) :
    DrawnSlider(parent, QSize(10, 20), QSize(5, 10))
{
}

void VolumeSlider::makeBackground()
{
    int pr = devicePixelRatio();
    int pw = int(drawnArea.width() * pr);
    int ph = int(drawnArea.height() * pr);
    backgroundPic = QImage(pw, ph, QImage::Format_RGB32);
    backgroundPic.fill(bgColor);
    QPainter p(&backgroundPic);
    p.scale(pr, pr);

    double x1 = drawnArea.left() + 0.5;
    double y1 = drawnArea.top() + 0.5;
    double x2 = drawnArea.width() - 0.5;
    double y2 = drawnArea.height() - 0.5;
    QPointF groove[] = { QPointF(x1, y2), QPointF(x2, y2), QPointF(x2, y1) };

    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(grooveBorder);
    p.setBrush(grooveFill);
    p.drawConvexPolygon(groove, 3);
}

void VolumeSlider::makeHandle()
{
    int pr = devicePixelRatio();
    int pw = handleWidth * pr;
    int ph = handleHeight * pr;
    for (int i = 0; i < 16; i++) {
        QImage handlePic(pw+1, ph, QImage::Format_ARGB32);
        handlePic.fill(0);
        QPainter p(&handlePic);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(pr, pr);
        p.translate(i/16.0,0);
        p.setBrush(handleFill);
        p.setPen(handleBorder);
        dr(&p, QRectF(0,0,handleWidth,handleHeight));
        handlePics[i] = handlePic;
    }
}
