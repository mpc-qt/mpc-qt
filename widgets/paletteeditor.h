#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMap>
#include <QWidget>
#include <QVariant>

class PaletteBox : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor value READ value WRITE setValue NOTIFY valueChanged)
public:
    explicit PaletteBox(QWidget *parent = nullptr);
    QColor value();

signals:
    void valueChanged(QColor c);
    void valueSelected(QColor c);

public slots:
    void setValue(const QColor &c);

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);

private:
    QColor color;
};


class PaletteEditor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QPalette palette READ palette WRITE setPalette NOTIFY paletteChanged RESET resetPalette)
    Q_PROPERTY(QVariant value READ variant WRITE setVariant NOTIFY valueChanged)

public:
    typedef QPair<QPalette::ColorGroup,QPalette::ColorRole> ColorPair;
    typedef QMap<ColorPair,PaletteBox*> BoxMap;
    typedef QMap<ColorPair,QColor> PaletteEntries;

    explicit PaletteEditor(QWidget *parent = nullptr);
    QPalette palette();
    QPalette systemPalette();

    // custom variant et al routines are needed for json serialisation
    QVariant variant();
    QPalette variantToPalette(const QVariant &v);
    QVariant paletteToVariant(const QPalette &p);

signals:
    void paletteChanged(QPalette pal);
    void valueChanged();

public slots:
    void setPalette(const QPalette &pal);
    void setVariant(const QVariant &value);
    void resetPalette();

private slots:
    void colorField_valueSelected(PaletteEditor::ColorPair entry, QColor color);
    void generateButton_clicked();
    void generateButtonWindow_clicked();

private:
    QPalette system;
    QPalette selected;
    BoxMap boxes;
};

#endif // PALETTEEDITOR_H
