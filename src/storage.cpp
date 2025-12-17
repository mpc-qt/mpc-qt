#include <QCoreApplication>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include "logger.h"
#include "storage.h"
#include "platform/unify.h"

const char fileFavorites[] = "favorites";
const char fileGeometryV2[] = "geometry_v2";
const char fileKeys[] = "keys_v2";
const char filePlaylists[] = "playlists";
const char filePlaylistsBackup[] = "playlists_backup";
const char fileRecent[] = "recent";
const char fileSettings[] = "settings";

QString Storage::configPath;

Storage::Storage(QObject *parent) :
    QObject(parent)
{
    QDir().mkpath(fetchConfigPath());
}

QString Storage::fetchConfigPath()
{
    if (Platform::isWindows) {
        QDir d(QCoreApplication::applicationDirPath());
        if (d.entryList().contains("portable.txt")) {
            configPath = d.absolutePath() + "/config";
        }
    }
    if (configPath.isEmpty()) {
        configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/mpc-qt";
        configPath = Platform::fixedConfigPath(configPath);
    }
    return configPath;
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
    LogStream("storage") << "writing " + name + " start";
    QJsonDocument doc;
    doc.setArray(QJsonArray::fromVariantList(qvl));
    writeJsonObject(name, doc);
    LogStream("storage") << "writing " + name + " done";
}

QVariantList Storage::readVList(QString name)
{
    LogStream("storage") << "reading " + name + " start";
    QVariantList vList = readJsonObject(name).array().toVariantList();
    LogStream("storage") << "reading " + name + " done";
    return vList;
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
    if (fname == filePlaylistsBackup && doc == playlistsBackup &&
            QFileInfo::exists(QDir(configPath).absoluteFilePath(fname + ".json"))) {
        LogStream("storage") << "backup not needed for " + fname;
        return;
    }
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
    if (fname == filePlaylistsBackup)
        playlistsBackup = doc;
    return doc;
}
