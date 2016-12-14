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
class PlaylistSelection;
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
    QPair<QUuid, QUuid> getItemAfter(QUuid list, QUuid item);
    QUuid getItemBefore(QUuid list, QUuid item);
    QUrl getUrlOf(QUuid list, QUuid item);
    void setMetadata(QUuid list, QUuid item, const QVariantMap &map);
    void replaceItem(QUuid list, QUuid item, const QList<QUrl> &urls);
    int extraPlayTimes(QUuid list, QUuid item);
    void setExtraPlayTimes(QUuid list, QUuid item, int amount);

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
    void addQuickQueue();

signals:
    void windowDocked();
    void itemDesired(QUuid playlistUuid, QUuid itemUuid);
    void importPlaylist(QString fname);
    void exportPlaylist(QString fname, QStringList items);
    void quickQueueMode(bool yes);

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
    void self_visibilityChanged();

    void self_dockLocationChanged(Qt::DockWidgetArea area);

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
    QDrawnPlaylist* queueWidget;
    PlaylistSelection *clipboard;
};

#endif // PLAYLISTWINDOW_H
