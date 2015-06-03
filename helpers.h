#ifndef HELPERS_H
#define HELPERS_H
#include <QObject>
#include <QString>
#include <QVariant>

// generic helper module for general-use static functions
namespace Helpers {
    QString toDateFormat(double time);

    // Helper class for emitting data
    class DataEmitter : public QObject
    {
        Q_OBJECT
    public:
        QVariant data;
        DataEmitter(QObject *parent) : QObject(parent) {}

    signals:
        void heresSomething(QVariant data);

    public slots:
        void gotSomething();
    };
}


#endif // HELPERS_H
