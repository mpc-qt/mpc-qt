#include <QAction>
#include <algorithm>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QGuiApplication>
#include <QMimeData>
#include <QInputDialog>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QTextBrowser>
#include <QTextDocument>
#include <QTreeWidget>
#include <QUrlQuery>
#include <QVBoxLayout>
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
    ensureChapterPreviewTab();
    ensureChapterTopicsTab();
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
    refreshChapterTopicsForCurrentPlaylist();
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
    refreshChapterTopicsForCurrentPlaylist();
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
        auto widget = qobject_cast<DrawnPlaylist *>(ui->tabWidget->widget(i));
        if (!widget)
            continue;
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
    chapterPreviewTabWidget = nullptr;
    chapterPreviewBrowser = nullptr;
    chapterPreviewEditor = nullptr;
    chapterModeButton = nullptr;
    chapterSaveButton = nullptr;
    chapterInsertTimeButton = nullptr;
    chapterTopicsTabWidget = nullptr;
    chapterTopicTree = nullptr;
    chapterTopicCards = nullptr;
    chapterMarkdownPath.clear();
    chapterEditMode = false;
    topicCardsByPath.clear();
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
    ensureChapterPreviewTab();
    ensureChapterTopicsTab();
    updatePlaylistHasItems();
}

void PlaylistWindow::updateIcons()
{
    themer.updateIcons();
}

void PlaylistWindow::updateLanguage()
{
    ui->retranslateUi(this);
    if (chapterPreviewTabWidget) {
        int idx = ui->tabWidget->indexOf(chapterPreviewTabWidget);
        if (idx >= 0)
            ui->tabWidget->setTabText(idx, tr("视频章节预览"));
        setChapterEditorMode(chapterEditMode);
    }
    if (chapterTopicsTabWidget) {
        int idx = ui->tabWidget->indexOf(chapterTopicsTabWidget);
        if (idx >= 0)
            ui->tabWidget->setTabText(idx, tr("章节话题索引"));
    }
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
    return qobject_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
}

void PlaylistWindow::updateCurrentPlaylist()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp) {
        emit currentPlaylistHasItems(false);
        return;
    }
    currentPlaylist = qdp->uuid();
    setTabOrder(ui->tabWidget->focusProxy(), qdp);
    setTabOrder(qdp, ui->searchField);
    updatePlaylistHasItems();
    refreshChapterTopicsForCurrentPlaylist();
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
    int insertIndex = ui->tabWidget->count();
    int chapterTabIndex = ui->tabWidget->indexOf(chapterPreviewTabWidget);
    if (chapterTabIndex >= 0)
        insertIndex = std::min(insertIndex, chapterTabIndex);
    int topicsTabIndex = ui->tabWidget->indexOf(chapterTopicsTabWidget);
    if (topicsTabIndex >= 0)
        insertIndex = std::min(insertIndex, topicsTabIndex);
    if (insertIndex >= ui->tabWidget->count())
        ui->tabWidget->addTab(qdp, title);
    else
        ui->tabWidget->insertTab(insertIndex, qdp, title);
    ui->tabWidget->setCurrentWidget(qdp);
}

bool PlaylistWindow::isChapterPreviewTab(int index) const
{
    return chapterPreviewTabWidget && ui->tabWidget->widget(index) == chapterPreviewTabWidget;
}

bool PlaylistWindow::isChapterTopicsTab(int index) const
{
    return chapterTopicsTabWidget && ui->tabWidget->widget(index) == chapterTopicsTabWidget;
}

void PlaylistWindow::ensureChapterPreviewTab()
{
    if (chapterPreviewTabWidget)
        return;

    chapterPreviewTabWidget = new QWidget(ui->tabWidget);
    auto *layout = new QVBoxLayout(chapterPreviewTabWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto *actionsLayout = new QHBoxLayout();
    chapterModeButton = new QPushButton(tr("编辑模式"), chapterPreviewTabWidget);
    chapterModeButton->setCheckable(true);
    connect(chapterModeButton, &QPushButton::clicked,
            this, &PlaylistWindow::toggleChapterEditMode);
    actionsLayout->addWidget(chapterModeButton);

    chapterInsertTimeButton = new QPushButton(tr("插入时间标签"), chapterPreviewTabWidget);
    connect(chapterInsertTimeButton, &QPushButton::clicked,
            this, &PlaylistWindow::insertChapterTimeTag);
    actionsLayout->addWidget(chapterInsertTimeButton);

    chapterSaveButton = new QPushButton(tr("保存"), chapterPreviewTabWidget);
    connect(chapterSaveButton, &QPushButton::clicked,
            this, &PlaylistWindow::saveChapterMarkdown);
    actionsLayout->addWidget(chapterSaveButton);
    actionsLayout->addStretch();
    layout->addLayout(actionsLayout);

    chapterPreviewBrowser = new QTextBrowser(chapterPreviewTabWidget);
    chapterPreviewBrowser->setOpenLinks(false);
    chapterPreviewBrowser->setOpenExternalLinks(false);
    connect(chapterPreviewBrowser, &QTextBrowser::anchorClicked,
            this, &PlaylistWindow::chapterPreviewAnchorClicked);
    layout->addWidget(chapterPreviewBrowser);

    chapterPreviewEditor = new QPlainTextEdit(chapterPreviewTabWidget);
    chapterPreviewEditor->setVisible(false);
    layout->addWidget(chapterPreviewEditor);

    ui->tabWidget->addTab(chapterPreviewTabWidget, tr("视频章节预览"));
    setChapterPreviewHtml(tr("<p>No chapter markdown found for the current video.</p>"));
    setChapterEditorMode(false);
}

void PlaylistWindow::ensureChapterTopicsTab()
{
    if (chapterTopicsTabWidget)
        return;

    chapterTopicsTabWidget = new QWidget(ui->tabWidget);
    auto *layout = new QVBoxLayout(chapterTopicsTabWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    chapterTopicTree = new QTreeWidget(chapterTopicsTabWidget);
    chapterTopicTree->setHeaderHidden(true);
    connect(chapterTopicTree, &QTreeWidget::itemSelectionChanged,
            this, &PlaylistWindow::chapterTopicSelectionChanged);
    layout->addWidget(chapterTopicTree, 2);

    chapterTopicCards = new QListWidget(chapterTopicsTabWidget);
    connect(chapterTopicCards, &QListWidget::itemActivated,
            this, &PlaylistWindow::chapterTopicCardActivated);
    connect(chapterTopicCards, &QListWidget::itemClicked,
            this, &PlaylistWindow::chapterTopicCardActivated);
    layout->addWidget(chapterTopicCards, 3);

    ui->tabWidget->addTab(chapterTopicsTabWidget, tr("章节话题索引"));
    refreshChapterTopicsForCurrentPlaylist();
}

void PlaylistWindow::setChapterPreviewHtml(const QString &html)
{
    if (chapterPreviewBrowser)
        chapterPreviewBrowser->setHtml(html);
}

void PlaylistWindow::setChapterEditorMode(bool editing)
{
    chapterEditMode = editing;
    if (!chapterModeButton || !chapterPreviewBrowser || !chapterPreviewEditor
            || !chapterSaveButton || !chapterInsertTimeButton)
        return;

    chapterModeButton->setChecked(editing);
    chapterModeButton->setText(editing ? tr("观看模式") : tr("编辑模式"));
    chapterPreviewBrowser->setVisible(!editing);
    chapterPreviewEditor->setVisible(editing);
    chapterSaveButton->setVisible(editing);
    chapterInsertTimeButton->setVisible(editing);
}

QString PlaylistWindow::buildChapterPreviewHtml(const QString &markdown)
{
    static const QRegularExpression timeMarker(
        QStringLiteral("<time\\s+datetime=(?:\"|')(\\d{1,2}:\\d{2}:\\d{2})(?:\"|')>(.*?)</time>"),
        QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);

    QString converted = markdown;
    converted.replace(timeMarker,
                      QStringLiteral("[\\2](chapter://jump?time=\\1) <small>[\\1]</small>"));

    QTextDocument doc;
    doc.setMarkdown(converted);
    return doc.toHtml();
}

bool PlaylistWindow::validateChapterMarkdown(const QString &markdown, QString &error) const
{
    static const QRegularExpression timeMarker(
        QStringLiteral("<time\\s+datetime=(?:\"|')(\\d{1,2}:\\d{2}:\\d{2})(?:\"|')>(.*?)</time>"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = timeMarker.globalMatch(markdown);
    QString stripped = markdown;
    while (it.hasNext()) {
        auto match = it.next();
        bool ok = false;
        parseChapterTimeToSeconds(match.captured(1), &ok);
        if (!ok) {
            error = tr("时间标签格式错误：%1").arg(match.captured(1));
            return false;
        }
        stripped.replace(match.captured(0), QString());
    }

    if (stripped.contains("<time", Qt::CaseInsensitive)
            || stripped.contains("</time>", Qt::CaseInsensitive)) {
        error = tr("检测到未闭合或格式错误的 <time> 标签。");
        return false;
    }
    return true;
}

void PlaylistWindow::loadChapterMarkdownFromPath(const QString &markdownPath)
{
    ensureChapterPreviewTab();
    chapterMarkdownPath = markdownPath;

    QFile markdownFile(markdownPath);
    if (!markdownFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (chapterPreviewEditor)
            chapterPreviewEditor->clear();
        setChapterPreviewHtml(tr("<p>Unable to read chapter markdown file.</p>"));
        return;
    }
    const QString markdown = QString::fromUtf8(markdownFile.readAll());
    if (chapterPreviewEditor)
        chapterPreviewEditor->setPlainText(markdown);
    setChapterPreviewHtml(buildChapterPreviewHtml(markdown));
}

void PlaylistWindow::refreshChapterTopicsForCurrentPlaylist()
{
    ensureChapterTopicsTab();
    topicCardsByPath.clear();
    if (!chapterTopicTree || !chapterTopicCards)
        return;

    chapterTopicTree->clear();
    chapterTopicCards->clear();

    auto pl = PlaylistCollection::getSingleton()->getPlaylist(currentPlaylist);
    if (!pl)
        return;

    static const QRegularExpression topicRegex(
        QStringLiteral("#([\\w\\x{4e00}-\\x{9fa5}-]+(?:/[\\w\\x{4e00}-\\x{9fa5}-]+)*)"));
    static const QRegularExpression cardRegex(
        QStringLiteral("<time\\s+datetime=(?:\"|')(\\d{1,2}:\\d{2}:\\d{2})(?:\"|')>(.*?)</time>"),
        QRegularExpression::CaseInsensitiveOption);

    QSet<QString> topicPaths;
    pl->iterateItems([&](QSharedPointer<Item> item) {
        if (!item || !item->url().isLocalFile())
            return;
        QFileInfo videoInfo(item->url().toLocalFile());
        const QString markdownPath = videoInfo.dir().filePath(videoInfo.completeBaseName() + ".md");
        QFile markdownFile(markdownPath);
        if (!markdownFile.exists() || !markdownFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return;
        const QString markdown = QString::fromUtf8(markdownFile.readAll());
        const QStringList lines = markdown.split('\n');
        for (const QString &line : lines) {
            QStringList topicsInLine;
            auto topicIt = topicRegex.globalMatch(line);
            while (topicIt.hasNext()) {
                const QString topicPath = topicIt.next().captured(1);
                topicsInLine.append(topicPath);
                topicPaths.insert(topicPath);
            }
            if (topicsInLine.isEmpty())
                continue;
            auto cardIt = cardRegex.globalMatch(line);
            while (cardIt.hasNext()) {
                auto card = cardIt.next();
                const QString time = card.captured(1);
                const QString label = card.captured(2).trimmed();
                if (label.isEmpty())
                    continue;
                for (const QString &topic : topicsInLine)
                    topicCardsByPath[topic].append({label, time, markdownPath});
            }
        }
    });

    QHash<QString, QTreeWidgetItem*> nodes;
    for (const QString &fullPath : topicPaths) {
        QStringList parts = fullPath.split('/', Qt::SkipEmptyParts);
        QString currentPath;
        QTreeWidgetItem *parent = nullptr;
        for (const QString &part : parts) {
            currentPath = currentPath.isEmpty() ? part : (currentPath + "/" + part);
            if (!nodes.contains(currentPath)) {
                QTreeWidgetItem *node = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(chapterTopicTree);
                node->setText(0, part);
                node->setData(0, Qt::UserRole, currentPath);
                nodes.insert(currentPath, node);
            }
            parent = nodes[currentPath];
        }
    }
    chapterTopicTree->expandAll();
}

double PlaylistWindow::parseChapterTimeToSeconds(const QString &timeText, bool *ok) const
{
    const QStringList parts = timeText.split(':');
    if (parts.size() != 3) {
        if (ok)
            *ok = false;
        return 0.0;
    }
    bool hourOk = false;
    bool minuteOk = false;
    bool secondOk = false;
    int hour = parts[0].toInt(&hourOk);
    int minute = parts[1].toInt(&minuteOk);
    int second = parts[2].toInt(&secondOk);
    bool valid = hourOk && minuteOk && secondOk
            && hour >= 0 && minute >= 0 && minute < 60
            && second >= 0 && second < 60;
    if (ok)
        *ok = valid;
    if (!valid)
        return 0.0;
    return hour * 3600.0 + minute * 60.0 + second;
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

void PlaylistWindow::setIconTheme(IconThemer::FolderMode folderMode, const QString &customFolder)
{
    themer.setIconFolders(folderMode, customFolder);
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
    if (ui->tabWidget->currentWidget())
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
    if (!origin)
        return;
    auto remote = PlaylistCollection::getSingleton()->clonePlaylist(origin->uuid());
    addNewTab(remote->uuid(), remote->title());
}

void PlaylistWindow::renameTab()
{
    auto widget = qobject_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
    if (!widget)
        return;
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
    auto current = currentPlaylistWidget();
    if (!current)
        return;
    auto playlistUuid = current->uuid();
    savePlaylist(playlistUuid);
}

void PlaylistWindow::copy()
{
    auto current = currentPlaylistWidget();
    if (current)
        clipboard->fromSelected(current);
}

void PlaylistWindow::copyQueue()
{
    auto current = currentPlaylistWidget();
    if (current)
        clipboard->fromQueue(current);
}

void PlaylistWindow::paste()
{
    auto current = currentPlaylistWidget();
    if (current)
        clipboard->appendToPlaylist(current);
}

void PlaylistWindow::pasteQueue()
{
    auto current = currentPlaylistWidget();
    if (current)
        clipboard->appendAndQuickQueue(current);
}

void PlaylistWindow::playCurrentItem()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto itemUuid = qdp->currentItemUuid();
    if (itemUuid.isNull())
        return;
    emit itemDesired(pl->uuid(), itemUuid, false);
}

bool PlaylistWindow::playActiveItem()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return false;
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
    if (!qdp)
        return;
    int index = qdp->currentRow();
    if (index + 1 < qdp->count())
        qdp->setCurrentRow(index + 1);
}

void PlaylistWindow::selectPrevious()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
    int index = qdp->currentRow();
    if (index > 0)
        qdp->setCurrentRow(index - 1);
}

void PlaylistWindow::incExtraPlayTimes()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
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
    if (!qdp)
        return;
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
    if (!qdp)
        return;
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
    if (!qdp)
        return;
    auto now = qdp->nowPlayingItem();
    auto pl = PlaylistCollection::getSingleton()->getPlaylist(qdp->uuid());
    auto next = pl->itemAfter(now);
    if (!!next)
        activateItem(qdp->uuid(), next->uuid());
}

void PlaylistWindow::activatePrevious()
{
    auto qdp = currentPlaylistWidget();
    if (!qdp)
        return;
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
    if (!qdp)
        return;
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
    auto current = currentPlaylistWidget();
    if (!current)
        return;

    QList<QUuid> added;
    QList<int> removed;
    PlaylistCollection::queuePlaylist()->\
            toggleFromPlaylist(current->uuid(), added, removed);
    queueWidget->removeItems(removed);
    queueWidget->addItems(added);
    queueWidget->viewport()->update();
    current->viewport()->update();
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

    if (ui->searchField->hasFocus()) {
        auto current = currentPlaylistWidget();
        if (current)
            current->setFocus();
    }

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
    if (isChapterPreviewTab(index))
        return;
    int current = ui->tabWidget->currentIndex();
    auto qdp = qobject_cast<DrawnPlaylist *>(ui->tabWidget->widget(index));
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
    emit closingPlaylist(qdp->uuid());

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
    if (isChapterPreviewTab(index) || isChapterTopicsTab(index))
        return;
    renameTab();
}

void PlaylistWindow::on_tabWidget_customContextMenuRequested(const QPoint &pos)
{
    auto widget = qobject_cast<DrawnPlaylist *>(ui->tabWidget->currentWidget());
    if (!widget)
        return;
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
    if (isChapterPreviewTab(index) || isChapterTopicsTab(index))
        ui->searchHost->setVisible(false);
    updateCurrentPlaylist();
}

void PlaylistWindow::on_searchField_returnPressed()
{
    playCurrentItem();
}

void PlaylistWindow::toggleChapterEditMode()
{
    ensureChapterPreviewTab();
    const bool toEdit = !chapterEditMode;
    if (toEdit && chapterPreviewEditor)
        chapterPreviewEditor->setFocus();
    if (!toEdit && chapterPreviewEditor)
        setChapterPreviewHtml(buildChapterPreviewHtml(chapterPreviewEditor->toPlainText()));
    setChapterEditorMode(toEdit);
}

void PlaylistWindow::saveChapterMarkdown()
{
    ensureChapterPreviewTab();
    if (chapterMarkdownPath.isEmpty()) {
        QMessageBox::warning(this, tr("无法保存"),
                             tr("当前视频不是本地文件，无法保存章节 Markdown。"));
        return;
    }
    if (!chapterPreviewEditor)
        return;

    const QString markdown = chapterPreviewEditor->toPlainText();
    QString error;
    if (!validateChapterMarkdown(markdown, error)) {
        QMessageBox::warning(this, tr("Markdown 校验失败"), error);
        return;
    }

    QFile file(chapterMarkdownPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("保存失败"),
                             tr("无法写入文件：%1").arg(chapterMarkdownPath));
        return;
    }
    file.write(markdown.toUtf8());
    file.close();
    setChapterPreviewHtml(buildChapterPreviewHtml(markdown));
    refreshChapterTopicsForCurrentPlaylist();
    QMessageBox::information(this, tr("保存成功"), tr("章节 Markdown 已保存。"));
}

void PlaylistWindow::insertChapterTimeTag()
{
    ensureChapterPreviewTab();
    if (!chapterPreviewEditor)
        return;

    bool ok = false;
    QString timeText = QInputDialog::getText(
                this, tr("插入时间点"), tr("请输入时间 (HH:MM:SS)："),
                QLineEdit::Normal, QStringLiteral("00:00:00"), &ok);
    if (!ok)
        return;

    QString label = QInputDialog::getText(
                this, tr("标签内容"), tr("请输入标签内容："),
                QLineEdit::Normal, tr("你好"), &ok);
    if (!ok)
        return;

    bool valid = false;
    parseChapterTimeToSeconds(timeText, &valid);
    if (!valid) {
        QMessageBox::warning(this, tr("格式错误"),
                             tr("时间格式必须是 HH:MM:SS 且分秒范围正确。"));
        return;
    }
    chapterPreviewEditor->insertPlainText(
                QStringLiteral("<time datetime=\"%1\">%2</time>\n")
                .arg(timeText, label.toHtmlEscaped()));
}

void PlaylistWindow::chapterTopicSelectionChanged()
{
    if (!chapterTopicTree || !chapterTopicCards)
        return;
    chapterTopicCards->clear();
    auto selected = chapterTopicTree->selectedItems();
    if (selected.isEmpty())
        return;
    const QString topicPath = selected.first()->data(0, Qt::UserRole).toString();
    const auto cards = topicCardsByPath.value(topicPath);
    for (const TopicCard &card : cards) {
        QFileInfo markdownInfo(card.markdownPath);
        auto *item = new QListWidgetItem(
                    QStringLiteral("%1 [%2] - %3")
                    .arg(card.label, card.time, markdownInfo.fileName()),
                    chapterTopicCards);
        item->setData(Qt::UserRole, card.markdownPath);
        item->setData(Qt::UserRole + 1, card.label);
        item->setData(Qt::UserRole + 2, card.time);
    }
}

void PlaylistWindow::chapterTopicCardActivated(QListWidgetItem *item)
{
    if (!item)
        return;
    const QString markdownPath = item->data(Qt::UserRole).toString();
    const QString label = item->data(Qt::UserRole + 1).toString();
    const QString time = item->data(Qt::UserRole + 2).toString();
    if (markdownPath.isEmpty())
        return;

    loadChapterMarkdownFromPath(markdownPath);
    if (chapterPreviewTabWidget)
        ui->tabWidget->setCurrentWidget(chapterPreviewTabWidget);
    setChapterEditorMode(false);
    if (chapterPreviewBrowser) {
        if (!chapterPreviewBrowser->find(label))
            chapterPreviewBrowser->find(time);
    }
}

void PlaylistWindow::updateChapterPreviewForItem(QUrl itemUrl, QUuid playlistUuid,
                                                 QUuid itemUuid, bool clickedInPlaylist)
{
    Q_UNUSED(playlistUuid)
    Q_UNUSED(itemUuid)
    Q_UNUSED(clickedInPlaylist)
    ensureChapterPreviewTab();

    if (!itemUrl.isLocalFile()) {
        chapterMarkdownPath.clear();
        if (chapterPreviewEditor)
            chapterPreviewEditor->clear();
        setChapterPreviewHtml(tr("<p>Chapter preview only supports local video files.</p>"));
        refreshChapterTopicsForCurrentPlaylist();
        return;
    }

    const QFileInfo videoInfo(itemUrl.toLocalFile());
    chapterMarkdownPath = videoInfo.dir().filePath(videoInfo.completeBaseName() + ".md");
    QFile markdownFile(chapterMarkdownPath);
    if (!markdownFile.exists()) {
        if (chapterPreviewEditor)
            chapterPreviewEditor->clear();
        setChapterPreviewHtml(tr("<p>No chapter markdown found for the current video.</p>"));
        refreshChapterTopicsForCurrentPlaylist();
        return;
    }
    loadChapterMarkdownFromPath(chapterMarkdownPath);
    refreshChapterTopicsForCurrentPlaylist();
}

void PlaylistWindow::chapterPreviewAnchorClicked(const QUrl &link)
{
    if (link.scheme() != "chapter")
        return;
    const QUrlQuery query(link);
    const QString timeText = query.queryItemValue("time");
    bool ok = false;
    const double seconds = parseChapterTimeToSeconds(timeText, &ok);
    if (!ok)
        return;
    emit chapterTimeRequested(seconds);
}
