#include <QPainter>
#include "logowidget.h"

static constexpr char logModule[] =  "logowidget";

LogoDrawer::LogoDrawer(QObject *parent)
    : QObject(parent)
{
    setLogoUrl("");
    setLogoBackground(QColor(0,0,0));
}

LogoDrawer::~LogoDrawer()
{

}

void LogoDrawer::setLogoUrl(const QString &filename)
{
    logoUrl = filename.isEmpty() ? ":/images/bitmaps/blank-screen.png"
                                 : filename;
    regenerateTexture();
}

void LogoDrawer::setLogoBackground(const QColor &color)
{
    logoBackground = color.isValid() ? color : QColor(0,0,0);
}

void LogoDrawer::resizeGL(int w, int h, qreal pixelRatio)
{
    qreal lw = logo.width() / pixelRatio;
    qreal lh = logo.height() / pixelRatio;
    if (lh > h) { // too tall
        qreal scale = h/std::max(1.0,lh);
        lh = h;
        lw *= scale;
    }
    if (lw > w) { // too wide
        qreal scale = w/std::max(1.0,lw);
        lw = w;
        lh *= scale;
    }
    qreal lx = (w-lw) / 2;
    qreal ly = (h-lh) / 2;

    // remove subpixel offset
    qreal px = lx * pixelRatio;
    qreal py = ly * pixelRatio;
    lx -= (px-floor(px)) / pixelRatio;
    ly -= (py-floor(py)) / pixelRatio;

    logoLocation = QRectF(lx, ly, lw, lh);
}

void LogoDrawer::paintGL(QOpenGLWidget *widget)
{
    QPainter painter(widget);
    QRect bgRect = {0, 0, widget->width(), widget->height()};
    painter.fillRect(bgRect, QBrush(logoBackground));

    if (logo.isNull())
        return;
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(logoLocation, logo);
}

void LogoDrawer::regenerateTexture()
{
    logo.load(logoUrl);
    emit logoSize(logo.size());
}



LogoWidget::LogoWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

LogoWidget::~LogoWidget()
{
    if (logoDrawer) {
        makeCurrent();
        delete logoDrawer;
        logoDrawer = nullptr;
    }
}

void LogoWidget::setLogo(const QString &filename) {
    logoUrl = filename;
    if (logoDrawer) {
        makeCurrent();
        logoDrawer->setLogoUrl(filename);
        logoDrawer->resizeGL(width(), height(), devicePixelRatioF());
        doneCurrent();
        update();
    }
}

void LogoWidget::setLogoBackground(const QColor &color)
{
    logoBackground = color;
    if (logoDrawer)
        logoDrawer->setLogoBackground(color);
}

void LogoWidget::initializeGL()
{
    if (!logoDrawer) {
        logoDrawer = new LogoDrawer(this);
        logoDrawer->setLogoUrl(logoUrl);
        logoDrawer->setLogoBackground(logoBackground);
    }
}

void LogoWidget::paintGL()
{
    logoDrawer->paintGL(this);
}

void LogoWidget::resizeGL(int w, int h)
{
    logoDrawer->resizeGL(w, h, devicePixelRatioF());
}
