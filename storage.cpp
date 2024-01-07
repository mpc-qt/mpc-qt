#include <QCoreApplication>
#include <QStandardPaths>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include "storage.h"
#include "platform/unify.h"


static QString portablePath() {
    QDir d(QCoreApplication::applicationDirPath());
    if (d.entryList().contains("portable.txt"))
        return d.absolutePath();
    return QString();
}

QString Storage::configPath;
QString Storage::thumbPath;

Storage::Storage(QObject *parent) :
    QObject(parent)
{
    QDir().mkpath(fetchConfigPath());
    QDir().mkpath(fetchThumbnailPath());
}

QString Storage::fetchConfigPath()
{
    if (!configPath.isEmpty())
        return configPath;
    if (Platform::isWindows) {
        QString p = portablePath();
        if (!p.isEmpty()) {
            configPath = p + "/config";
            return configPath;
        }
    }
    configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/mpc-qt";
    configPath = Platform::fixedConfigPath(configPath);
    return configPath;
}

QString Storage::fetchThumbnailPath()
{
    if (!thumbPath.isEmpty())
        return thumbPath;
    if (Platform::isWindows) {
        QString p = portablePath();
        if (!p.isEmpty()) {
            thumbPath = p + "/thumbnails";
            return thumbPath;
        }
    }
    thumbPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/mpc-qt/thumbnails";
    thumbPath = Platform::fixedConfigPath(thumbPath);
    return thumbPath;
}

void Storage::writeVMap(QString name, const QVariantMap &qvm)
{
    QJsonDocument doc;
    doc.setObject(QJsonObject::fromVariantMap(qvm));
    writeJsonObject(name, doc);
}

QVariantMap Storage::readVMap(QString name)
{
    QJsonDocument doc = readJsonObject(name);
    return doc.object().toVariantMap();
}

void Storage::writeVList(QString name, const QVariantList &qvl)
{
    QJsonDocument doc;
    doc.setArray(QJsonArray::fromVariantList(qvl));
    writeJsonObject(name, doc);
}

QVariantList Storage::readVList(QString name)
{
    QJsonDocument doc = readJsonObject(name);
    return doc.array().toVariantList();
}

QStringList Storage::readM3U(const QString &where)
{
    QStringList items;
    QFile file(where);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringList();
    items = QTextStream(&file).readAll().split("\n");
    for (int i = 0; i < items.count(); ++i) {
        items[i] = items[i].trimmed();
        if (items[i].isEmpty() || items[i].startsWith("#")) {
            items.removeAt(i);
            --i;
        }
    }
    return items;
}

void Storage::writeM3U(const QString &where, QStringList items)
{
    QFile file(where);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream(&file) << "#EXTM3U\n\n" << items.join("\n");
}

void Storage::writeJsonObject(QString fname, const QJsonDocument &doc)
{
    QFile file(QDir(configPath).absoluteFilePath(fname + ".json"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    QTextStream(&file) << doc.toJson();
}

QJsonDocument Storage::readJsonObject(QString fname)
{
    QFile file(QDir(configPath).absoluteFilePath(fname + ".json"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonDocument();
    QJsonDocument doc = QJsonDocument::fromJson(QTextStream(&file).readAll().toUtf8());
    return doc;
}
