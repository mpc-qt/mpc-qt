#include <QApplication>
#include <QScreen>
#include "helpers.h"
#include "screencombo.h"

ScreenCombo::ScreenCombo(QWidget *parent) : QComboBox(parent)
{
    populateItems();
    connect(qApp, &QApplication::screenAdded, this, &ScreenCombo::qApp_screensChanged);
    connect(qApp, &QApplication::screenRemoved, this, &ScreenCombo::qApp_screensChanged);
}

QString ScreenCombo::currentScreenName()
{
    if (currentIndex() < 1)
        return QString();
    return currentText();
}

void ScreenCombo::setCurrentScreenName(QString name)
{
    int c = count();
    for (int i = 1; i < c; i++) {
        if (itemText(i) == name) {
            setCurrentIndex(i);
            return;
        }
    }
    setCurrentIndex(0);
}

void ScreenCombo::qApp_screensChanged()
{
    QString oldName = currentScreenName();
    populateItems();
    setCurrentScreenName(oldName);
}

void ScreenCombo::populateItems()
{
    this->clear();
    this->addItem(tr("Current"));
    for (auto screen : qApp->screens()) {
        this->addItem(Helpers::screenToVisualName(screen));
    }
}
