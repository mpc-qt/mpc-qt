#include <QHBoxLayout>
#include <QKeySequence>
#include <QKeySequenceEdit>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <cmath>
#include "qactioneditor.h"

QActionEditor::QActionEditor(QWidget *parent) :
    QTableView(parent)
{
    model.setHorizontalHeaderLabels({tr("Command"), tr("Key"),
                                     tr("Mouse Window"),
                                     tr("Mouse Fullscr")});
    setModel(&model);
    setItemDelegateForColumn(1, new ShortcutDelegate(this));
    setItemDelegateForColumn(2, new ButtonDelegate(this, false));
    setItemDelegateForColumn(3, new ButtonDelegate(this, true));
    setColumnWidth(0, sizeHintForColumn(0));
    setColumnWidth(1, sizeHintForColumn(1));
    setColumnWidth(2, sizeHintForColumn(2));
    setColumnWidth(3, sizeHintForColumn(3));
    setEditTriggers(QAbstractItemView::AllEditTriggers);
}

int QActionEditor::sizeHintForColumn(int column) const
{
    // TL note: not actually the widest, but unobnoxious
    QStringList maxWidthStrings {
        tr("Open Network Stream..."),
        tr("Alt+Ctrl+Backspace"),
        tr("Mouse Window"),
        tr("Mouse Fullscr")
    };
    QString maxWidthString = maxWidthStrings.value(column, "...");
    return fontMetrics().boundingRect("_" + maxWidthString + "_").width();
}

void QActionEditor::setCommands(const QList<Command> &commands)
{
    model.setRowCount(0);

    for (int i = 0; i < commands.count(); i++) {
        QList<QStandardItem*> items = {
            new QStandardItem(commands[i].action->text().replace('&',"")),
            new QStandardItem(commands[i].action->shortcut().toString()),
            new QStandardItem(commands[i].mouseWindowed.toString()),
            new QStandardItem(commands[i].mouseFullscreen.toString()),
        };
        items[0]->setEditable(false);
        for (auto &index : items) {
            index->setData(i);
        }
        model.appendRow(items);
    }
    this->commands = commands;
}

Command QActionEditor::getCommand(int index) const
{
    return commands.value(index);
}

void QActionEditor::setCommand(int index, const Command &c)
{
    if (index >= 0 && index < commands.count())
        commands[index] = c;

    for (int i = 0; i < commands.count(); i++) {
        if (i == index)
            continue;
        Command &other = commands[i];
        if (!!other.mouseFullscreen && other.mouseFullscreen == c.mouseFullscreen) {
            model.setData(model.index(i, 3), "None");
            other.mouseFullscreen = MouseState();
        }
        if (!!other.mouseWindowed && other.mouseWindowed == c.mouseWindowed) {
            model.setData(model.index(i, 2), "None");
            other.mouseWindowed = MouseState();
        }
        if (!other.keys.isEmpty() && other.keys.matches(c.keys) == QKeySequence::ExactMatch) {
            model.setData(model.index(i, 1), "");
            other.keys = QKeySequence();
        }
    }
}

void QActionEditor::updateActions()
{
    MouseStateMap fullscreen, windowed;
    for (const Command &c : commands) {
        c.action->setShortcut(c.keys);
        if (!!c.mouseFullscreen)
            fullscreen[c.mouseFullscreen] = c.action;
        if (!!c.mouseWindowed)
            windowed[c.mouseWindowed] = c.action;
    }
    emit mouseFullscreenMap(fullscreen);
    emit mouseWindowedMap(windowed);
}

QVariantMap QActionEditor::toVMap() const
{
    QVariantMap map;
    for (const Command &c : commands) {
        map[c.action->objectName()] = c.toVMap();
    }
    return map;
}

void QActionEditor::fromVMap(const QVariantMap &map)
{
    QMap<QString, int> nameToIndex;
    for (int i = 0; i < commands.count(); i++)
        nameToIndex[commands[i].action->objectName()] = i;
    for (auto it = map.begin(); it != map.end(); it++) {
        int index = nameToIndex[it.key()];
        Command c = commands.value(index);
        c.fromVMap(it.value().toMap());
        commands[index] = c;
    }
    setCommands(commands);
}

ShortcutDelegate::ShortcutDelegate(QObject *parent)
    : QStyledItemDelegate(parent), owner((QActionEditor*)parent)
{

}

QWidget *ShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    ShortcutWidget *editor = new ShortcutWidget(parent);
    return (QWidget*)editor;
}

void ShortcutDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ShortcutWidget *keyEditor = static_cast<ShortcutWidget*>(editor);
    keyEditor->setKeySequence(QKeySequence(index.data(Qt::EditRole).toString()));
}

void ShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ShortcutWidget *keyEditor = static_cast<ShortcutWidget*>(editor);
    QKeySequence seq = keyEditor->keySequence();
    Command c = owner->getCommand(index.row());
    c.keys = seq;
    owner->setCommand(index.row(), c);
    model->setData(index, keyEditor->keySequence());
}

void ShortcutDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}



ButtonDelegate::ButtonDelegate(QObject *parent, bool fullscreen)
    : owner((QActionEditor*)parent), fullscreen(fullscreen)
{
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return (QWidget*)new ButtonWidget(parent);
}

void ButtonDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ButtonWidget *buttons = static_cast<ButtonWidget*>(editor);
    Command c = owner->getCommand(index.row());
    buttons->setState(fullscreen ? c.mouseFullscreen : c.mouseWindowed);
}

void ButtonDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ButtonWidget *buttons = static_cast<ButtonWidget*>(editor);
    Command c = owner->getCommand(index.row());
    MouseState state = buttons->state();
    if (fullscreen)
        c.mouseFullscreen = state;
    else
        c.mouseWindowed = state;
    owner->setCommand(index.row(), c);

    model->setData(index, state.toString());
}

void ButtonDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    QRect rc = option.rect;
    QSize sz = editor->minimumSizeHint();
    if (rc.width() < sz.width())  rc.setWidth(sz.width());
    editor->setGeometry(rc);
}



ButtonWidget::ButtonWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout;
    button = new QToolButton(this);
    button->setText(tr("B"));
    button->setPopupMode(QToolButton::InstantPopup);
    QMenu *menu = new QMenu;
    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);
    for (int i = 0; i < MouseState::buttonToTextCount(); i++) {
        QAction *a = new QAction( MouseState::buttonToText(i), this);
        connect(a, &QAction::triggered, [i, this]() {
            buttonMenu_selected(i);
        });
        a->setCheckable(true);
        group->addAction(a);
        menu->addAction(a);
        buttonActions.append(a);
    }
    button->setMenu(menu);
    layout->addWidget(button);

    keyMod = new QToolButton(this);
    keyMod->setText(tr("K"));
    keyMod->setPopupMode(QToolButton::InstantPopup);
    menu = new QMenu;
    for (int i = 0; i < MouseState::modToTextCount(); i++) {
        QAction *a = new QAction(MouseState::modToText(i), this);
        connect(a, &QAction::toggled, [i, this](bool yes) {
            keyModMenu_selected(i, yes);
        });
        a->setCheckable(true);
        menu->addAction(a);
        keyModActions.append(a);
    }
    keyMod->setMenu(menu);
    layout->addWidget(keyMod);


    press = new QToolButton(this);
    press->setText(tr("↑↓"));
    press->setPopupMode(QToolButton::InstantPopup);
    menu = new QMenu;
    group = new QActionGroup(this);
    group->setExclusive(true);
    for (int i = 0; i < MouseState::pressToTextCount(); i++) {
        QAction *a = new QAction(MouseState::pressToText(i), this);
        connect(a, &QAction::triggered, [i, this]() {
            pressMenu_selected(i);
        });
        a->setCheckable(true);
        group->addAction(a);
        menu->addAction(a);
        pressActions.append(a);
    }
    press->setMenu(menu);
    layout->addWidget(press);

    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
}

void ButtonWidget::setState(const MouseState &state)
{
    state_ = state;
    buttonActions[state.button]->setChecked(true);
    keyModActions[0]->setChecked(state.mod & 1);
    keyModActions[1]->setChecked(state.mod & 2);
    keyModActions[2]->setChecked(state.mod & 4);
    keyModActions[3]->setChecked(state.mod & 8);
    pressActions[state.press]->setChecked(true);
}

MouseState ButtonWidget::state() const
{
    return state_;
}

void ButtonWidget::buttonMenu_selected(int item)
{
    state_.button = item;
}

void ButtonWidget::keyModMenu_selected(int item, bool yes)
{
    if (yes)
        state_.mod |= (1 << item);
    else
        state_.mod &= ~(1 << item);
}

void ButtonWidget::pressMenu_selected(int item)
{
    state_.press = static_cast<MouseState::MousePress>(item);
}



ShortcutWidget::ShortcutWidget(QWidget *parent) : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout();
    keyEditor = new QKeySequenceEdit(this);
    layout->addWidget(keyEditor, 1);

    keyClear = new QToolButton(this);
    keyClear->setText("<");
    layout->addWidget(keyClear, 0);

    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(keyClear, &QToolButton::clicked,
            this, &ShortcutWidget::keyClear_clicked);
}

void ShortcutWidget::setKeySequence(const QKeySequence &keySequence)
{
    keyEditor->setKeySequence(keySequence);
}

QKeySequence ShortcutWidget::keySequence()
{
    return keyEditor->keySequence();
}

void ShortcutWidget::keyClear_clicked()
{
    keyEditor->setKeySequence(QKeySequence());
}
