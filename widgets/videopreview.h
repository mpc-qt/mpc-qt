#ifndef VIDEOPREVIEW_H
#define VIDEOPREVIEW_H

#include <qboxlayout.h>
#include <qlabel.h>
#include "mpvwidget.h"

class VideoPreview : public QWidget {
    public:
        explicit VideoPreview(QWidget *parent = nullptr);
        ~VideoPreview();
        void openFile(const QUrl &fileUrl);
        void setTimeText(const QString &text, double videoPosition);
        void showEvent(QShowEvent *event) override;
        void hideEvent(QHideEvent *event) override;
        
    private:
        QLabel *textLabel;
        QVBoxLayout *layout;
        MpvObject *mpv = nullptr;
        MpvGlWidget *videoWidget = nullptr;
        bool aspectRatioSet = false;
        bool shouldBeShown = false;
    };
    
#endif // VIDEOPREVIEW_H
    