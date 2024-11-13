#ifndef STORAGE_H
#define STORAGE_H

#include <QJsonDocument>
#include <QObject>

extern const char fileFavorites[];
extern const char fileGeometryV2[];
extern const char fileKeys[];
extern const char filePlaylists[];
extern const char filePlaylistsBackup[];
extern const char fileRecent[];
extern const char fileSettings[];

class Storage : public QObject
{
    Q_OBJECT
public:
    explicit Storage(QObject *parent = nullptr);
    static QString fetchConfigPath();

    void writeVMap(QString name, const QVariantMap &qvm);
    QVariantMap readVMap(QString name);

    void writeVList(QString name, const QVariantList &qvl);
    QVariantList readVList(QString name);

    QStringList readM3U(const QString &where);
    void writeM3U(const QString &where, QStringList items);

private:
    void writeJsonObject(QString fname, const QJsonDocument &doc);
    QJsonDocument readJsonObject(QString fname);

signals:

public slots:

private:
    static QString configPath;
    QJsonDocument playlistsBackup;
};

#endif // STORAGE_H
