#include <QToolTip>
#include "tooltip.h"

static constexpr char logModule[] =  "tooltip";
static constexpr int windowSidesMargin = 20;
static constexpr int insidePadding = 10;

Tooltip::Tooltip(QWidget *parent) : QWidget(parent)
{
    textLabel = new QLabel(parent);
    textLabel->setAlignment(Qt::AlignCenter);

    textLabel->setAutoFillBackground(true);
    QPalette tooltipPalette = QToolTip::palette();
    QPalette customPalette = this->palette();
    customPalette.setColor(QPalette::Window, tooltipPalette.color(QPalette::ToolTipBase));
    customPalette.setColor(QPalette::WindowText, tooltipPalette.color(QPalette::ToolTipText));
    textLabel->setPalette(customPalette);
    textLabel->setStyleSheet(R"(
        QLabel {
            background-color: palette(Window);
            color: palette(WindowText);
            border: 1px solid palette(Light);
            border-radius: 4px;
        }
    )");

    hide();
    this->setVisible(false);
}

Tooltip::~Tooltip()
{
    delete textLabel;
    textLabel = nullptr;
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
