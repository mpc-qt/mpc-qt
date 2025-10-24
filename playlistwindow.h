#ifndef PLAYLISTWINDOW_H
#define PLAYLISTWINDOW_H

#include <QDockWidget>
#include <QHash>
#include <QUuid>
#include "helpers.h"
#include "playlist.h"

namespace Ui {
class PlaylistWindow;
}

class DrawnPlaylist;
class PlaylistSelection;
class QThread;
class PlaylistSearcher;
class PlaylistWindow : public QDockWidget
{
    Q_OBJECT

public:
    explicit PlaylistWindow(QWidget *parent = nullptr);
    ~PlaylistWindow();

    void setCurrentPlaylist(QUuid what);
    void clearPlaylist(QUuid what);
    PlaylistItem addToPlaylist(const QUuid &playlist, const QList<QUrl> &what);
    PlaylistItem addToCurrentPlaylist(QList<QUrl> what);
    PlaylistItem urlToQuickPlaylist(QUrl what, bool appendToPlaylist);
    bool isCurrentPlaylistEmpty();
    bool isPlaylistSingularFile(QUuid list);
    bool isPlaylistRepeat(QUuid list);
    bool isPlaylistShuffle(QUuid list);
    PlaylistItem getFirstItem(QUuid list);
    PlaylistItem getItemAfter(QUuid list, QUuid item);
    QUuid getItemBefore(QUuid list, QUuid item);
    QUrl getUrlOf(QUuid list, QUuid item);
    QUrl getUrlOfFirst(QUuid list);
    void setMetadata(QUuid list, QUuid item, const QVariantMap &map);
    void replaceItem(QUuid list, QUuid item, const QList<QUrl> &urls);
    int extraPlayTimes(QUuid list, QUuid item);
    void setExtraPlayTimes(QUuid list, QUuid item, int amount);
    void deltaExtraPlayTimes(QUuid list, QUuid item, int delta);
    void reshufflePlaylist(const QUuid &playlistUuid);

    QVariantList tabsToVList(bool saveQuickPlaylist) const;
    void tabsFromVList(const QVariantList &qvl);

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void setupIconThemer();
    void connectButtonsToActions();
    void connectSignalsToSlots();

    DrawnPlaylist *currentPlaylistWidget();
    void updateCurrentPlaylist();
    void updatePlaylistHasItems();
    void setPlaylistFilters(QString filterText);
    void addNewTab(QUuid playlist, QString title);
    void addQuickQueue();

signals:
    void windowDocked();
    void viewActionChanged(bool visible);
    void currentPlaylistHasItems(bool yes);
    void itemDesired(QUuid playlistUuid, QUuid itemUuid, bool clickedInPlaylist);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);
    void quickQueueMode(bool yes);
    void playlistAddItem(QUuid playlistUUid);
    void playlistAddFolder();
    void playlistRepeatChanged(QUuid playlistUuid, bool repeat);
    void playlistShuffleChanged(QUuid playlistUuid, bool shuffle);
    void hideFullscreenChanged(bool checked);
    void playlistsBackupRequested();
    void playlistMovedToBackup(QUuid backupUuid);

public slots:
    void setIconTheme(IconThemer::FolderMode folderMode, const QString &fallbackFolder, const QString &customFolder);
    void setHideFullscreen(bool hidden);
    void setRememberSelectedPlaylist(bool remember);

    bool activateItem(QUuid playlistUuid, QUuid itemUuid, bool clickedInPlaylist = false);
    void changePlaylistSelection(QUrl itemUrl, QUuid playlistUuid, QUuid itemUuid, bool clickedInPlaylist);
    void addSimplePlaylist(QStringList data);
    void addPlaylistByUuid(QUuid playlistUuid);
    void setDisplayFormatSpecifier(QString fmt);
    void removePlaylistItem(const QUuid &itemUuid);
    void dockLocationMaybeChanged();

    void newTab();
    void closeTab();
    void duplicateTab();
    void importTab();
    void exportTab();

    void copy();
    void copyQueue();
    void paste();
    void pasteQueue();

    void playCurrentItem();
    bool playActiveItem();
    void selectNext();
    void selectPrevious();

    void incExtraPlayTimes();
    void decExtraPlayTimes();
    void zeroExtraPlayTimes();

    void activateNext();
    void activatePrevious();

    void quickQueue();
    void visibleToQueue();
    void setQueueMode(bool yes);

    void revealSearch();
    void finishSearch();

private slots:
    void savePlaylist(const QUuid &playlistUuid);
    void sortPlaylistByLabel(const QUuid &playlistUuid);
    void sortPlaylistByUrl(const QUuid &playlistUuid);
    void shufflePlaylist(const QUuid &playlistUuid, bool shuffle);
    void refreshPlaylist(const QUuid &playlistUuid);
    void restorePlaylist(const QUuid &playlistUuid);

    void self_visibilityChanged();
    void self_dockLocationChanged(Qt::DockWidgetArea area);
    void playlist_removeItemRequested();
    void playlist_removeAllRequested();
    void playlist_copySelectionToClipboard(const QUuid &playlistUuid);
    void playlist_hideOnFullscreenToggled(bool checked);
    void playlist_contextMenuRequested(const QPoint &p, const QUuid &playlistUuid, const QUuid &itemUuid);

    void on_tabWidget_tabCloseRequested(int index);

    void on_tabWidget_tabBarDoubleClicked(int index);

    void on_tabWidget_customContextMenuRequested(const QPoint &pos);

    void on_searchField_textEdited(const QString &arg1);

    void on_tabWidget_currentChanged(int index);

    void on_searchField_returnPressed();

private:
    Ui::PlaylistWindow *ui = nullptr;
    IconThemer themer;
    QUuid currentPlaylist;
    DisplayParser displayParser;
    bool showSearch = false;
    bool hideFullscreen = false;
    bool rememberSelectedPlaylist = false;

    QHash<QUuid, DrawnPlaylist*> widgets;
    DrawnPlaylist* queueWidget = nullptr;
    PlaylistSelection *clipboard = nullptr;
};

#endif // PLAYLISTWINDOW_H
