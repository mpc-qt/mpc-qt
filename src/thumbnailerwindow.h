#ifndef THUMBNAILERWINDOW_H
#define THUMBNAILERWINDOW_H

#include <QImage>
#include <QOpenGLWidget>
#include <QWidget>
#include <QQueue>
#include <QUrl>
#include "mpvwidget.h"

namespace Ui {
class ThumbnailerWindow;
}
class MpvThumbnailDrawer;
class MpvThumbnailer;

class ThumbnailerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailerWindow(QWidget *parent = nullptr);
    ~ThumbnailerWindow();

public slots:
    void open(QUrl sourceUrl);
    void begin();
    void setScreenshotDirectory(QString folder);
    void setScreenshotFormat(QString format);

private slots:  
    void on_saveImageBrowse_clicked();
    void thumbnailer_setProgress(int percent);
    void thumbnailer_finished();

private:
    Ui::ThumbnailerWindow *ui = nullptr;
    MpvThumbnailer *thumbnailer = nullptr;
    QString screenshotDirectory;
    QString screenshotFormat = "png";
};

class MpvThumbnailer : public QObject {
    Q_OBJECT

    enum ThumbnailingState {
        AvailableState, // Available for use
        StartedState,   // File opened
        StaleState,     // File opened, but no video size yet
        SeekingState,   // Seek command sent
        PlayingState,   // Playing video until frozen
        WaitingForTimer, // Waiting for snapshot timer
        FinishedState   // Playback finished
    };

    struct ThumbPts {
        int x, y;
        double pts;
        int percent, index;
        QImage thumb;
    };

public:
    struct Params {
        QUrl sourceUrl;
        QString imageFile;
        int jpegQuality, imageWidth;
        int cols, rows;
    };

    explicit MpvThumbnailer(QObject *parent);
    ~MpvThumbnailer();
    void execute(const Params &p);

signals:
    void progress(int percent);
    void finished();

private:
    void initPlayer();
    void deinitPlayer();

    void initThumbPts();
    void processThumb();
    bool seekNextFrame();
    void renderImage();
    void saveImage();

private slots:
    void mpv_fileSizeChanged(int64_t bytes);
    void mpv_videoSizeChanged(QSize size);
    void mpv_playLengthChanged(double length);
    void mpv_playTimeChanged(double time);
    void mpv_playbackFinished();
    void mpv_eofReachedChanged();
    void timer_navigateTick();

private:
    Params p;
    MpvObject *mpv = nullptr;
    MpvThumbnailDrawer *thumbnailer = nullptr;

    ThumbnailingState thumbState = AvailableState;
    double mpvTime = -1;
    double mpvDuration = -1;
    Helpers::TimeFormat osdTimeFormat = Helpers::ShortFormat;
    int64_t mpvFileSize = 0;
    QSize mpvVideoSize = {-1,-1};
    QQueue<ThumbPts> pendingPts;
    QQueue<ThumbPts> processedPts;
    QImage render;
    QSize thumbSize;
};


class MpvThumbnailDrawer : public QOpenGLWidget, public MpvWidgetInterface {
    Q_OBJECT
    Q_INTERFACES(MpvWidgetInterface)

public:
    explicit MpvThumbnailDrawer(MpvObject *object);
    ~MpvThumbnailDrawer();

    QWidget *self();
    void initMpv();
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void setDrawLogo(bool yes);

signals:
    void renderedFrame();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private slots:
    void alwaysUpdate();

private:
    static void render_update(void *ctx);
    mpv_render_context *render = nullptr;
    int glWidth, glHeight;
};

#endif // THUMBNAILERWINDOW_H
