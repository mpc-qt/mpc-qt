#ifndef PLAYLISTWINDOW_H
#define PLAYLISTWINDOW_H

#include <QDockWidget>
#include <QHash>
#include <QUuid>
#include "helpers.h"

namespace Ui {
class PlaylistWindow;
}

class QDrawnPlaylist;
class QThread;
class PlaylistSearcher;
class PlaylistWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistWindow(QWidget *parent = 0);
    ~PlaylistWindow();

    void setCurrentPlaylist(QUuid what);
    void clearPlaylist(QUuid what);
    QPair<QUuid, QUuid> addToCurrentPlaylist(QList<QUrl> what);
    QPair<QUuid, QUuid> urlToQuickPlaylist(QUrl what);
    bool isCurrentPlaylistEmpty();
    QUuid getItemAfter(QUuid list, QUuid item);
    QUuid getItemBefore(QUuid list, QUuid item);
    QUrl getUrlOf(QUuid list, QUuid item);
    void setMetadata(QUuid list, QUuid item, const QVariantMap &map);

    QVariantList tabsToVList() const;
    void tabsFromVList(const QVariantList &qvl);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void connectButtonsToActions();
    void connectSignalsToSlots();

    QDrawnPlaylist *currentPlaylistWidget();
    void updateCurrentPlaylist();
    void setPlaylistFilters(QString filterText);
    void addNewTab(QUuid playlist, QString title);

signals:
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);
    void relativeSeekRequested(bool fowards, bool small);

public slots:
    bool activateItem(QUuid playlistUuid, QUuid itemUuid);
    void changePlaylistSelection(QUrl itemUrl, QUuid playlistUuid, QUuid itemUuid);
    void addSimplePlaylist(QStringList data);
    void setDisplayFormatSpecifier(QString fmt);

    void newTab();
    void closeTab();
    void duplicateTab();
    void importTab();
    void exportTab();

    void playCurrentItem();
    bool playActiveItem();
    void selectNext();
    void selectPrevious();

    void activateNext();
    void activatePrevious();

    void quickQueue();
    void visibleToQueue();

    void revealSearch();
    void finishSearch();

private slots:
    void self_relativeSeekRequested(bool forwards, bool small);
    void self_visibilityChanged();

    void on_tabWidget_tabCloseRequested(int index);

    void on_tabWidget_tabBarDoubleClicked(int index);

    void on_tabWidget_customContextMenuRequested(const QPoint &pos);

    void on_searchField_textEdited(const QString &arg1);

    void on_tabWidget_currentChanged(int index);

    void on_searchField_returnPressed();

private:
    Ui::PlaylistWindow *ui;
    QUuid currentPlaylist;
    DisplayParser displayParser;
    bool showSearch;

    QHash<QUuid, QDrawnPlaylist*> widgets;
};

#endif // PLAYLISTWINDOW_H
