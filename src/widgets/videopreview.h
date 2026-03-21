#ifndef VIDEOPREVIEW_H
#define VIDEOPREVIEW_H

#include <qlabel.h>
#include "mpvwidget.h"

class VideoPreview : public QWidget {
    public:
        explicit VideoPreview(QWidget *parent = nullptr);
        ~VideoPreview();
        void openFile(const QUrl &fileUrl);
        void updatePalette();
        void show(const QString &text, double videoPosition, const QPoint &where, int mainWindowWidth);
        void hide();
        
    private:
        void seek();
        void setPreviewPosition(const QPoint &where, int mainWindowWidth);
        void show();
        void updateWidth(double newAspect);
        void positionChanged(double position);

        QLabel *textLabel;
        MpvObject *mpv = nullptr;
        MpvGlWidget *videoWidget = nullptr;
        double currentPosition = 0;
        double requestedPosition = 0;
        double lastRequestedPosition = 0;
        bool isSeeking = false;
        enum class Direction { None, Backward, Forward };
        Direction lastHoverDirection = Direction::None;
        bool aspectRatioSet = false;
        bool shouldBeShown = false;
        QPoint previewBottomLeft;
    };
    
#endif // VIDEOPREVIEW_H
    