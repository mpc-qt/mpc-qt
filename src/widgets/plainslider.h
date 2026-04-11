#ifndef CLEANSLIDER_H
#define CLEANSLIDER_H

#include <QSlider>

class PlainSlider : public QSlider
{
public:
    explicit PlainSlider(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void checkPalette();
    void drawGroove(QPainter& p, const QStyleOptionSlider& opt);
    void drawHandle(QPainter& p, const QStyleOptionSlider& opt);

    QColor grooveFill;
    QColor grooveBorder;
    QColor handleFill;
    QColor handleBorder;

    bool darkPalette = true;
};

#endif // CLEANSLIDER_H
