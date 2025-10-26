#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QColorDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include "paletteeditor.h"



using RoleLabels = QList<QPair<QPalette::ColorRole, QString>>;
Q_GLOBAL_STATIC_WITH_ARGS(RoleLabels, roleText, ({
    { QPalette::WindowText, QObject::tr("Window text") },
    { QPalette::Button, QObject::tr("Button") },
    { QPalette::Light, QObject::tr("Light") },
    { QPalette::Midlight, QObject::tr("Midlight") },
    { QPalette::Dark, QObject::tr("Dark") },
    { QPalette::Mid, QObject::tr("Mid") },
    { QPalette::Text, QObject::tr("Text") },
    { QPalette::BrightText, QObject::tr("Bright text") },
    { QPalette::ButtonText, QObject::tr("Button text") },
    { QPalette::Base, QObject::tr("Base") },
    { QPalette::Window, QObject::tr("Window") },
    { QPalette::Shadow, QObject::tr("Shadow") },
    { QPalette::Highlight, QObject::tr("Highlight") },
    { QPalette::HighlightedText, QObject::tr("Highlighted text") },
    { QPalette::Link, QObject::tr("Link") },
    { QPalette::LinkVisited, QObject::tr("Link (visited)") },
    { QPalette::AlternateBase, QObject::tr("Base (alternate)") },
    { QPalette::NoRole, QObject::tr("No role") },
    { QPalette::ToolTipBase, QObject::tr("Tooltip base") },
    { QPalette::ToolTipText, QObject::tr("Tooltip text") }
}));

using GroupLabels = QList<QPair<QPalette::ColorGroup, QString>>;
Q_GLOBAL_STATIC_WITH_ARGS(GroupLabels, groupText, ({
    { QPalette::Active, QObject::tr("Active") },
    { QPalette::Disabled, QObject::tr("Disabled") },
    { QPalette::Inactive, QObject::tr("Inactive") }
}));



PaletteBox::PaletteBox(QWidget *parent) : QWidget(parent), color(0,0,0)
{

}

QColor PaletteBox::value()
{
    return color;
}

void PaletteBox::setValue(const QColor &c)
{
    color = c;
    emit valueChanged(color);
    update();
}

void PaletteBox::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    p.fillRect(QRect(0,0,width(),height()), QBrush(color));
}

void PaletteBox::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        event->accept();
        QColor ret = QColorDialog::getColor(color, this);
        if (ret.isValid()) {
            setValue(ret);
            emit valueSelected(color);
        }
        return;
    }
    QWidget::mousePressEvent(event);
}



PaletteEditor::PaletteEditor(QWidget *parent) : QWidget(parent)
{
    system = QApplication::palette();
    auto makeBox = [this](ColorPair data) {
        PaletteBox *box = new PaletteBox;
        PaletteEditor *owner = this;
        connect(box, &PaletteBox::valueSelected,
                owner, [owner,data](QColor c) {
            owner->colorField_valueSelected(data,c);
        });
        boxes.insert(data, box);
        return box;
    };

    int col = 0;
    int row = 0;
    QGridLayout *layout = new QGridLayout();

    // First row - labels
    for (auto const &[colorGroup, colorGroupTitle] : *groupText)
        layout->addWidget(new QLabel(colorGroupTitle), row, 1 + col++);
    row++;

    // Middle rows - colorboxes
    for (auto const &[colorRole, colorRoleTitle] : *roleText) {
        col = 0;
        layout->addWidget(new QLabel(colorRoleTitle), row, col++);
        for (auto const &[colorGroup, colorGroupTitle] : *groupText)
            layout->addWidget(makeBox({colorGroup, colorRole}), row, col++);
        row++;
    }

    // Final row - generate buttons
    col = 0;
    layout->addWidget(new QLabel(tr("Generate")), row, col++);

    QPushButton *button;
    button = new QPushButton(tr("Button"), this);
    connect(button, &QPushButton::clicked,
            this, &PaletteEditor::generateButton_clicked);
    layout->addWidget(button, row, col++);

    button = new QPushButton(tr("Button && Window"), this);
    connect(button, &QPushButton::clicked,
            this, &PaletteEditor::generateButton_clicked);
    layout->addWidget(button, row, col++);

    button = new QPushButton(tr("Reset to System"), this);
    connect(button, &QPushButton::clicked,
            this, &PaletteEditor::resetPalette);
    layout->addWidget(button, row, col++);

    setLayout(layout);

    connect(this, &PaletteEditor::paletteChanged,
            this, &PaletteEditor::valueChanged);
}

QPalette PaletteEditor::palette()
{
    return selected;
}

QPalette PaletteEditor::systemPalette()
{
    return system;
}

QVariant PaletteEditor::variant()
{
    QVariant v = paletteToVariant(selected);
    return v;
}

QPalette PaletteEditor::variantToPalette(const QVariant &v)
{
    QPalette p = system;
    QVariantList array = v.toList();
    RoleLabels::ConstIterator it;
    QVariant defaultValue("#000000");
    int index = 0;
    for (it = roleText->constBegin(); it != roleText->constEnd(); it++) {
        QVariantList items = array.value(index++).toList();
        p.setColor(QPalette::Active, it->first, items.value(0, defaultValue).toString());
        p.setColor(QPalette::Disabled, it->first, items.value(1, defaultValue).toString());
        p.setColor(QPalette::Inactive, it->first, items.value(2, defaultValue).toString());
    }
    return p;
}

QVariant PaletteEditor::paletteToVariant(const QPalette &p)
{
    QVariantList array;
    RoleLabels::ConstIterator it;
    for (it = roleText->constBegin(); it != roleText->constEnd(); it++) {
        QVariantList items;
        items.append(p.color(QPalette::Active, it->first).name());
        items.append(p.color(QPalette::Disabled, it->first).name());
        items.append(p.color(QPalette::Inactive, it->first).name());
        array.append(QVariant(items));
    }
    return array;
}

void PaletteEditor::setPalette(const QPalette &pal)
{
    selected = pal;
    for (BoxMap::Iterator it = boxes.begin(); it != boxes.end(); it++)
        it.value()->setValue(selected.color(it.key().first, it.key().second));
    emit paletteChanged(selected);
}

void PaletteEditor::setVariant(const QVariant &value)
{
    setPalette(variantToPalette(value));
}

void PaletteEditor::resetPalette()
{
    setPalette(system);
}

void PaletteEditor::colorField_valueSelected(PaletteEditor::ColorPair entry, QColor color)
{
    selected.setColor(entry.first, entry.second, color);
    emit paletteChanged(selected);
}

void PaletteEditor::generateButton_clicked()
{
    setPalette(QPalette(selected.color(QPalette::Button)));
}

void PaletteEditor::generateButtonWindow_clicked()
{
    setPalette(QPalette(selected.color(QPalette::Button),
                        selected.color(QPalette::Window)));
}
