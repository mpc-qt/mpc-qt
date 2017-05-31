#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include "storage.h"

Storage::Storage(QObject *parent) :
    QObject(parent)
{
    // CHECKME: is this the right org domain for OSX?
    QSettings settings(QSettings::IniFormat,
                       QSettings::UserScope,
                       "mpc-qt", "mpc-qt");
    configPath = QFileInfo(settings.fileName()).absolutePath() + "/";
    QDir().mkpath(configPath);
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
