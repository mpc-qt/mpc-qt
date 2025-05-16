#include "helpers.h"
#include "propertieswindow.h"
#include "ui_propertieswindow.h"

#include <QMimeDatabase>
#include <QTextCursor>
#include <QStandardPaths>
#include <QFileDialog>

PropertiesWindow::PropertiesWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PropertiesWindow)
{
    ui->setupUi(this);
    ui->tabWidget->setCurrentIndex(0);
    setMetaData(QVariantMap());
    updateSaveVisibility();
    connect(ui->tabWidget, &QTabWidget::currentChanged,
            this, &PropertiesWindow::updateSaveVisibility);
}

PropertiesWindow::~PropertiesWindow()
{
    delete ui;
}

void PropertiesWindow::setFileName(const QString &filename)
{
    this->filename = filename;
    QString text = filename.isEmpty() ? QString("-") : filename;
    ui->detailsFilename->setText(text);
    ui->clipFilename->setText(text);

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filename);
    QIcon icon = QIcon::fromTheme(mime.iconName(), QIcon(":/images/icon.png"));
    ui->detailsIcon->setPixmap(icon.pixmap(32, 32));
    ui->clipIcon->setPixmap(icon.pixmap(32, 32));

    updateSaveVisibility();
}

void PropertiesWindow::setFileFormat(const QString &format)
{
    ui->detailsType->setText(format.isEmpty() ? QString("-")
                                              : format);
}

void PropertiesWindow::setFileSize(const int64_t &bytes)
{
    ui->detailsSize->setText(Helpers::fileSizeToString(bytes));
}

void PropertiesWindow::setMediaLength(double time)
{
    ui->detailsLength->setText(time < 0 ? QString("-")
                                        : Helpers::toDateFormat(time));
}

void PropertiesWindow::setVideoSize(const QSize &sz)
{
    ui->detailsLength->setText(sz.isValid() ? QString("-")
                                            : QString("%1 x %2").arg(sz.width(), sz.height()));
}

void PropertiesWindow::setFileCreationTime(const int64_t &secsSinceEpoch)
{
    QDateTime date(QDateTime::fromMSecsSinceEpoch(secsSinceEpoch * 1000));
    ui->detailsCreated->setText(date.isNull() ? QString("-") : date.toString());
}

void PropertiesWindow::setMediaTitle(const QString &title)
{
    ui->clipTitle->setText(title.isEmpty() ? QString("-") : title);
}

void PropertiesWindow::setFilePath(const QString &path)
{
    ui->clipLocation->setText(path.isEmpty() ? QString("-") : path);
}

void PropertiesWindow::setTracks(const QVariantList &tracks)
{
    QMap<QString,QString> typeToText({
        { "video", tr("Video") },
        { "audio", tr("Audio") },
        { "sub", tr("Subtitles") }
    });

    trackText.clear();
    QStringList lines;
    for (const QVariant &var : tracks) {
        QVariantMap track = var.toMap();
        QStringList line;

        QString type = track.contains("type") ? track["type"].toString() : QString();
        QString typeText = typeToText.value(type, "Unknown:");
        line << typeText << ":";
        if (track.contains("decoder-desc"))
            line << track["decoder-desc"].toString().remove('\n');
        else if (track.contains("codec"))
            line << track["codec"].toString().remove('\n');
        if (track.contains("lang"))
            line << track["lang"].toString().remove('\n');
        if (track.contains("demux-w") && track.contains("demux-h"))
            line << QString("%1x%2").arg(QString::number(track["demux-w"].toInt()),
                                         QString::number(track["demux-h"].toInt()));
        if (track.contains("demux-fps"))
            line << QString("%1fps").arg(QString::number(track["demux-fps"].toDouble(), 'g', 6));
        if (track.contains("demux-samplerate"))
            line << QString("%1kHz").arg(std::round(track["demux-samplerate"].toInt() / 100) / 10.0);
        if (track.contains("audio-channels"))
            line << QString("%1ch").arg(track["audio-channels"].toInt());
        lines << line.join(' ');

        int trackId = 0;
        if (track.contains("id"))
            trackId = track["id"].toInt();
        track.remove("selected");
        trackText += sectionText(typeText + " #" + QString::number(trackId), track);
    }

    QTextCursor cursor = ui->detailsTracks->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    cursor.insertText(lines.join('\n'));
    ui->detailsTracks->setTextCursor(cursor);

    updateLastTab();
}

void PropertiesWindow::setMetaData(QVariantMap data)
{
    QStringList items;
    if (data.contains("author"))    items << data["author"].toString();
    if (data.contains("artist"))    items << data["artist"].toString();
    if (data.isEmpty())             items << "-";
    ui->clipAuthor->setText(items.join(' '));
    ui->clipCopyright->setText(data.contains("copyright") ? data["copyright"].toString()
                               : data.contains("date") ? data["date"].toString()
                               : QString("-"));
    ui->clipRating->setText(data.contains("rating") ? data["rating"].toString() : QString("-"));

    QTextCursor cursor = ui->clipDescription->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    cursor.insertText(data.contains("description") ? data["description"].toString() : QString());
    ui->clipDescription->setTextCursor(cursor);

    metadataText = sectionText(tr("General"), data);
    if (data.contains("artist") && data.contains("title"))
        emit artistAndTitleChanged(data["artist"].toString() + " - " + data["title"].toString());
    updateLastTab();
}

void PropertiesWindow::setChapters(const QVariantList &chapters)
{
    chapterText.clear();
    if (chapters.isEmpty())
        return;

    chapterText += tr("Menu\n");
    for (const QVariant &v : chapters) {
        QVariantMap node(v.toMap());
        QString fmt("%1 - %2\n");
        QString timeText = "[" + Helpers::toDateFormat(node["time"].toDouble()) + "]";
    #if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        timeText.resize(25, ' ');
    #else
        if (timeText.length() < 25)
            timeText += QString(25 - timeText.length(), ' ');
    #endif
        chapterText += fmt.arg(timeText, node["title"].toString());
    }
    chapterText += '\n';
    updateLastTab();
}

void PropertiesWindow::updateSaveVisibility()
{
    ui->save->setVisible(ui->tabWidget->currentIndex()==2
                         && !filename.isEmpty());
}

void PropertiesWindow::updateLastTab()
{
    QTextCursor cursor = ui->mediaInfoText->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    cursor.insertText(metadataText + trackText + chapterText);
    cursor.setPosition(0);
    ui->mediaInfoText->setTextCursor(cursor);
}

QString PropertiesWindow::sectionText(const QString &header, const QVariantMap &fields)
{
    QString text;
    text += header + '\n';
    if (fields.isEmpty())
        text.append(tr("File has no data for this section.\n"));

    QVariantMap::const_iterator i = fields.constBegin();
    while (i != fields.constEnd()) {
        QString keyText = i.key();
    #if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
        keyText.resize(25, ' ');
    #else
        if (keyText.length() < 25)
            keyText += QString(25 - keyText.length(), ' ');
    #endif
        text += QString("%1 : %2\n").arg(keyText, i.value().toString());
        ++i;
    }
    text += '\n';
    return text;
}

void PropertiesWindow::on_save_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString docsFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QFileInfo info(filename + ".MediaInfo.txt");
    QString filename = QString("%1/%2").arg(docsFolder, info.fileName());
    QString metadataText = ui->mediaInfoText->document()->toPlainText();
    filename = QFileDialog::getSaveFileName(this, QString(), filename,
                                            tr("Text documents (*.txt);;"
                                               "All files (*.*)"), nullptr, options);
    if (filename.isEmpty())
        return;
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    file.write(metadataText.toUtf8());
}
