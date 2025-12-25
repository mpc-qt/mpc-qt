#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpressionValidator>
#include <helpers.h>

#include "gotowindow.h"
#include "ui_gotowindow.h"
#include "logger.h"

static constexpr char logModule[] =  "goto";

GoToWindow::GoToWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GoToWindow)
{
    Logger::log(logModule, "creating ui");
    ui->setupUi(this);
    Logger::log(logModule, "finished creating ui");
}

GoToWindow::~GoToWindow()
{
    delete ui;
}

void GoToWindow::init(double currentTime, double maxTime, double fps)
{
    maxTime_ = maxTime;
    fps_ = fps == 0 ? 24 : fps;

    ui->goToTime->setText(Helpers::toDateFormatWithZero(currentTime));
    QRegularExpression regexTime(R"(^([0-9]?[0-9]):([0-9]?[0-9]):([0-9]?[0-9])\.[0-9]{3}$)");
    auto const* validatorTime = new QRegularExpressionValidator(regexTime, this);
    ui->goToTime->setValidator(validatorTime);
    ui->goToTime->installEventFilter(this);

    QRegularExpression regexFrame(R"(^([0-9]{1,})$)");
    auto const* validatorFrame = new QRegularExpressionValidator(regexFrame, this);
    ui->goToFrame->setValidator(validatorFrame);
    ui->goToFrame->setText(QString::number(currentTime * fps, 'f', 0));

    setFixedSize(size());
    show();
}

void GoToWindow::on_goToTime_textChanged(const QString &text)
{
    double time = Helpers::fromDateFormat(text);
    if (time < 0 || time > maxTime_)
        ui->goToTime->setStyleSheet("QLineEdit { background-color: #903736; }");
    else
        ui->goToTime->setStyleSheet("");
}

void GoToWindow::on_goToFrame_textChanged(const QString &text)
{
    double time = text.toDouble() / fps_;
    if (time < 0 || time > maxTime_)
        ui->goToFrame->setStyleSheet("QLineEdit { background-color: #903736; }");
    else
        ui->goToFrame->setStyleSheet("");
}

void GoToWindow::on_goToTimeButton_clicked()
{
    double time = Helpers::fromDateFormat(ui->goToTime->text());
    if (time < 0 || time > maxTime_)
        return;
    emit goTo(time);
    close();
}

void GoToWindow::on_goToFrameButton_clicked()
{
    double time = ui->goToFrame->text().toDouble() / fps_;
    if (time < 0 || time > maxTime_)
        return;
    emit goTo(time);
    close();
}

bool GoToWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->goToTime && event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        int cursorPos = ui->goToTime->cursorPosition();
        int key = keyEvent->key();
        if (key == Qt::Key_Backspace) {
            ui->goToTime->deselect();
            if (cursorPos > 0) {
                if (ui->goToTime->text()[cursorPos - 1].isDigit()) {
                    ui->goToTime->cursorBackward(true);
                    ui->goToTime->insert("0");
                    ui->goToTime->setCursorPosition(cursorPos - 1);
                }
                else
                    ui->goToTime->cursorBackward(false);
                return true;
            }
        }
        else if (key == Qt::Key_Delete)
            return true;
        else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
            if (ui->goToTime->text()[cursorPos].isDigit())
                ui->goToTime->cursorForward(true);
            else {
                ui->goToTime->cursorForward(false);
                return true;
            }
        }
    }
    return false;
}
