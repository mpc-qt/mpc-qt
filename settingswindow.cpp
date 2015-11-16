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
    s.temporalInterpolation = ui->scalingTemporalInterpolation->isChecked();
    s.dither = ui->ditherDithering->isChecked();
    s.ditherDepth = ui->ditherDepth->value();
    s.ditherType = (settings::DitherType)ui->ditherType->currentIndex();
    s.ditherFruitSize = ui->ditherFruitSize->value();
    s.debanding = ui->debandEnabled->isChecked();
    s.scaleScalar = (settings::ScaleScalar)ui->scaleScalar->currentIndex();
    s.dscaleScalar = (settings::ScaleScalar)(ui->dscaleScalar->currentIndex() - 1);
    s.cscaleScalar = (settings::ScaleScalar)ui->cscaleScalar->currentIndex();
    s.tscaleScalar = (settings::TimeScalar)ui->tscaleScalar->currentIndex();
    return s;
}

void SettingsWindow::takeSettings(const settings &s)
{
    ui->videoDumbMode->setChecked(s.videoIsDumb);
    ui->scalingTemporalInterpolation->setChecked(s.temporalInterpolation);
    ui->ditherDepth->setValue(s.ditherDepth);
    ui->ditherType->setCurrentIndex(s.ditherType);
    ui->ditherFruitSize->setValue(s.ditherFruitSize);
    ui->debandEnabled->setChecked(s.debanding);
    ui->scaleScalar->setCurrentIndex(s.scaleScalar);
    ui->dscaleScalar->setCurrentIndex(s.dscaleScalar);
    ui->cscaleScalar->setCurrentIndex(s.cscaleScalar);
    ui->tscaleScalar->setCurrentIndex(s.tscaleScalar);
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

QVariantMap settings::toVMap()
{

}

void settings::fromVMap(const QVariantMap &m)
{

}
