#include "helpers.h"
#include "propertieswindow.h"
#include "ui_propertieswindow.h"

#include <QMimeDatabase>
#include <QTextCursor>
#include <QVariantMap>
#include <QProcess>

PropertiesWindow::PropertiesWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PropertiesWindow)
{
    ui->setupUi(this);
}

PropertiesWindow::~PropertiesWindow()
{
    delete ui;
}

void PropertiesWindow::setFileName(const QString &filename)
{
    QString text = filename.isEmpty() ? QString("-") : filename;
    ui->detailsFilename->setText(text);
    ui->clipFilename->setText(text);

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filename);
    QIcon icon = QIcon::fromTheme(mime.iconName(), QIcon(":/images/icon.png"));
    ui->detailsIcon->setPixmap(icon.pixmap(32, 32));
    ui->clipIcon->setPixmap(icon.pixmap(32, 32));
}

void PropertiesWindow::setFileFormat(const QString &format)
{
    ui->detailsType->setText(format.isEmpty() ? QString("-")
                                              : format);
}

void PropertiesWindow::setFileSize(const int64_t &bytes)
{
    QString text;
    if (bytes < 1024) {
        text = QString("%1 bytes").arg(QString::number(bytes));
    } else {
        QString concise;
        QString unit;
        int64_t divisor;
        if (bytes < 1024*1024) {
            divisor = 1024;
            unit = "KiB";
        } else if (bytes < 1024*1024*1024) {
            divisor = 1024*1024;
            unit = "MiB";
        } else {
            divisor = 1024*1024*1024;
            unit = "TiB";
        }
        text = QString("%1 %2 (%3 bytes)").arg(QString::number((bytes+divisor-1)/divisor),
                                               unit, QString::number(bytes));
    }
    ui->detailsSize->setText(text);
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
    QDateTime date;
    date.setSecsSinceEpoch(secsSinceEpoch);
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
        { "video", "Video:" },
        { "audio", "Audio:" },
        { "sub", "Subtitles:" }
    });
    QStringList lines;
    for (const QVariant &var : tracks) {
        QVariantMap track = var.toMap();
        QStringList line;

        QString typ = track.contains("type") ? track["type"].toString() : QString();
        line << typeToText.value(typ, "Unknown:");
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
            line << QString("%1fps").arg(QString::number(track["demux-fps"].toDouble(), 'g', 3));
        if (track.contains("demux-samplerate"))
            line << QString("%1Hz").arg(track["demux-samplerate"].toInt());
        if (track.contains("audio-channels"))
            line << QString("%1ch").arg(track["audio-channels"].toInt());
        lines << line.join(' ');
    }
    QTextCursor cursor = ui->detailsTracks->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.removeSelectedText();
    cursor.insertText(lines.join('\n'));
    ui->detailsTracks->setTextCursor(cursor);
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
}

