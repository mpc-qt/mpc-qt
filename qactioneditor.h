#ifndef QACTIONEDITOR_H
#define QACTIONEDITOR_H

#include <QTableWidget>
#include <QStyledItemDelegate>
#include <QStandardItemModel>
#include <QList>
#include <QAction>
#include "helpers.h"

class QActionEditor : public QTableView
{
    Q_OBJECT
public:
    QActionEditor(QWidget *parent = 0);

    void setCommands(const QList<Command> &commands);
    Command getCommand(int index) const;
    void setCommand(int index, const Command &c);
    void updateActions();

    QVariantMap toVMap() const;
    void fromVMap(QVariantMap &map);

signals:
    void mouseFullscreenMap(MouseStateMap map);
    void mouseWindowedMap(MouseStateMap map);

private:
    QStandardItemModel model;
    QList<Command> commands;
};



class ShortcutDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ShortcutDelegate(QObject *parent = 0);
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QActionEditor *owner;
};



class QComboBox;
class QToolButton;
class ButtonWidget : public QWidget {
    Q_OBJECT
public:
    ButtonWidget(QWidget * parent = 0);
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
    QToolButton *button;
    QToolButton *keyMod;
    QToolButton *press;
    MouseState state_;
};



class ButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    ButtonDelegate(QObject *parent = 0, bool fullscreen = false);
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    QActionEditor *owner;
    bool fullscreen;
};

#endif // QACTIONEDITOR_H
