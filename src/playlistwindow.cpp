#include <QAction>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QGuiApplication>
#include <QMimeData>
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include "logger.h"
#include "playlistwindow.h"
#include "ui_playlistwindow.h"
#include "widgets/drawnplaylist.h"
#include "playlist.h"

static constexpr char logModule[] =  "playlistwindow";
static constexpr char keyCurrentPlaylist[] = "currentPlaylist";

PlaylistWindow::PlaylistWindow(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::PlaylistWindow),
    currentPlaylist()
{
    clipboard = new PlaylistSelection;

    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
    setObjectName("playlistWindow");
    setWindowTitle(tr("Playlist"));
    addNewTab(QUuid(), tr("Quick Playlist"));
    addQuickQueue();
    ui->searchHost->setVisible(false);
    ui->searchField->installEventFilter(this);

    setupIconThemer();
    connectSignalsToSlots();
}

PlaylistWindow::~PlaylistWindow()
{
    Logger::log(logModule, "~PlaylistWindow");
    delete ui;
    delete clipboard;
}

void PlaylistWindow::setCurrentPlaylist(QUuid what)
{
    if (widgets.contains(what)) {
        ui->tabWidget->setCurrentWidget(widgets[what]);
        currentPlaylist = what;
    }
    updateCurrentPlaylist();
}

void PlaylistWindow::clearPlaylist(QUuid what)
{
    if (widgets.contains(what))
        widgets[what]->removeAll();
    updatePlaylistHasItems();
}

PlaylistItem PlaylistWindow::addToPlaylist(const QUuid &playlist, const QList<QUrl> &what)
{
    QList<QUrl> filtered = Helpers::filterUrls(what);
    PlaylistItem firstPlaylistItem, currentItem;
    bool firstSet = false;
    auto qdp = widgets.contains(playlist) ? widgets.value(playlist) : widgets[QUuid()];
    for (QUrl const &url : filtered) {
        currentItem = qdp->importUrl(url);
        if (!firstSet) {
            firstPlaylistItem = currentItem;
            firstSet = true;
        }
    }
    updatePlaylistHasItems();
    return firstPlaylistItem;
}

PlaylistItem PlaylistWindow::addToCurrentPlaylist(QList<QUrl> what)
{
    return addToPlaylist(currentPlaylist, what);
}

PlaylistItem PlaylistWindow::urlToQuickPlaylist(QUrl what, bool appendToPlaylist)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(QUuid());
    if (!appendToPlaylist) {
        pl->clear();
        widgets[QUuid()]->clear();
    }
    ui->tabWidget->setCurrentWidget(widgets[QUuid()]);
    return addToCurrentPlaylist(QList<QUrl>() << what);
}

bool PlaylistWindow::isCurrentPlaylistEmpty()
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(currentPlaylist);
    return pl ? pl->isEmpty() : true;
}

bool PlaylistWindow::isPlaylistSingularFile(QUuid list)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl || pl->count() != 1)
        return false;
    auto item = pl->itemFirst();
    return item->url().isLocalFile();
}

bool PlaylistWindow::isPlaylistRepeat(QUuid list)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return false;
    return pl->repeat();
}

bool PlaylistWindow::isPlaylistShuffle(QUuid list)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return false;
    return pl->shuffle();
}

PlaylistItem PlaylistWindow::getFirstItem(QUuid list)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    auto item = pl->itemFirst();
    if (item.isNull())
        return PlaylistItem();
    return {item->playlistUuid(), item->uuid()};
}

PlaylistItem PlaylistWindow::getItemAfter(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return { QUuid(), QUuid() };
    auto qpl = PlaylistCollection::queuePlaylist();
    PlaylistItem next = qpl->takeFirst();
    if (!next.item.isNull())
        return next;
    QSharedPointer<Item> after;
    after = pl->itemAfter(item);
    if (!after)
        return { QUuid(), QUuid() };
    return { pl->uuid(), after->uuid() };
}

QUuid PlaylistWindow::getItemBefore(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return QUuid();
    QSharedPointer<Item> before = pl->itemBefore(item);
    if (!before)
        return QUuid();
    return before->uuid();
}

QUrl PlaylistWindow::getUrlOf(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return QUrl();
    auto i = pl->getItem(item);
    if (!i)
        return QUrl();
    return i->url();
}

QUrl PlaylistWindow::getUrlOfFirst(QUuid list)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    auto item = pl->itemFirst();
    if (item.isNull())
        return QUrl();
    return item->url();
}

void PlaylistWindow::setMetadata(QUuid list, QUuid item, const QVariantMap &map)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return;
    auto i = pl->getItem(item);
    if (!i)
        return;
    i->setMetadata(map);

    auto qdp = currentPlaylistWidget();
    if (qdp->uuid() == list)
        qdp->viewport()->update();

}

void PlaylistWindow::replaceItem(QUuid list, QUuid item, const QList<QUrl> &urls)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return;

    QList<QUrl> filtered = Helpers::filterUrls(urls);
    if (filtered.isEmpty()) {
        // FIXME: remove the item that cannot played
        return;
    }

    QList<QUuid> addedItems = pl->replaceItem(item, filtered);
    auto listWidget = widgets[list];
    if (listWidget)
        listWidget->addItemsAfter(item, addedItems);

    auto qdp = currentPlaylistWidget();
    if (qdp->uuid() == list)
        qdp->viewport()->update();

    updatePlaylistHasItems();
}

int PlaylistWindow::extraPlayTimes(QUuid list, QUuid item)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return -1;
    auto i = pl->getItem(item);
    return i ? i->extraPlayTimes() : -1;
}

void PlaylistWindow::setExtraPlayTimes(QUuid list, QUuid item, int amount)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return;
    auto i = pl->getItem(item);
    if (!i)
        return;
    i->setExtraPlayTimes(amount);
}

void PlaylistWindow::deltaExtraPlayTimes(QUuid list, QUuid item, int delta)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(list);
    if (!pl)
        return;
    auto i = pl->getItem(item);
    if (!i)
        return;
    i->deltaExtraPlayTimes(delta);
    widgets[list]->viewport()->repaint();
}

// Save the tabs on stop
QVariantList PlaylistWindow::tabsToVList(bool saveQuickPlaylist) const
{
    QVariantList qvl;
    for (int i = 0; i < ui->tabWidget->count(); i++) {
        auto widget = static_cast<DrawnPlaylist *>(ui->tabWidget->widget(i));
        if (!saveQuickPlaylist && widget->uuid().isNull())
            widget->removeAll();
        qvl.append(widget->toVMap());
    }
    qvl.append(QVariantMap{{keyCurrentPlaylist, currentPlaylist}});
    return qvl;
}

// Create the tabs on start
void PlaylistWindow::tabsFromVList(const QVariantList &qvl)
{
    ui->tabWidget->clear();
    widgets.clear();
    for (const QVariant &v : qvl) {
        QVariantMap qvm = v.toMap();
        if (qvm.contains(keyCurrentPlaylist)) {
            if (rememberSelectedPlaylist)
                setCurrentPlaylist(qvm[keyCurrentPlaylist].toUuid());
            continue;
        }
        auto qdp = new DrawnPlaylist(PlaylistCollection::getSingleton());
        qdp->setDisplayParser(&displayParser);
        qdp->fromVMap(qvm);
        connect(qdp, &DrawnPlaylist::itemDesiredByDoubleClick,
                this, &PlaylistWindow::itemDesired);
        connect(qdp, &DrawnPlaylist::contextMenuRequested,
                this, &PlaylistWindow::playlist_contextMenuRequested);
        auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
        if (pl->uuid().isNull())
            pl->setTitle(tr("Quick Playlist"));
        ui->tabWidget->addTab(qdp, pl->title());
        widgets.insert(pl->uuid(), qdp);
    }
    if (widgets.count() < 1)
        addNewTab(QUuid(), tr("Quick Playlist"));
    updatePlaylistHasItems();
}

bool PlaylistWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    if (obj == ui->searchField && event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        if (!keyEvent->modifiers() &&
                (keyEvent->key() == Qt::Key_Up ||
                 keyEvent->key() == Qt::Key_Down)) {
            if (keyEvent->key() == Qt::Key_Up)
                selectPrevious();
            else
                selectNext();
            return true;
        }
    }
    return QDockWidget::eventFilter(obj, event);
}

void PlaylistWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->accept();
}

void PlaylistWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;
    addToCurrentPlaylist(event->mimeData()->urls());
}

void PlaylistWindow::wheelEvent(QWheelEvent *event)
{
    // Don't pass scroll events up the chain.  They are used for e.g. tab
    // switching when over the tab bar and also scrolling the playlists.
    event->accept();
}

void PlaylistWindow::setupIconThemer()
{
    QVector<IconThemer::IconData> data {
        { ui->newTab, "tab-new", {} },
        { ui->closeTab, "tab-close", {} },
        { ui->duplicateTab, "tab-duplicate", {} },
        { ui->importList, "document-import", {} },
        { ui->exportList, "document-export", {} },
        { ui->visibleToQueue, "media-queue-visible", {} },
        { ui->showQueue, "view-media-queue", {} }
    };
    for (const auto &d : data)
        themer.addIconData(d);
}

void PlaylistWindow::connectSignalsToSlots()
{
    connect(this, &PlaylistWindow::visibilityChanged,
            this, &PlaylistWindow::self_visibilityChanged);
    connect(this, &PlaylistWindow::dockLocationChanged,
            this, &PlaylistWindow::self_dockLocationChanged);
    connect(this->toggleViewAction(), &QAction::toggled,
            this, &PlaylistWindow::viewActionChanged);

    connect(ui->newTab, &QPushButton::clicked,
            this, &PlaylistWindow::newTab);
    connect(ui->closeTab, &QPushButton::clicked,
            this, &PlaylistWindow::closeTab);
    connect(ui->duplicateTab, &QPushButton::clicked,
            this, &PlaylistWindow::duplicateTab);
    connect(ui->importList, &QPushButton::clicked,
            this, &PlaylistWindow::importTab);
    connect(ui->exportList, &QPushButton::clicked,
            this, &PlaylistWindow::exportTab);
    connect(ui->visibleToQueue, &QPushButton::clicked,
            this, &PlaylistWindow::visibleToQueue);
    connect(ui->showQueue, &QPushButton::clicked,
            this, &PlaylistWindow::setQueueMode);
}

DrawnPlaylist *PlaylistWindow::currentPlaylistWidget()
{
    return static_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
}

void PlaylistWindow::updateCurrentPlaylist()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
    currentPlaylist = qdp->uuid();
    setTabOrder(ui->tabWidget->focusProxy(), qdp);
    setTabOrder(qdp, ui->searchField);
    updatePlaylistHasItems();
}

void PlaylistWindow::updatePlaylistHasItems()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
    emit currentPlaylistHasItems(qdp->count() > 0);
}

void PlaylistWindow::setPlaylistFilters(QString filterText)
{
    for (auto &widget : widgets) {
        widget->setFilter(filterText);
    }
    queueWidget->setFilter(filterText);
}

void PlaylistWindow::addNewTab(QUuid playlist, QString title)
{
    auto qdp = new DrawnPlaylist(PlaylistCollection::getSingleton());
    qdp->setDisplayParser(&displayParser);
    qdp->setUuid(playlist);
    connect(qdp, &DrawnPlaylist::itemDesiredByDoubleClick, this, &PlaylistWindow::itemDesired);
    connect(qdp, &DrawnPlaylist::contextMenuRequested,
            this, &PlaylistWindow::playlist_contextMenuRequested);
    widgets.insert(playlist, qdp);
    ui->tabWidget->addTab(qdp, title);
    ui->tabWidget->setCurrentWidget(qdp);
}

void PlaylistWindow::addQuickQueue()
{
    queueWidget = new DrawnQueue();
    queueWidget->setDisplayParser(&displayParser);
    queueWidget->setUuid(QUuid());
    connect(queueWidget, &DrawnQueue::itemDesiredByDoubleClick,
            this, &PlaylistWindow::itemDesired);
    ui->quickPage->layout()->addWidget(queueWidget);
}

void PlaylistWindow::setIconTheme(IconThemer::FolderMode folderMode,
                                  const QString &fallbackFolder,
                                  const QString &customFolder)
{
    themer.setIconFolders(folderMode, fallbackFolder, customFolder);
}

void PlaylistWindow::setHideFullscreen(bool hidden)
{
    hideFullscreen = hidden;
}

void PlaylistWindow::setRememberSelectedPlaylist(bool remember)
{
    rememberSelectedPlaylist = remember;
}

bool PlaylistWindow::activateItem(QUuid playlistUuid, QUuid itemUuid,
                                  bool clickedInPlaylist)
{
    if (!widgets.contains(playlistUuid))
        return false;
    auto qdp = widgets[playlistUuid];
    qdp->scrollToItem(itemUuid, clickedInPlaylist);
    qdp->setNowPlayingItem(itemUuid);
    return true;
}

void PlaylistWindow::changePlaylistSelection(QUrl itemUrl, QUuid playlistUuid,
                                             QUuid itemUuid, bool clickedInPlaylist)
{
    (void)itemUrl;
    if (!activateItem(playlistUuid, itemUuid, clickedInPlaylist))
        return;
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    auto qpl = PlaylistCollection::queuePlaylist();
    if (!itemUuid.isNull() && qpl->first().item == itemUuid) {
        queueWidget->removeItem(itemUuid);
    }
}

void PlaylistWindow::addSimplePlaylist(QStringList data)
{

    auto pl = PlaylistCollection::getSingleton()->newPlaylist(tr("New Playlist"));
    pl->fromStringList(data);
    addNewTab(pl->uuid(), pl->title());
}

void PlaylistWindow::addPlaylistByUuid(QUuid playlistUuid)
{
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    if (!pl) {
        throw std::invalid_argument("received a nullptr for a playlist");
    }
    addNewTab(pl->uuid(), pl->title());
}

void PlaylistWindow::setDisplayFormatSpecifier(QString fmt)
{
    displayParser.takeFormatString(fmt);
    ui->tabWidget->currentWidget()->update();
}

void PlaylistWindow::dockLocationMaybeChanged()
{
    QLayout *layout = ui->contents->layout();
    QMargins margins = layout->contentsMargins();
    margins.setBottom(isFloating() ? margins.top() : 0);
    layout->setContentsMargins(margins);
}

void PlaylistWindow::newTab()
{
    bool ok;
    QString title = QInputDialog::getText(this, tr("Enter Playlist Name"),
                                           tr("Name"), QLineEdit::Normal,
                                          tr("New Playlist"), &ok);
    if (!ok)
        return;
    else if (title.isEmpty())
        title = tr("New Playlist");

    auto pl = PlaylistCollection::getSingleton()->newPlaylist(title.replace("&","+"));
    addNewTab(pl->uuid(), pl->title());
}

void PlaylistWindow::closeTab()
{
    int index = ui->tabWidget->currentIndex();
    on_tabWidget_tabCloseRequested(index);
    updateCurrentPlaylist();
}

void PlaylistWindow::duplicateTab()
{
    auto origin = currentPlaylistWidget();
    auto remote = PlaylistCollection::getSingleton()->clonePlaylist(origin->uuid());
    addNewTab(remote->uuid(), remote->title());
}

void PlaylistWindow::renameTab()
{
    auto widget = static_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
    QUuid tabUuid = widget->uuid();
    if (tabUuid.isNull())
        return;
    QInputDialog *qid = new QInputDialog(this);
    qid->setAttribute(Qt::WA_DeleteOnClose);
    qid->setWindowModality(Qt::ApplicationModal);
    qid->setWindowTitle(tr("Enter playlist name"));
    qid->setTextValue(ui->tabWidget->tabText(ui->tabWidget->currentIndex()).replace("&", ""));
    connect(qid, &QInputDialog::accepted, this, [this, widget, tabUuid, qid]() {
        int tabIndex = ui->tabWidget->indexOf(widget);
        if (tabIndex < 0)
            return;
        auto pl = PlaylistCollection::getSingleton()->getPlaylist(tabUuid);
        if (!pl)
            return;
        pl->setTitle(qid->textValue());
        ui->tabWidget->setTabText(tabIndex, qid->textValue().replace("&", "+"));
    });
    qid->show();
}

void PlaylistWindow::importTab()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString file;
    file = QFileDialog::getOpenFileName(this, tr("Import Playlist File"), QString(),
                                        tr("Playlist files (*.m3u *.m3u8 *.txt)"), nullptr, options);
    if (!file.isEmpty())
        emit importPlaylist(file);
}

void PlaylistWindow::exportTab()
{
    auto playlistUuid = currentPlaylistWidget()->uuid();
    savePlaylist(playlistUuid);
}

void PlaylistWindow::copy()
{
    clipboard->fromSelected(currentPlaylistWidget());
}

void PlaylistWindow::copyQueue()
{
    clipboard->fromQueue(currentPlaylistWidget());
}

void PlaylistWindow::paste()
{
    clipboard->appendToPlaylist(currentPlaylistWidget());
}

void PlaylistWindow::pasteQueue()
{
    clipboard->appendAndQuickQueue(currentPlaylistWidget());
}

void PlaylistWindow::playCurrentItem()
{
    auto qdp = currentPlaylistWidget();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto itemUuid = qdp->currentItemUuid();
    if (itemUuid.isNull())
        return;
    emit itemDesired(pl->uuid(), itemUuid, false);
}

bool PlaylistWindow::playActiveItem()
{
    auto qdp = currentPlaylistWidget();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto itemUuid = qdp->nowPlayingItem();
    if (itemUuid.isNull())
        return false;
    emit itemDesired(pl->uuid(), itemUuid, false);
    return true;
}

void PlaylistWindow::selectNext()
{
    auto qdp = currentPlaylistWidget();
    int index = qdp->currentRow();
    if (index + 1 < qdp->count())
        qdp->setCurrentRow(index + 1);
}

void PlaylistWindow::selectPrevious()
{
    auto qdp = currentPlaylistWidget();
    int index = qdp->currentRow();
    if (index > 0)
        qdp->setCurrentRow(index - 1);
}

void PlaylistWindow::incExtraPlayTimes()
{
    auto qdp = currentPlaylistWidget();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto incrementer = [pl](QUuid itemUuid) {
        auto item = pl->getItem(itemUuid);
        if (Q_LIKELY(!item.isNull()))
            item->incExtraPlayTimes();
    };
    qdp->traverseSelected(incrementer);
    qdp->viewport()->update();
}

void PlaylistWindow::decExtraPlayTimes()
{
    auto qdp = currentPlaylistWidget();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto decrementer = [pl](QUuid itemUuid) {
        auto item = pl->getItem(itemUuid);
        if (Q_LIKELY(!item.isNull()))
            item->decExtraPlayTimes();
    };
    qdp->traverseSelected(decrementer);
    qdp->viewport()->update();
}

void PlaylistWindow::zeroExtraPlayTimes()
{
    auto qdp = currentPlaylistWidget();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto zeroer = [pl](QUuid itemUuid) {
        auto item = pl->getItem(itemUuid);
        if (Q_LIKELY(!item.isNull()))
            item->setExtraPlayTimes(0);
    };
    qdp->traverseSelected(zeroer);
    qdp->viewport()->update();
}

void PlaylistWindow::activateNext()
{
    auto qdp = currentPlaylistWidget();
    auto now = qdp->nowPlayingItem();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto next = pl->itemAfter(now);
    if (!!next)
        activateItem(qdp->uuid(), next->uuid());
}

void PlaylistWindow::activatePrevious()
{
    auto qdp = currentPlaylistWidget();
    auto now = qdp->nowPlayingItem();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto prev = pl->itemBefore(now);
    if (!!prev)
        activateItem(qdp->uuid(), prev->uuid());
}

void PlaylistWindow::quickQueue()
{
    if (ui->showQueue->isChecked())
        return;

    auto qdp = currentPlaylistWidget();
    auto itemUuids = qdp->currentItemUuids();
    if (itemUuids.isEmpty())
        return;
    auto qpl = PlaylistCollection::queuePlaylist();
    QList<QUuid> added;
    QList<int>removed;
    qpl->toggle(qdp->uuid(), itemUuids, added, removed);
    queueWidget->removeItems(removed);
    queueWidget->addItems(added);
    qdp->viewport()->update();
    queueWidget->viewport()->update();
}

void PlaylistWindow::visibleToQueue()
{
    if (ui->showQueue->isChecked())
        return;

    QList<QUuid> added;
    QList<int> removed;
    PlaylistCollection::queuePlaylist()->\
            toggleFromPlaylist(currentPlaylistWidget()->uuid(), added, removed);
    queueWidget->removeItems(removed);
    queueWidget->addItems(added);
    queueWidget->viewport()->update();
    currentPlaylistWidget()->viewport()->update();
}

void PlaylistWindow::setQueueMode(bool yes)
{
    ui->playStack->setCurrentIndex(yes ? 1 : 0);
    setWindowTitle(yes ? tr("Queue") : tr("Playlist"));
    ui->showQueue->setChecked(yes);
    emit quickQueueMode(yes);
}

void PlaylistWindow::revealSearch()
{
    showSearch = true;
    activateWindow();
    ui->searchHost->setVisible(true);
    ui->searchField->setFocus();
}

void PlaylistWindow::finishSearch()
{
    showSearch = false;
    if (!ui->searchHost->isVisible())
        return;

    if (!ui->searchField->text().isEmpty()) {
        ui->searchField->setText(QString());
        setPlaylistFilters(QString());
    }

    if (ui->searchField->hasFocus())
        currentPlaylistWidget()->setFocus();

    ui->searchHost->setVisible(false);
}

void PlaylistWindow::savePlaylist(const QUuid &playlistUuid)
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString file;
    file = QFileDialog::getSaveFileName(this, tr("Export Playlist File"), QString(),
                                        tr("Playlist files (*.m3u *.m3u8)"), nullptr, options);
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    if (!file.isEmpty() && pl)
        emit exportPlaylist(file, pl->toStringList());
}

void PlaylistWindow::sortPlaylistByLabel(const QUuid &playlistUuid)
{
    auto qdp = widgets.value(playlistUuid, nullptr);
    if (!qdp)
        return;
    auto converter = [this](QSharedPointer<Item> i) {
        return displayParser.parseMetadata(i->metadata(), i->toDisplayString(), Helpers::VideoFile);
    };
    auto lessThan = [](const QString &a, const QString &b) {
        return a < b;
    };
    qdp->sort<QString>(converter, lessThan);
}

void PlaylistWindow::sortPlaylistByUrl(const QUuid &playlistUuid)
{
    auto qdp = widgets.value(playlistUuid, nullptr);
    if (!qdp)
        return;

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);

    using namespace std::placeholders;
    auto converter = [](QSharedPointer<Item> i) { return i->url(); };
    auto comparer = std::bind(&Helpers::compareUrls, _1, _2, collator);
    qdp->sort<QUrl>(converter, comparer);
}

void PlaylistWindow::shufflePlaylist(const QUuid &playlistUuid, bool shuffle)
{
    Logger::log(logModule, "shufflePlaylist start");
    if (widgets.contains(playlistUuid))
        widgets[playlistUuid]->playlist()->setShuffle(shuffle);
    refreshPlaylist(playlistUuid);
    Logger::log(logModule, "shufflePlaylist done");
}

void PlaylistWindow::reshufflePlaylist(const QUuid &playlistUuid)
{
    if (widgets.contains(playlistUuid))
        widgets[playlistUuid]->playlist()->shuffleItems();
    refreshPlaylist(playlistUuid);
}

void PlaylistWindow::refreshPlaylist(const QUuid &playlistUuid)
{
    Logger::log(logModule, "refreshPlaylist start");
    auto qdp = widgets.value(playlistUuid, nullptr);
    if (qdp) {
        qdp->repopulateItems();
        qdp->setCurrentItem(widgets[playlistUuid]->playlist()->nowPlaying());
    }
    Logger::log(logModule, "refreshPlaylist done");
}

void PlaylistWindow::restorePlaylist(const QUuid &playlistUuid)
{
    auto qdp = widgets.value(playlistUuid, nullptr);
    if (!qdp)
        return;
    auto converter = [&](QSharedPointer<Item> i) {
        return i->originalPosition();
    };
    auto lessThan = [](const int &a, const int &b) {
        return a < b;
    };
    qdp->sort<int>(converter, lessThan);
}

void PlaylistWindow::self_visibilityChanged()
{
    // When the window was (re)created/destroyed for whatever reason by
    // the toolkit/wm/etc, reveal the search widget if it was active last.
    if (showSearch)
        revealSearch();
    else
        finishSearch();
}

void PlaylistWindow::self_dockLocationChanged(Qt::DockWidgetArea area)
{
    if (area != Qt::NoDockWidgetArea)
        emit windowDocked();
    dockLocationMaybeChanged();
}

void PlaylistWindow::playlist_removeItemRequested()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;

    qdp->traverseSelected([this, qdp](QUuid itemUuid) {
        int index = qdp->currentRow();
        if (index + 1 < qdp->count())
            selectNext();
        else
            selectPrevious();

        qdp->removeItem(itemUuid);
    });
    updatePlaylistHasItems();
}

void PlaylistWindow::removePlaylistItem(const QUuid &itemUuid)
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;

    qdp->removeItem(itemUuid);
    updatePlaylistHasItems();
}

void PlaylistWindow::playlist_removeAllRequested()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;

    qdp->removeAll();
    updatePlaylistHasItems();
}

void PlaylistWindow::playlist_copySelectionToClipboard(const QUuid &playlistUuid)
{
    auto qdp = widgets.value(playlistUuid, nullptr);
    if (!qdp)
        return;
    auto pl = qdp->playlist();
    QList<QUrl> urls;
    qdp->traverseSelected([&pl,&urls](QUuid itemUuid) {
        urls.append(pl->getItem(itemUuid)->url());
    });
    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void PlaylistWindow::playlist_hideOnFullscreenToggled(bool checked)
{
    Q_UNUSED(checked)
    // TODO: is this stub of any use?
}

void PlaylistWindow::playlist_contextMenuRequested(const QPoint &p, const QUuid &playlistUuid, const QUuid &itemUuid)
{
    if (!widgets.contains(playlistUuid))
        return;
    DrawnPlaylist *listWidget = widgets[playlistUuid];
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(playlistUuid);
    bool noItemSelected = true;
    auto qdp = currentPlaylistWidget();
    if (qdp)
        noItemSelected = qdp->currentItemUuids().empty();

    QMenu *m = new QMenu(this);
    QAction *a;

    a = new QAction(m);
    a->setText(tr("Open"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid,itemUuid]() {
        emit itemDesired(playlistUuid, itemUuid, true);
    });
    a->setDisabled(noItemSelected);
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Add"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        emit playlistAddItem(playlistUuid);
    });
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Add Folder"));
    connect(a, &QAction::triggered,
            this, [this]() {
        emit playlistAddFolder();
    });
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Remove"));
    connect(a, &QAction::triggered,
            this, &PlaylistWindow::playlist_removeItemRequested);
    a->setDisabled(noItemSelected);
    m->addAction(a);

    m->addSeparator();

    a = new QAction(m);
    a->setText(tr("Copy To clipboard"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        playlist_copySelectionToClipboard(playlistUuid);
    });
    a->setDisabled(noItemSelected);
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Save As..."));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        savePlaylist(playlistUuid);
    });
    m->addAction(a);

    m->addSeparator();

    a = new QAction(m);
    a->setText(tr("Sort By Label"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        sortPlaylistByLabel(playlistUuid);
    });
    if (pl && pl->shuffle())
        a->setDisabled(true);
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Sort By Url"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        sortPlaylistByUrl(playlistUuid);
    });
    if (pl && pl->shuffle())
        a->setDisabled(true);
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Restore"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
        restorePlaylist(playlistUuid);
    });
    if (pl && pl->shuffle())
        a->setDisabled(true);
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Reshuffle"));
    connect(a, &QAction::triggered,
            this, [this,playlistUuid]() {
                reshufflePlaylist(playlistUuid);
    });
    if (!pl || !pl->shuffle())
        a->setDisabled(true);
    m->addAction(a);

    m->addSeparator();

    a = new QAction(m);
    a->setText(tr("Repeat"));
    a->setCheckable(true);
    a->setChecked(listWidget->playlist()->repeat());
    connect(a, &QAction::triggered,
            this, [this,playlistUuid](bool checked) {
        if (widgets.contains(playlistUuid))
            widgets[playlistUuid]->playlist()->setRepeat(checked);
        emit playlistRepeatChanged(playlistUuid, checked);
    });
    m->addAction(a);

    a = new QAction(m);
    a->setText(tr("Shuffle"));
    a->setCheckable(true);
    a->setChecked(listWidget->playlist()->shuffle());
    connect(a, &QAction::triggered,
            this, [this,playlistUuid](bool checked) {
                shufflePlaylist(playlistUuid, checked);
                emit playlistShuffleChanged(playlistUuid, checked);
    });
    m->addAction(a);

    m->addSeparator();

    a = new QAction(m);
    a->setText(tr("Hide On Fullscreen"));
    a->setCheckable(true);
    a->setChecked(hideFullscreen);
    connect(a, &QAction::triggered,
            this, [this](bool checked) {
        hideFullscreen = checked;
        emit hideFullscreenChanged(checked);
    });
    m->addAction(a);

    m->exec(listWidget->mapToGlobal(p));
    delete m;
}

void PlaylistWindow::on_tabWidget_tabCloseRequested(int index)
{
    int current = ui->tabWidget->currentIndex();
    auto qdp = static_cast<DrawnPlaylist *>(ui->tabWidget->widget(index));
    if (!qdp)
        return;

    auto collection = PlaylistCollection::getSingleton();
    emit playlistsBackupRequested();
    auto backup = PlaylistCollection::getBackup();
    auto copy = collection->clonePlaylist(qdp->uuid());
    copy->setCreated(qdp->playlist()->created());
    collection->takePlaylist(copy->uuid()); // detach 'copy' from collection
    backup->addPlaylist(copy);
    emit playlistMovedToBackup(copy->uuid());

    if (qdp->uuid().isNull()) {
        qdp->removeAll();
    } else {
        collection->removePlaylist(qdp->uuid());
        widgets.remove(qdp->uuid());
        ui->tabWidget->removeTab(index);
    }
    if (current == index)
        updateCurrentPlaylist();
}

void PlaylistWindow::on_tabWidget_tabBarClicked(int index)
{
    ui->tabWidget->setCurrentIndex(index);
    auto qdp = currentPlaylistWidget();
    if (qdp)
        qdp->scrollToItem(qdp->nowPlayingItem(), false);
}

void PlaylistWindow::on_tabWidget_tabBarDoubleClicked(int index)
{
    Q_UNUSED(index);
    renameTab();
}

void PlaylistWindow::on_tabWidget_customContextMenuRequested(const QPoint &pos)
{
    auto widget = static_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
    bool isQuickPlaylist = widget->uuid().isNull();

    QMenu *m = new QMenu(this);
    m->addAction(tr("&New Playlist"), this, SLOT(newTab()));
    m->addAction(tr("&Remove Playlist"), this, SLOT(closeTab()));
    m->addAction(tr("&Clear Playlist"), this, SLOT(playlist_removeAllRequested()));
    m->addAction(tr("&Duplicate Playlist"), this, SLOT(duplicateTab()));
    QAction *renameTab = m->addAction(tr("&Rename Playlist"), this, SLOT(renameTab()));
    if (isQuickPlaylist)
        renameTab->setEnabled(false);
    m->addAction(tr("&Import Playlist"), this, SLOT(importTab()));
    m->addAction(tr("&Export Playlist"), this, SLOT(exportTab()));
    m->exec(ui->tabWidget->mapToGlobal(pos));
    delete m;
}

void PlaylistWindow::on_searchField_textEdited(const QString &arg1)
{
    setPlaylistFilters(arg1);
}

void PlaylistWindow::on_tabWidget_currentChanged(int index)
{
    Q_UNUSED(index)
    updateCurrentPlaylist();
}

void PlaylistWindow::on_searchField_returnPressed()
{
    playCurrentItem();
}
