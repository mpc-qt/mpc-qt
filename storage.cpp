#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include "storage.h"

Storage::Storage(QObject *parent) :
    QObject(parent)
{
    // This may need patching for OSX
    QSettings::setDefaultFormat(QSettings::IniFormat);
    configPath = QFileInfo(QSettings("mpc-qt", "mpc-qt").fileName()).absolutePath() + "/";
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
