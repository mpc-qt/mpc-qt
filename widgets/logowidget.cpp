#include <QPainter>
#include "logowidget.h"

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

void LogoDrawer::resizeGL(int w, int h)
{
    QTransform t;
    t.scale(2.0/w, 2.0/h);
    t.translate(((w + logo.width())&1)/2.0,
                ((h + logo.height())&1)/2.0);
    logoLocation = t.mapRect(QRectF(-logo.width()/2.0, -logo.height()/2.0,
                                     logo.width(), logo.height()));

    if (logoLocation.height() > 2) {
        t.reset();
        t.scale(2/logoLocation.height(), 2/logoLocation.height());
        logoLocation = t.mapRect(logoLocation);
    }
    if (logoLocation.width() > 2) {
        t.reset();
        t.scale(2/logoLocation.width(), 2/logoLocation.width());
        logoLocation = t.mapRect(logoLocation);
    }
}

void LogoDrawer::paintGL(QOpenGLWidget *widget)
{
    QPainter painter(widget);
    int ratio = widget->devicePixelRatio();
    QRect window(-1, -1, 2*ratio, 2*ratio);
    painter.setWindow(window);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(window, QBrush(logoBackground));
    if (!logo.isNull())
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
        logoDrawer->resizeGL(width(), height());
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
    logoDrawer->resizeGL(w,h);
}
