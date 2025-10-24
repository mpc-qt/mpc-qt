#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <qlabel.h>

class Tooltip : public QWidget {
    public:
        explicit Tooltip(QWidget *parent = nullptr);
        ~Tooltip();
        void show(const QString &text, const QPoint &where, int mainWindowWidth, const QString &textTemplate);
        void hide();

    private:
        void setPosition(const QPoint &where, int mainWindowWidth);

        QLabel *textLabel;
        bool aspectRatioSet = false;
        QPoint bottomLeft;
    };

#endif // TOOLTIP_H
