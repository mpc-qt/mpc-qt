#include "settingswindow.h"
#include "ui_settingswindow.h"
#include <QDebug>
#include <QDialogButtonBox>

const char *settings::ditherTypeToText[] = {"fruit", "ordered", "no"};

const char *settings::scaleScalarToText[]  = {
    "bilinear", "bicubic_fast", "oversample", "spline16", "spline36",
    "spline64", "sinc", "lanczos", "gingseng", "jinc", "ewa_lanczos",
    "ewa_hanning", "ewa_gingseng", "ewa_lanczossharp", "ewa_lanczossoft",
    "hassnsoft",  "bicubic", "bcspline", "catmull_rom", "mitchell",
    "robidoux", "robidouxsharp", "ewa_robidoux", "ewa_robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};

const char *settings::timeScalarToText[] = {
    "oversample", "spline16", "spline64", "sinc", "lanczos",
    "gingseng", "catmull_rom", "mitchell", "robidoux", "robidouxsharp",
    "box", "nearest", "triangle", "gaussian"
};

SettingsWindow::SettingsWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SettingsWindow)
{
    ui->setupUi(this);
    ui->pageStack->setCurrentIndex(0);
    ui->videoTabs->setCurrentIndex(0);
    ui->scalingTabs->setCurrentIndex(0);
    ui->prescalarStack->setCurrentIndex(0);
    ui->audioRendererStack->setCurrentIndex(0);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

settings SettingsWindow::buildSettings() {
    settings s;
    s.videoIsDumb = ui->videoDumbMode->isChecked();
    return s;
}

void SettingsWindow::takeSettings(const settings &s)
{
    ui->videoDumbMode->setChecked(s.videoIsDumb);
}

void SettingsWindow::on_pageTree_itemSelectionChanged()
{
    QModelIndex modelIndex = ui->pageTree->currentIndex();
    if (!modelIndex.isValid())
        return;

    static int parentIndex[] = { 0, 4, 9, 12, 13 };
    int index = 0;
    if (!modelIndex.parent().isValid())
        index = parentIndex[modelIndex.row()];
    else
        index = parentIndex[modelIndex.parent().row()] + modelIndex.row() + 1;
    ui->pageStack->setCurrentIndex(index);
    ui->pageLabel->setText(QString("<big><b>%1</b></big>").
                           arg(modelIndex.data().toString()));
}

void SettingsWindow::on_buttonBox_clicked(QAbstractButton *button)
{
    QDialogButtonBox::ButtonRole buttonRole;
    buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::ApplyRole ||
            buttonRole == QDialogButtonBox::AcceptRole) {
        emit settingsData(buildSettings());
    }
    if (buttonRole == QDialogButtonBox::AcceptRole ||
            buttonRole == QDialogButtonBox::RejectRole)
        close();
}
