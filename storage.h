#ifndef STORAGE_H
#define STORAGE_H

#include <QObject>

class Storage : public QObject
{
    Q_OBJECT
public:
    explicit Storage(QObject *parent = 0);

    void writeVMap(QString name, const QVariantMap &qvm);
    QVariantMap readVMap(QString name);

    void writeVList(QString name, const QVariantList &qvl);
    QVariantList readVList(QString name);

private:
    void writeJsonObject(QString fname, const QJsonDocument &doc);
    QJsonDocument readJsonObject(QString fname);

signals:

public slots:

private:
    QString configPath;
};

#endif // STORAGE_H
