#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>

class PlaybackManager : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackManager(QObject *parent = 0);

signals:

public slots:

};

#endif // MANAGER_H
