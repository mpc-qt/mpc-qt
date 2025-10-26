#ifndef LOGOWIDGET_H
#define LOGOWIDGET_H

#include <QObject>
#include <QOpenGLWidget>

class LogoDrawer : public QObject {
    Q_OBJECT
public:
    explicit LogoDrawer(QObject *parent = nullptr);
    ~LogoDrawer();
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void resizeGL(int w, int h, qreal pixelRatio);
    void paintGL(QOpenGLWidget *widget);

signals:
    void logoSize(QSize size);

private:
    void regenerateTexture();

private:
    QRectF logoLocation;
    QImage logo;
    QString logoUrl;
    QColor logoBackground;
};

class LogoWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LogoWidget(QWidget *parent = nullptr);
    ~LogoWidget();
    void setLogo(const QString &filename);
    void setLogoBackground(const QColor &color);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    LogoDrawer *logoDrawer = nullptr;
    QString logoUrl;
    QColor logoBackground;
};

#endif // LOGOWIDGET_H
