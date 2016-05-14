// A simple(?) set of sliders related to controlling multimedia players.
#include <QPainter>
#include <cmath>
#include "qdrawnslider.h"



// Convert a rectangle to a string, useful for debugging purposes
static QString r2s(QRectF r) {
    (void)r2s;  // suppress warning about unused function
    return QString("(%1,%2)-(%3,%4)[%5x%6]")
            .arg(r.left()).arg(r.top()).arg(r.right()).arg(r.bottom())
            .arg(r.width()).arg(r.height());
}

// dr: drawrect electric floating boogaloo
// As the doc says, QPainter draws the right and bottom edges one pixel below
// and to the right of the expected location.  So provide a helper function
// that draws an adjusted rect to keep the main math clean.  As it turns out,
// we can avoid a whole host of bugs by using doubles rather than ints.
static void dr(QPainter *p, QRectF r) {
    QRectF r2(r.left() + 0.5, r.top() + 0.5, r.width() - 1, r.height() - 1);
    p->drawRect(r2);
}

// C++18 can't come soon enough
template <class T>
static const T& clamp(const T& value, const T& low, const T& high)
{
    return std::min(std::max(value, low), high);
}



QDrawnSlider::QDrawnSlider(QWidget *parent, QSize handle, QSize margin) :
    QWidget(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setMouseTracking(true);
    setSliderGeometry(handle.width(), handle.height(),
                      margin.width(), margin.height());
    isDragging = false;
}

void QDrawnSlider::setSliderGeometry(int handleWidth, int handleHeight,
                                     int marginX, int marginY)
{
    this->handleWidth = handleWidth;
    this->handleHeight = handleHeight;
    this->marginX = marginX;
    this->marginY = marginY;
    setMinimumHeight(handleHeight);
    setMaximumHeight(handleHeight);
}

void QDrawnSlider::handleHover(double x)
{
    // do nothing by default
    (void)x;
}

double QDrawnSlider::valueToX(double value)
{
    double stride = sliderArea.right() - sliderArea.left();
    double x = sliderArea.left() + (((value-minimum()) * stride)
                                 / std::max(1.0, maximum() - minimum()));
    return clamp(x, sliderArea.left(), sliderArea.right());
}

double QDrawnSlider::xToValue(double x)
{
    double stride = std::max(1.0, sliderArea.right() - sliderArea.left());
    double val = ((x-sliderArea.left()) * (maximum() - minimum()))
            / stride;
    return clamp(val, minimum(), maximum());
}

void QDrawnSlider::paintEvent(QPaintEvent *ev)
{
    (void)ev;
    QPainter p(this);

    QPalette pal;
    pal = reinterpret_cast<QWidget*>(parentWidget())->palette();
    grooveBorder = pal.color(QPalette::Normal, QPalette::Shadow);
    grooveFill   = pal.color(QPalette::Normal, QPalette::Base);
    handleBorder = pal.color(QPalette::Normal, QPalette::Dark);
    handleFill   = pal.color(QPalette::Normal, QPalette::Button);
    loopColor    = pal.color(QPalette::Normal, QPalette::Highlight);
    markColor    = pal.color(QPalette::Normal, QPalette::Shadow);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setOpacity(isEnabled() ? 1.0 : 0.333);

    drawGroove(&p);
    if (minimum() != maximum())
        drawHandle(&p, isDragging ? xPosition : valueToX(value()));
}

void QDrawnSlider::resizeEvent(QResizeEvent *re)
{
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

    (void)re;
    int w = re->size().width();
    int h = re->size().height();
    grooveArea = QRectF(0, 0, w, h);
    drawnArea = grooveArea;
    grooveArea.adjust(marginX, marginY, -marginX, -marginY);
    sliderArea = grooveArea;
    sliderArea.adjust(0, 0, -(handleWidth&1), 0);
}

void QDrawnSlider::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        isDragging = true;
        xPosition = ev->localPos().x();
        setValue(xToValue(ev->localPos().x()));
        sliderMoved(value());
    }
}

void QDrawnSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    if (isDragging && ev->button() == Qt::LeftButton) {
        isDragging = false;
        update();
    }
}

void QDrawnSlider::mouseMoveEvent(QMouseEvent *ev)
{
    if (isDragging && ev->buttons() & Qt::LeftButton) {
        double mouseValue = xToValue(ev->localPos().x());
        double mouseX = clamp(ev->localPos().x(), sliderArea.left(),
                              sliderArea.right());
        if (value() != mouseValue) {
            setValue(mouseValue);
            sliderMoved(value());
        }
        if (mouseX != xPosition) {
            xPosition = mouseX;
            update();
        }
    }
    handleHover(ev->localPos().x());
}



QMediaSlider::QMediaSlider(QWidget *parent) :
    QDrawnSlider(parent, QSize(11, 12), QSize(5, 3)),
    vLoopA(-1), vLoopB(-1), loopArea(-1,-1,0,0)
{
}

void QMediaSlider::clearTicks()
{
    ticks.clear();
    vLoopA = vLoopB = -1;
    loopArea = {-1, -1, 0, 0};
}

void QMediaSlider::setTick(double value, QString text)
{
    ticks.insert(value, text);
}

void QMediaSlider::drawGroove(QPainter *p)
{
    // Draw inside area
    p->setPen(Qt::NoPen);
    p->setBrush(grooveFill);
    dr(p, grooveArea);

    // Draw any highlighted area
    p->setPen(loopColor);
    p->setBrush(loopColor);
    if (vLoopA >= 0 && vLoopB >= 0) {
        p->setBrush(loopColor);
        dr(p, loopArea);
    } else if (vLoopA >= 0) {
        double pos = valueToX(vLoopA);
        p->drawLine(QPointF(pos + 0.5, grooveArea.top() + 1.5),
                    QPointF(pos + 0.5, grooveArea.bottom() - 1.5));
    } else if (vLoopB >= 0) {
        double pos = valueToX(vLoopB);
        p->drawLine(QPointF(pos + 0.5, grooveArea.top() + 1.5),
                    QPointF(pos + 0.5, grooveArea.bottom() - 1.5));

    }

    // Draw outside groove
    p->setPen(grooveBorder);
    p->setBrush(Qt::NoBrush);
    dr(p, grooveArea);

    // Draw chapter marks
    for (auto i = ticks.constBegin(); i != ticks.constEnd(); i++) {
        double pos = valueToX(i.key());
        // Don't draw over the edge of the groove twice when disabled, so the
        // affected groove sides don't appear dark.
        if (isEnabled() || (pos > grooveArea.left() + 1.0 &&
                            pos < grooveArea.right() - 1.0)) {
            p->drawLine(QPointF(pos + 0.5, grooveArea.top() + 0.5),
                        QPointF(pos + 0.5, grooveArea.bottom() - 0.5));
        }
    }
}

void QMediaSlider::drawHandle(QPainter *p, double x)
{
    QRectF slider(x - 5, 0, handleWidth, handleHeight);
    slider.adjust(1.5, 1.5, -1.5, -1.5);
    p->setBrush(Qt::NoBrush);
    p->setPen(QPen(handleBorder, 4, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    dr(p, slider);
    p->setPen(QPen(handleFill, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    dr(p, slider);
}

void QMediaSlider::enterEvent(QEvent *event)
{
    (void)event;
    hoverBegin();
}

void QMediaSlider::leaveEvent(QEvent *event)
{
    (void)event;
    hoverEnd();
}

void QMediaSlider::handleHover(double x)
{
    double valueOfX = xToValue(x);

    hoverValue(valueOfX, valueToTickText(valueOfX), x);
}

void QMediaSlider::updateLoopArea()
{
    double left = valueToX(vLoopA);
    double right = valueToX(vLoopB);
    loopArea = {left, grooveArea.top() + 1, right - left, grooveArea.height() - 2};
    update();
}

QString QMediaSlider::valueToTickText(double value)
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



QVolumeSlider::QVolumeSlider(QWidget *parent) :
    QDrawnSlider(parent, QSize(10, 20), QSize(5, 10))
{
}

void QVolumeSlider::drawGroove(QPainter *p)
{
    double x1 = drawnArea.left() + 0.5;
    double y1 = drawnArea.top() + 0.5;
    double x2 = drawnArea.width() - 0.5;
    double y2 = drawnArea.height() - 0.5;
    QPointF groove[] = { QPointF(x1, y2), QPointF(x2, y2), QPointF(x2, y1) };

    p->setPen(grooveBorder);
    p->setBrush(grooveFill);
    p->drawConvexPolygon(groove, 3);
}

void QVolumeSlider::drawHandle(QPainter *p, double x)
{
    QRectF slider(QPointF(x - (handleWidth / 2),
                          sliderArea.top() - (handleHeight / 2)),
                  QSizeF(handleWidth, handleHeight));

    p->setBrush(handleFill);
    p->setPen(handleBorder);
    dr(p, slider);
}

