#include "plainslider.h"
#include <QApplication>
#include <QPainter>
#include <QStyleOption>

PlainSlider::PlainSlider(QWidget *parent) : QSlider(parent)
{
}

void PlainSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    checkPalette();
    drawGroove(p, opt);
    drawHandle(p, opt);
}

void PlainSlider::checkPalette()
{
    if (palette().color(QPalette::Normal, QPalette::Button).value() <
            palette().color(QPalette::Normal, QPalette::Text).value()) {
        darkPalette = true;
    } else {
        darkPalette = false;
    }
}

void PlainSlider::drawGroove(QPainter& p, const QStyleOptionSlider& opt)
{
    QRect groove = style()->subControlRect(
        QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);

    if (darkPalette) {
        grooveFill = palette().color(QPalette::Midlight);
        grooveBorder = palette().color(QPalette::Light);
        grooveBorder = grooveBorder.lighter(150);
    } else {
        grooveFill = palette().color(QPalette::Midlight);
        grooveBorder = palette().color(QPalette::Mid);
    }

    p.setPen(grooveBorder);
    p.setBrush(grooveFill);
    QRectF grooveRect = QRectF(groove.adjusted(7, 1, -7, -1));
    grooveRect = grooveRect.adjusted(0.5, 0.5, -0.5, -0.5);
    p.drawRoundedRect(grooveRect, 1, 1);
}
void PlainSlider::drawHandle(QPainter& p, const QStyleOptionSlider& opt)
{
    QRect handle = style()->subControlRect(
        QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

    if (QApplication::style()->name() != "fusion") {
        QRect original = handle;
        handle.setWidth(handle.height() / 2);
        handle.moveCenter(original.center());
    }

    handleFill = palette().color(QPalette::Button);
    if (darkPalette) {
        handleBorder = palette().color(QPalette::Light);
        handleBorder = handleBorder.lighter(150);
    }
    else {
        handleBorder = palette().color(QPalette::Mid);
    }

    if (opt.state & QStyle::State_MouseOver)
        handleBorder = palette().color(QPalette::Highlight);
    if (opt.state & QStyle::State_Sunken) {
        if (darkPalette)
            handleFill = handleFill.lighter(150);
        else
            handleFill = handleFill.darker(150);
    }

    p.setPen(handleBorder);
    p.setBrush(handleFill);
    QRectF handleRect = QRectF(handle.adjusted(1, 1, -1, -1));
    handleRect = handleRect.adjusted(0.5, 0.5, -0.5, -0.5);
    p.drawRoundedRect(handleRect, 2, 2);
}


