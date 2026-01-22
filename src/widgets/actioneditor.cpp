#include <QActionGroup>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QToolButton>
#include <QMenu>
#include <QMessageBox>
#include "actioneditor.h"
#include "logger.h"

static constexpr char logModule[] =  "action";

ActionEditor::ActionEditor(QWidget *parent) :
    QTableView(parent)
{
    model.setHorizontalHeaderLabels({tr("Command"), tr("Key"),
                                     tr("Mouse Window"),
                                     tr("Mouse Fullscreen")});
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

void ActionEditor::filterActions(const QString &text)
{
    if (text.isEmpty()) {
        for (int i = 0; i < model.rowCount(); ++i) {
            setRowHidden(i, false);
        }
        return;
    }

    for (int i = 0; i < model.rowCount(); ++i) {
        bool match = false;
        for (int j = 0; j < model.columnCount(); ++j) {
            QString keyText = model.item(i, j)->text().toLower();
            if (keyText.contains(text.toLower()))
                match = true;
        }
        setRowHidden(i, !match);
    }
}

void ActionEditor::setCommands(const QList<Command> &commands)
{
    model.setRowCount(0);

    for (int i = 0; i < commands.count(); i++) {
        QString actionDescription = getDescriptiveName(commands[i].action);
        QList<QStandardItem*> items = {
            new QStandardItem(actionDescription),
            new QStandardItem(commands[i].action->shortcut().toString(QKeySequence::NativeText)),
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

QString ActionEditor::getDescriptiveName(const QAction *action)
{
    QHash<QString, QString> nameToDescription = {
        { "actionPlayPause",               tr("Play / Pause") },
        { "actionPlayVolumeUp",            tr("Volume Increase") },
        { "actionPlayVolumeDown",          tr("Volume Decrease") },
        { "actionPlayVolumeMute",          tr("Volume Mute") },
        { "actionPlayAfterOnceExit",       tr("After Playback: Exit") },
        { "actionPlayAfterOnceStandby",    tr("After Playback: Stand by") },
        { "actionPlayAfterOnceHibernate",  tr("After Playback: Hibernate") },
        { "actionPlayAfterOnceShutdown",   tr("After Playback: Shutdown") },
        { "actionPlayAfterOnceLogoff",     tr("After Playback: Log Off") },
        { "actionPlayAfterOnceLock",       tr("After Playback: Lock") },
        { "actionPlayAfterAlwaysNext",     tr("After Playback: Play next file") },
        { "actionPlayAfterOnceNothing",    tr("After Playback: Do nothing") },
        { "actionViewOntopDefault",        tr("On Top: Default") },
        { "actionViewOntopAlways",         tr("On Top: Always") },
        { "actionViewOntopPlaying",        tr("On Top: While Playing") },
        { "actionViewOntopVideo",          tr("On Top: While Playing Video") },
        { "actionPlaylistExtraIncrement",  tr("Extra Play Times: Increment") },
        { "actionPlaylistExtraDecrement",  tr("Extra Play Times: Decrement") },
        { "actionMoveSubtitlesUp",         tr("Move Subtitles Up") },
        { "actionMoveSubtitlesDown",       tr("Move Subtitles Down") },
        { "action43VideoAspect",           tr("4:3 Aspect ratio") },
        { "actionDecreaseVideoAspect",     tr("Decrease Aspect ratio") },
        { "actionIncreaseVideoAspect",     tr("Increase Aspect ratio") },
        { "actionResetVideoAspect",        tr("Reset Aspect ratio") },
        { "actionDisableVideoAspect",      tr("Disable Aspect ratio") },
        { "actionDecreasePanScan",         tr("Decrease Pan and Scan") },
        { "actionIncreasePanScan",         tr("Increase Pan and Scan") },
        { "actionMinPanScan",              tr("Minimum Pan and Scan") },
        { "actionMaxPanScan",              tr("Maximum Pan and Scan") },
        { "actionDecreaseZoom",            tr("Decrease Zoom") },
        { "actionIncreaseZoom",            tr("Increase Zoom") },
        { "actionResetZoom",               tr("Reset Zoom") },
        { "actionResetWidthHeight",        tr("Reset Resize") },
        { "actionMoveLeft",                tr("Move Left") },
        { "actionMoveRight",               tr("Move Right") },
        { "actionMoveUp",                  tr("Move Up") },
        { "actionMoveDown",                tr("Move Down") },
        { "actionResetMove",               tr("Reset Move") },
        { "actionRotateClockwise",         tr("Rotate Clockwise") },
        { "actionRotateCounterclockwise",  tr("Rotate Counterclockwise") },
        { "actionResetRotate",             tr("Reset Rotate") }
    };

    QString actionDescription;
    auto it = nameToDescription.find(action->objectName());
    if (it != nameToDescription.end())
        actionDescription = it.value();
    else
        actionDescription = action->text().replace('&',"");

    return actionDescription;
}

Command ActionEditor::getCommand(int index) const
{
    return commands.value(index);
}

bool ActionEditor::setCommand(int index, const Command &c)
{
    bool replace = true;
    auto shouldReplace = [&](const QString &shortcut,
                            const QString &oldCommand,
                            const QString &newCommand)
    {
        auto answer = QMessageBox::question(
            this,
            tr("Shortcut already used"),
            QString(
                tr("\"%1\" is already used by \"%2\".\n"
                "Do you want to use it for \"%3\" instead?")
            )
                .arg(shortcut)
                .arg(oldCommand)
                .arg(newCommand),
            QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No),
            QMessageBox::No
        );

        return answer == QMessageBox::Yes;
    };

    for (int i = 0; i < commands.count(); i++) {
        if (i == index)
            continue;
        Command &other = commands[i];
        if (!!other.mouseFullscreen && other.mouseFullscreen == c.mouseFullscreen) {
            replace = replace && shouldReplace(c.mouseFullscreen.toString(), getDescriptiveName(other.action), getDescriptiveName(c.action));
            if (replace) {
                model.setData(model.index(i, 3), "None");
                other.mouseFullscreen = MouseState();
            }
        }
        if (!!other.mouseWindowed && other.mouseWindowed == c.mouseWindowed) {
            replace = replace && shouldReplace(c.mouseWindowed.toString(), getDescriptiveName(other.action), getDescriptiveName(c.action));
            if (replace) {
                model.setData(model.index(i, 2), "None");
                other.mouseWindowed = MouseState();
            }
        }
        if (!other.keys.isEmpty() && other.keys.matches(c.keys) == QKeySequence::ExactMatch) {
            replace = replace && shouldReplace(c.keys.toString(), getDescriptiveName(other.action), getDescriptiveName(c.action));
            if (replace) {
                model.setData(model.index(i, 1), "");
                other.keys = QKeySequence();
            }
        }
    }
    if (replace && index >= 0 && index < commands.count())
        commands[index] = c;
    return replace;
}

void ActionEditor::updateActions()
{
    MouseStateMap fullscreen, windowed;
    for (const Command &c : std::as_const(commands)) {
        c.action->setShortcut(c.keys);
        if (!!c.mouseFullscreen)
            fullscreen[c.mouseFullscreen] = c.action;
        if (!!c.mouseWindowed)
            windowed[c.mouseWindowed] = c.action;
    }
    emit mouseFullscreenMap(fullscreen);
    emit mouseWindowedMap(windowed);
}

QVariantMap ActionEditor::toVMap() const
{
    QVariantMap map;
    for (const Command &c : commands) {
        map[c.action->objectName()] = c.toVMap();
    }
    return map;
}

void ActionEditor::fromVMap(const QVariantMap &map)
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
    : QStyledItemDelegate(parent), owner(static_cast<ActionEditor*>(parent))
{

}

QWidget *ShortcutDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    ShortcutWidget *editor = new ShortcutWidget(parent);
    connect(editor, &ShortcutWidget::finishedEditing,
            this, &ShortcutDelegate::commitData);

    return editor;
}

void ShortcutDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ShortcutWidget *keyEditor = static_cast<ShortcutWidget*>(editor);
    keyEditor->setKeySequence(QKeySequence(index.data(Qt::EditRole).toString()));
}

void ShortcutDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ShortcutWidget *keyEditor = static_cast<ShortcutWidget*>(editor);
    editor->close();
    QKeySequence seq = keyEditor->keySequence();
    Command c = owner->getCommand(index.row());
    c.keys = seq;
    if (owner->setCommand(index.row(), c))
        model->setData(index, keyEditor->keySequence());
}

void ShortcutDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}



ButtonDelegate::ButtonDelegate(QObject *parent, bool fullscreen)
    : owner(qobject_cast<ActionEditor*>(parent)), fullscreen(fullscreen)
{
}

QWidget *ButtonDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return new ButtonWidget(parent);
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
    if (owner->setCommand(index.row(), c))
        model->setData(index, state.toString());
}

void ButtonDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)
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
        connect(a, &QAction::triggered, this, [i, this]() {
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
        connect(a, &QAction::toggled, this, [i, this](bool yes) {
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
        connect(a, &QAction::triggered, this, [i, this]() {
            pressMenu_selected(i);
        });
        a->setCheckable(true);
        group->addAction(a);
        menu->addAction(a);
        pressActions.append(a);
    }
    press->setMenu(menu);
    layout->addWidget(press);

    layout->setContentsMargins(0,0,0,0);
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

    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    setLayout(layout);
    keyEditor->grabKeyboard();

    connect(keyClear, &QToolButton::clicked,
            this, &ShortcutWidget::keyClear_clicked);
    connect(keyEditor, &QKeySequenceEdit::editingFinished,
            this, &ShortcutWidget::editingFinished);
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
    keyEditor->clear();
    keyEditor->setFocus();
}

void ShortcutWidget::editingFinished()
{
    keyEditor->releaseKeyboard();
    emit finishedEditing(this);
}
