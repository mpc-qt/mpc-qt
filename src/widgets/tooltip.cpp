#include <QApplication>
#include "tooltip.h"

static constexpr char logModule[] =  "tooltip";
static constexpr int windowSidesMargin = 20;
static constexpr int insidePadding = 10;

Tooltip::Tooltip(QWidget *parent) : QWidget(parent)
{
    textLabel = new QLabel(parent);
    textLabel->setAlignment(Qt::AlignCenter);

    textLabel->setAutoFillBackground(true);
    updatePalette();

    hide();
    this->setVisible(false);
}

Tooltip::~Tooltip()
{
    delete textLabel;
    textLabel = nullptr;
}

void Tooltip::updatePalette()
{
    QPalette palette = QApplication::palette();
    QString css = QString(
        "background-color: %1;"
        "color: %2;"
        "border: 1px solid %3;"
        "border-radius: 4px;"
    )
    .arg(palette.color(QPalette::ToolTipBase).name(),
        palette.color(QPalette::ToolTipText).name(),
        palette.color(QPalette::Mid).name());

    textLabel->setStyleSheet(css);
}

void Tooltip::show(const QString &text, const QPoint &where, int mainWindowWidth, const QString &textTemplate)
{
    textLabel->setText(text);
    auto rect = QFontMetrics(font()).boundingRect(textTemplate);
    textLabel->setFixedSize(rect.width() + insidePadding, rect.height() + insidePadding);
    setPosition(where, mainWindowWidth);
    textLabel->move(bottomLeft.x(),
                    bottomLeft.y() - textLabel->height());
    textLabel->setVisible(true);
}

void Tooltip::setPosition(const QPoint &where, int mainWindowWidth)
{
    int tooltipWidth = textLabel->width();
    int xPos = where.x() - std::round(tooltipWidth / 2);
    if (xPos + tooltipWidth + windowSidesMargin > mainWindowWidth)
        xPos = mainWindowWidth - tooltipWidth - windowSidesMargin;
    else if (xPos < windowSidesMargin)
        xPos = windowSidesMargin;
    bottomLeft = QPoint(xPos, where.y());
}

void Tooltip::hide() {
    textLabel->setVisible(false);
}
