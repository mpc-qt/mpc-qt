#ifndef LOGGER_H
#define LOGGER_H

#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QTextStream>
#include <QTimer>
// Logger class, alternatively thought of as the LogBuffer class.
// To begin with will stores all debug output until setFlushTime
// or setLoggingEnabled is called, so you may construct early and
// lazily connect to whatever ui you later make.
class Logger : public QObject
{
    Q_OBJECT

public:
    explicit Logger(QObject *owner = nullptr);
    ~Logger();
    static Logger *singleton();
    static void setConsoleLogging(bool consoleLogging);

    // log: lossely based on the requirements for printing mpv messages
    static void log(QString line);
    static void log(QString prefix, QString message);
    static void log(QString prefix, QString level, QString message);
    // logs: like log, but with stringlists.  Spaces are inserted between items.
    static void logs(const QStringList &strings);
    static void logs(QString prefix, const QStringList &strings);
    static void logs(QString prefix, QString level, const QStringList &strings);

signals:
    void logMessage(QString message);
    void logMessageBuffer(QStringList messages);

public slots:
    void setLogFile(QString fileName);
    void setLoggingEnabled(bool enabled);
    void setFlushTime(int msec);
    void fatalMessage();
    void flushMessages();
    void makeLog(QString line);
    void makeLogPrefixed(QString prefix, QString message);
    void makeLogDescriptively(QString prefix, QString level, QString message);

private:
    bool loggingEnabled = true; // by default, log everything until we get told not to
    bool immediateMode = false; // by default, debug messages are stored
    QElapsedTimer elapsed;
    QTimer *flushTimer = nullptr;
    QFile *logFile = nullptr;
    QTextStream *logFileStream = nullptr;
    QString logFileName;
    QStringList pendingMessages;

};



// This log stream class is principally for serializing variants et al.
// It is generally faster to use the Logger::log(s) functions because
// LogStream has to jump through a few hoops.  Use LogStream this way:
//      LogStream(logModule) << "some text " << value;
// Unlike QDebug, this does not insert spaces between << invocations.
class LogStream {
public:
    explicit LogStream(QString prefix = QString(), QString level = QString());
    ~LogStream();
    LogStream &always();
    LogStream &operator<<(const char *a);
    LogStream &operator<<(const QString &a);
    LogStream &operator<<(const QVariant &a);

private:
    QString buffer;
    QString prefix;
    QString level;
    QTextStream stream;
    bool directFlush = false;
};

#endif // LOGGER_H
