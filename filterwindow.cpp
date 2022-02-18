#include <QStringList>
#include "filterwindow.h"
#include "ui_filterwindow.h"

// Setting vf to "lavfi=graph=\"gradfun=radius=30:strength=20,vflip\"" (as per
// the mpv manual) upon casual inspection with our json ipc server like so
//   echo '{ "command": "getMpvProperty", "name": "vf" }' | socat /tmp/cmdrkotori.mpc-qt -
// yields the following data structure:
//   [
//     {
//       "enabled":true,
//       "name":"lavfi",
//       "params": {
//         "graph":"gradfun=radius=30:strength=20,vflip"
//       }
//     }
//   ]
// So it would seem that setting a lavfi graph requires building a graph
// string rather than doing anything special with maps and the like.

struct FilterOption {
    enum Type { DoubleType, IntType, BoolType, OptionType };
    Type t = DoubleType;
    const char *name;
    const char *help;
    double value = 0;
    double min = 0;
    double max = 0;
    bool tf;
    int option = 0;
    QStringList options;
    int digits;
    bool decibels;

    FilterOption& name_(const char *s) { name = s; return *this; }
    FilterOption& help_(const char *s) { help = s; return *this; }
    FilterOption& dbl_(double d) { value = d; t = DoubleType; return *this; }
    FilterOption& i64_(int64_t i) { value = (double)i; t = IntType; return *this; }
    FilterOption& tf_(bool b) { tf = b; t = BoolType; return *this; }
    FilterOption& min_(double d) { min = d; return *this; }
    FilterOption& max_(double d) { max = d; return *this; }
    FilterOption& opt_(int o) { option = o; t = OptionType; return *this; }
    FilterOption& options_(const QStringList &o) { options = o; return *this; }
    FilterOption& digits_(int d) { digits = d; return *this; }
    FilterOption& decibels_() { decibels = true; return *this; }
};

class Filter {
public:
    const char *name;
    const char *description;
    QList<FilterOption> options;
    Filter(const char *name, const char *description, QList<FilterOption> options) : name(name), description(description), options(options) {}
};

QList<Filter> audioFilters = {
    Filter("acompressor", "Audio compressor", {
        FilterOption().name_("level_in")    .help_("input gain")    .dbl_(1)        .min_(0.015625)     .max_(64)       .digits_(1).decibels_(),
        FilterOption().name_("mode")        .help_("mode")          .opt_(0)        .options_({"downward","upward"}),
        FilterOption().name_("threshold")   .help_("threshold")     .dbl_(0.125)    .min_(0.000976563)  .max_(1)        .digits_(1).decibels_(),
        FilterOption().name_("ratio")       .help_("ratio")         .dbl_(2)        .min_(1)            .max_(20),
        FilterOption().name_("attack")      .help_("attack")        .dbl_(20)       .min_(0.01)         .max_(2000)     .digits_(2),
        FilterOption().name_("release")     .help_("release")       .dbl_(250)      .min_(0.01)         .max_(9000)     .digits_(2),
        FilterOption().name_("makeup")      .help_("make up gain")  .dbl_(1)        .min_(1)            .max_(64),
        FilterOption().name_("knee")        .help_("knee")          .dbl_(2.82843)  .min_(1)            .max_(8),
        FilterOption().name_("link")        .help_("link type")     .opt_(0)        .options_({"average","maximum"}),
        FilterOption().name_("detection")   .help_("detection")     .opt_(1)        .options_({"peak","rms"}),
        FilterOption().name_("level_sc")    .help_("sidechain gain").dbl_(1)        .min_(0.015625)     .max_(64)       .digits_(1).decibels_(),
        FilterOption().name_("mix")         .help_("mix")           .dbl_(1)        .min_(0)            .max_(1)
    }),
    Filter("loudnorm", "EBU R128 loudness normalization", {
        FilterOption().name_("I")           .help_("integrated loudness target")    .dbl_(24)   .min_(-70)  .max_(-5),
        FilterOption().name_("LRA")         .help_("loudness range target")         .dbl_(7)    .min_(1)    .max_(20),
        FilterOption().name_("TP")          .help_("maximum true peak")             .dbl_(-2)   .min_(-9)   .max_(0),
        FilterOption().name_("offset")      .help_("offset gain")                   .dbl_(0)    .min_(-99)  .max_(99),
        FilterOption().name_("linear")      .help_("normalize linearly if possible").tf_(true),
        FilterOption().name_("dual_mono")   .help_("treat mono input as dual-mono") .tf_(false)
    })
};

QList<Filter> videoFilters = {
};

FilterWindow::FilterWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FilterWindow)
{
    ui->setupUi(this);
}

FilterWindow::~FilterWindow()
{
    delete ui;
}
