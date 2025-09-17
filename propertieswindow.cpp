#include "helpers.h"
#include "propertieswindow.h"
#include "platform/unify.h"
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
    if (Platform::isWindows) {
        QFont monoFont;
        monoFont.setFamily("Consolas");
        ui->mediaInfoText->setFont(monoFont);
    }
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

    generalData.clear();
    generalData.insert("filename", filename);
    updateLastTab();

    updateSaveVisibility();
}

void PropertiesWindow::setFileFormat(const QString &format)
{
    ui->detailsType->setText(format.isEmpty() ? QString("-")
                                              : format);
    generalData.insert("format", format);
    updateLastTab();
}

void PropertiesWindow::setFileSize(const int64_t &bytes)
{
    ui->detailsSize->setText(Helpers::fileSizeToString(bytes));
    generalData.insert("size", qlonglong(bytes));
    updateLastTab();
}

void PropertiesWindow::setMediaLength(double time)
{
    ui->detailsLength->setText(time < 0 ? QString("-")
                                        : Helpers::toDateFormatFixed(time, Helpers::ShortFormat));
    generalData.insert("time", time);
}

void PropertiesWindow::setVideoSize(const QSize &sz)
{
    ui->detailsVideoSize->setText(!sz.isValid() ? QString("-")
                                            : QString("%1 x %2").arg(sz.width()).arg(sz.height()));
}

void PropertiesWindow::setFileModifiedTime(const QUrl &file)
{
    QLocale locale;
    auto lastModified = QFileInfo(file.toLocalFile()).lastModified();
    ui->detailsModified->setText(!file.isLocalFile() ? QString("-") :
                                                       locale.toString(lastModified, QLocale::ShortFormat));
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
        if (track.contains("lang")) {
#if QT_VERSION < QT_VERSION_CHECK(6,3,0)
            QString langName = QLocale::languageToString(QLocale(track["lang"].toString()).language());
#else
            QString langName = QLocale::languageToString(QLocale::codeToLanguage(
                                                            track["lang"].toString(),
                                                            QLocale::AnyLanguageCode));
#endif
            line << QString("%1").arg(langName);
            line << QString("[%1]").arg(track["lang"].toString().remove('\n'));
        }
        if (track.contains("decoder-desc"))
            line << track["decoder-desc"].toString().remove('\n');
        else if (track.contains("codec"))
            line << track["codec"].toString().remove('\n');
        if (track.contains("codec-profile"))
            line << track["codec-profile"].toString();
        if (track.contains("demux-w") && track.contains("demux-h"))
            line << QString("%1x%2").arg(QString::number(track["demux-w"].toInt()),
                                         QString::number(track["demux-h"].toInt()));
        if (track.contains("demux-fps"))
            line << QString("%1fps").arg(QString::number(track["demux-fps"].toDouble(), 'g', 6));
        if (track.contains("demux-samplerate"))
            line << QString("%1kHz").arg(std::round(track["demux-samplerate"].toInt() / 100) / 10.0);
        if (track.contains("demux-channels")) {
            QString channels = track["demux-channels"].toString();
            line << QString("%1").arg(channels.contains("unknown") ?
                                                  channels.remove("unknown") + "ch" : channels);
        }
        int bitrate = 0;
        if (track.contains("demux-bitrate"))
            bitrate = track["demux-bitrate"].toInt();
        else if (track.contains("metadata")) {
            QVariantMap metadata = track["metadata"].toMap();
            if (metadata.contains("BPS"))
                bitrate = metadata["BPS"].toInt();
        }
        if (bitrate != 0 && (!track.contains("type") || track["type"].toString() != "sub"))
            line << QString("%1 kb/s").arg(std::round(bitrate / 1000.0));

        lines << line.join(' ');

        int trackId = 0;
        if (track.contains("id"))
            trackId = track["id"].toInt();
        track.remove("selected");
        trackText += sectionText(typeText + " #" + QString::number(trackId), track);
    }

    ui->detailsTracks->clear();
    ui->detailsTracks->insertPlainText(lines.join('\n'));

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

    ui->clipDescription->clear();
    ui->clipDescription->insertPlainText(data.contains("description") ? data["description"].toString() : QString());

    generalData.insert(data);
    if (data.contains("artist") && data.contains("title"))
        emit artistAndTitleChanged(data["artist"].toString() + " - " + data["title"].toString(),
                                   this->filename);
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
    ui->mediaInfoText->clear();
    ui->mediaInfoText->insertPlainText(generalDataText() + trackText + chapterText);
}

QString PropertiesWindow::generalDataText()
{
    QVariantMap generalDataFormatted = generalData;
    if (generalData.contains("size") && generalData.contains("time")) {
        int64_t size = generalData["size"].toLongLong();
        double time = generalData["time"].toDouble();
        generalDataFormatted.insert("overall_bitrate", QString("%1 kb/s").arg(std::round(size / time / 1000.0 * 8.0)));
        generalDataFormatted.insert("size", Helpers::fileSizeToString(size));
        generalDataFormatted.insert("time", Helpers::toDateFormatFixed(time, Helpers::ShortFormat));
    }
    return sectionText(tr("General"), generalDataFormatted);
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
