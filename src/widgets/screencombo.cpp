#include <QApplication>
#include <QScreen>
#include "platform/unify.h"
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

void ScreenCombo::qScreen_geometryChanged()
{
    // FIXME: update the geometry on Windows without losing the selection!
    QString oldName = currentScreenName();
    populateItems();
    setCurrentScreenName(oldName);
}

void ScreenCombo::populateItems()
{
    this->clear();
    this->addItem(tr("Current"));
    for (auto screen : QApplication::screens()) {
        this->addItem(Helpers::screenToVisualName(screen));
        if (!Platform::isUnix) {
            QObject::disconnect(screen, &QScreen::geometryChanged,
                                this, &ScreenCombo::qScreen_geometryChanged);
            QObject::connect(screen, &QScreen::geometryChanged,
                             this, &ScreenCombo::qScreen_geometryChanged);
        }
    }
}
