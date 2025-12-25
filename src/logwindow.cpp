#include <QClipboard>
#include <QFileDialog>
#include <QPlainTextEdit>
#include <QTextDocument>
#include "logger.h"
#include "logwindow.h"
#include "ui_logwindow.h"

static constexpr char logModule[] =  "logwindow";

LogWindow::LogWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
    Logger *logger = Logger::singleton();
    connect(logger, &Logger::logMessage,
            this, &LogWindow::appendMessage,
            Qt::QueuedConnection);
    connect(logger, &Logger::logMessageBuffer,
            this, &LogWindow::appendMessageBlock,
            Qt::QueuedConnection);
}

LogWindow::~LogWindow()
{
    delete ui;
}

void LogWindow::appendMessage(QString message)
{
    ui->messages->appendPlainText(message);
}

void LogWindow::appendMessageBlock(QStringList messages)
{
    ui->messages->appendPlainText(messages.join('\n'));
}

void LogWindow::setLogLimit(int lines)
{
    ui->messages->setMaximumBlockCount(lines);
}

void LogWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit windowClosed();
}

void LogWindow::on_copy_clicked()
{
    QTextDocument *doc = ui->messages->document();
    QApplication::clipboard()->setText(doc->toPlainText());
}

void LogWindow::on_save_clicked()
{
    static QFileDialog::Options options = QFileDialog::Options();
#ifdef Q_OS_MAC
    options = QFileDialog::DontUseNativeDialog;
#endif
    QString textToSave = ui->messages->document()->toPlainText();
    static QString lastLog;
    QString file = QFileDialog::getSaveFileName(this, tr("Save File"), lastLog, "Text files (*.txt)",
                                                nullptr, options);
    if (file.isEmpty())
        return;

    QFile f(file);
    if (!f.open(QIODevice::WriteOnly))
        return;
    QTextStream stream(&f);
    stream.setGenerateByteOrderMark(true);
    stream << textToSave;
}

void LogWindow::on_clear_clicked()
{
    ui->messages->clear();
}
