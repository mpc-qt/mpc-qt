#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "logger.h"

class StdFileCopy {
public:
    explicit StdFileCopy(FILE *fp) {
        int handle = dup(fileno(fp));
        if (handle == -1) {
            ptr = fp;
            return;
        }
        ptr = fdopen(handle, "wt");
        fclose(fp);
    }
    ~StdFileCopy() {
        fclose(ptr);
        ptr = nullptr;
    }
    FILE *ptr = nullptr;
};


static bool logToConsole = false; // by default, do not log to console
static bool loggerInstanceSetBefore = false;
static Logger *loggerInstance = nullptr;
static StdFileCopy realStdErr(stderr);

void loggerCallback(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    switch (type) {
    case QtDebugMsg:
        Logger::log("qt", "debug", msg);
        break;
    case QtInfoMsg:
        Logger::log("qt", "info", msg);
        break;
    case QtWarningMsg:
        Logger::log("qt", "warn", msg);
        break;
    case QtCriticalMsg:
        Logger::log("qt", "crit", msg);
        break;
    case QtFatalMsg:
        Logger::log("qt", "fatal", msg);
        QMetaObject::invokeMethod(Logger::singleton(), "fatalMessage",
                                  Qt::BlockingQueuedConnection);
        std::abort();
    }
}

Logger::Logger(QObject *owner) : QObject(owner)
{
    if (loggerInstanceSetBefore)
        std::abort();
    qInstallMessageHandler(loggerCallback);
    immediateMode = false;
    flushTimer = new QTimer(this);
    connect(flushTimer, &QTimer::timeout,
            this, &Logger::flushMessages);
    elapsed.start();
}

Logger::~Logger()
{
    qInstallMessageHandler(nullptr);
    if (logFileStream) {
        delete logFileStream;
        logFileStream = nullptr;
    }
    if (logFile) {
        delete logFile;
        logFile = nullptr;
    }
    loggerInstance = nullptr;
}

Logger *Logger::singleton()
{
    // Only create an instance if it hasn't done so before
    if (!loggerInstanceSetBefore && !loggerInstance) {
        loggerInstance = new Logger();
        loggerInstanceSetBefore = true;
    }
    return loggerInstance;
}

void Logger::setConsoleLogging(bool consoleLogging)
{
    logToConsole = consoleLogging;
}

// The log buffer class has likely been moved to
// its own separate thread, so remotely invoke
// the respective log methods.
void Logger::log(QString line)
{
    Logger *log = singleton();
    if (!log)
        return;
    QMetaObject::invokeMethod(log, "makeLog",
                              Qt::QueuedConnection,
                              Q_ARG(QString, line));
}

void Logger::log(QString prefix, QString message)
{
    Logger *log = singleton();
    if (!log)
        return;
    QMetaObject::invokeMethod(log, "makeLogPrefixed",
                              Qt::QueuedConnection,
                              Q_ARG(QString, prefix),
                              Q_ARG(QString, message));
}

void Logger::log(QString prefix, QString level, QString message)
{
    Logger *log = singleton();
    if (!log)
        return;
    QMetaObject::invokeMethod(log, "makeLogDescriptively",
                              Qt::QueuedConnection,
                              Q_ARG(QString, prefix),
                              Q_ARG(QString, level),
                              Q_ARG(QString, message));
}

void Logger::logs(const QStringList &strings)
{
    log(strings.join(' '));
}

void Logger::logs(QString prefix, const QStringList &strings)
{
    log(prefix, strings.join(' '));
}

void Logger::logs(QString prefix, QString level, const QStringList &strings)
{
    log(prefix, level, strings.join(' '));
}

void Logger::setLogFile(QString fileName)
{
    if (logFileName == fileName)
        return;
    logFileName = fileName;
    if (fileName.isEmpty()) {
        log("logger", "log file closed");
        if (logFileStream) {
            delete logFileStream;
            logFileStream = nullptr;
        }
        if (logFile) {
            delete logFile;
            logFile = nullptr;
        }
        return;
    }
    logFile = new QFile(fileName);
    if (!logFile->open(QFile::WriteOnly))
        return;

    logs("logger", {"log file", logFileName, "opened for writing"});
    logFileStream = new QTextStream(logFile);
    logFileStream->setGenerateByteOrderMark(true);
}

void Logger::setLoggingEnabled(bool enabled)
{
    if (enabled && !loggingEnabled) {
        loggingEnabled = true;
        makeLogPrefixed("logger", "enabling logging");
        if (!immediateMode)
            flushTimer->start();
    } else if (!enabled && loggingEnabled) {
        makeLogPrefixed("logger", "disabling logging");
        flushMessages();
        flushTimer->stop();
        loggingEnabled = false;
    }
}

void Logger::setFlushTime(int msec)
{
    if (msec <= 0) {
        flushTimer->stop();
        if (!immediateMode)
            flushMessages();
        immediateMode = true;
        return;
    } else {
        immediateMode = false;
        flushTimer->setInterval(std::max(100, msec));
        flushTimer->start();
    }
}

void Logger::fatalMessage()
{
    // Oops!  Something went very wrong!
    // Try to flush anything pending to stderr
    if (!logToConsole) { // ignore when lines are already written
        for (const auto &i : std::as_const(pendingMessages))
            std::fprintf(realStdErr.ptr, "%s\n", i.toLocal8Bit().data());
        fflush(realStdErr.ptr);
    }
    if (logFileStream) {
        *logFileStream << pendingMessages.join("\n") << '\n';
        logFileStream->flush();
    }
    pendingMessages.clear();
}

void Logger::flushMessages()
{
    if (pendingMessages.isEmpty())
        return;
    if (logFileStream) {
        *logFileStream << pendingMessages.join('\n') << '\n';
        logFileStream->flush();
    }
    emit logMessageBuffer(pendingMessages);
    pendingMessages.clear();
}

void Logger::makeLog(QString line)
{
    if (!loggingEnabled)
        return;
    line = QString("[%1] %2").arg(QString::number(elapsed.nsecsElapsed()/1000000000.0, 'f', 3), line.trimmed());
    // If you're encountering early or fantastic errors, make this if statement true
    if (logToConsole) {
        fprintf(realStdErr.ptr, "%s\n",  line.toLocal8Bit().constData());
        fflush(realStdErr.ptr);
    }
    if (immediateMode) {
        emit logMessage(line);
        if (logFileStream) {
            *logFileStream << line << '\n';
            logFileStream->flush();
        }
    } else {
        pendingMessages.append(line);
    }
}

void Logger::makeLogPrefixed(QString prefix, QString message)
{
    makeLog(QString("[%1] %2").arg(prefix, message));
}

void Logger::makeLogDescriptively(QString prefix, QString level, QString message)
{
    makeLog(QString("[%1] %2: %3").arg(prefix, level, message));
}



LogStream::LogStream(QString prefix, QString level) : buffer(),
    prefix(prefix), level(level), stream(&buffer)
{

}

LogStream::~LogStream()
{
    if (buffer.isEmpty())
        return;
    if (directFlush) {
        fprintf(realStdErr.ptr, "[%s] %s: %s\n",
                prefix.toUtf8().data(),
                level.toUtf8().data(),
                buffer.toUtf8().data());
        return;
    }
    if (prefix.isEmpty() && level.isEmpty())
        Logger::log(buffer);
    else if (level.isEmpty())
        Logger::log(prefix, buffer);
    else
        Logger::log(prefix, level, buffer);
}

LogStream &LogStream::always()
{
    directFlush = true;
    return *this;
}

LogStream &LogStream::operator<<(const char *a)
{
    stream << a;
    return *this;
}

LogStream &LogStream::operator<<(const QString &a)
{
    stream << a;
    return *this;
}

LogStream &LogStream::operator<<(const QVariant &a)
{
    if (a.canConvert<QString>() && a.metaType().id() != QMetaType::QStringList) {
        stream << a.toString();
    } else if (a.canConvert(QMetaType(QMetaType::QVariantMap))) {
        stream << "{";
        auto list = a.toMap();
        int count = list.count();
        for (auto it = list.constBegin(); it != list.constEnd(); it++, count--) {
            stream << '"' << it.key() << "\":";
            *this << it.value();
            if (count > 1)
                stream << ", ";
        }
        *this << "}";
    } else if (a.canConvert(QMetaType(QMetaType::QVariantList))) {
        stream << "[";
        auto list = a.toList();
        int count = list.count();
        for (int i = 0; i < count; i++) {
            *this << list.value(i);
            if (i < count - 1)
                stream << ", ";
        }
        stream << "]";
    } else {
        stream << "(unserializable)";
    }
    return *this;
}
