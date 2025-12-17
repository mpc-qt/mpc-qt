#ifndef QACTIONEDITOR_H
#define QACTIONEDITOR_H

#include <QTableWidget>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QList>
#include <QAction>
#include "helpers.h"



class QKeySequenceEdit;
class QToolButton;
class QComboBox;



class ActionEditor : public QTableView
{
    Q_OBJECT
public:
    explicit ActionEditor(QWidget *parent = nullptr);

    void setCommands(const QList<Command> &commands);
    Command getCommand(int index) const;
    void setCommand(int index, const Command &c);
    void updateActions();

    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &map);

private:
    QString getDescriptiveName(const QAction *action);

signals:
    void mouseFullscreenMap(MouseStateMap map);
    void mouseWindowedMap(MouseStateMap map);

public slots:
    void filterActions(const QString &text);

private:
    QStandardItemModel model;
    QList<Command> commands;
};



class ShortcutWidget :public QWidget {
    Q_OBJECT
public:
    explicit ShortcutWidget(QWidget *parent = nullptr);
    void setKeySequence(const QKeySequence &keySequence);
    QKeySequence keySequence();

signals:
    void finishedEditing(ShortcutWidget *editor);

private slots:
    void keyClear_clicked();
    void editingFinished();

private:
    QKeySequenceEdit *keyEditor = nullptr;
    QToolButton *keyClear = nullptr;
};



class ShortcutDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ShortcutDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    ActionEditor *owner = nullptr;
};




class ButtonWidget : public QWidget {
    Q_OBJECT
public:
    explicit ButtonWidget(QWidget * parent = nullptr);
    void setState(const MouseState &state);
    MouseState state() const;

private slots:
    void buttonMenu_selected(int item);
    void keyModMenu_selected(int item, bool yes);
    void pressMenu_selected(int item);

private:
    QList<QAction*> buttonActions;
    QList<QAction*> keyModActions;
    QList<QAction*> pressActions;
    QToolButton *button = nullptr;
    QToolButton *keyMod = nullptr;
    QToolButton *press = nullptr;
    MouseState state_;
};



class ButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ButtonDelegate(QObject *parent = nullptr, bool fullscreen = false);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    ActionEditor *owner = nullptr;
    bool fullscreen = false;
};

#endif // QACTIONEDITOR_H
