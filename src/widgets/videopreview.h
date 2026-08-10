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
        void show(const QString &text, double videoPosition, const QPoint &where, int mainWindowWidth, int previewHeight);
        void hide();
        
    private:
        void setPreviewPosition(const QPoint &where, int mainWindowWidth);
        void show();
        void updateWidth(double newAspect);
        void setYtdlRawOptions();

        QLabel *textLabel;
        MpvObject *mpv = nullptr;
        MpvGlWidget *videoWidget = nullptr;
        double aspectRatio = 0;
        bool aspectRatioSet = false;
        bool shouldBeShown = false;
        QPoint previewBottomLeft;
    };
    
#endif // VIDEOPREVIEW_H
    