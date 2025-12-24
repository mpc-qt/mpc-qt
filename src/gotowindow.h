#ifndef GOTOWINDOW_H
#define GOTOWINDOW_H

#include <QWidget>

namespace Ui {
class GoToWindow;
}

class GoToWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GoToWindow(QWidget *parent = nullptr);
    ~GoToWindow();

signals:
    void goTo(double time);

public slots:
    void init(double currentTime, double maxTime, double fps);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void on_goToTime_textChanged(const QString &text);
    void on_goToFrame_textChanged(const QString &text);
    void on_goToTimeButton_clicked();
    void on_goToFrameButton_clicked();

private:
    Ui::GoToWindow *ui;
    double maxTime_ = 0;
    double fps_ = 0;
};

#endif // GOTOWINDOW_H
