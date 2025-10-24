#ifndef SCREENCOMBO_H
#define SCREENCOMBO_H
#include <QComboBox>

class ScreenCombo : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY (QString currentScreenName READ currentScreenName WRITE setCurrentScreenName)
public:
    explicit ScreenCombo(QWidget *parent);
    QString currentScreenName();
    void setCurrentScreenName(QString name);

private slots:
    void qApp_screensChanged();
    void qScreen_geometryChanged();

private:
    void populateItems();
};

#endif // SCREENCOMBO_H
