#include <QScreen>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QAction>
#include <QDirIterator>
#include <cmath>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStyleHints>
#include "helpers.h"
#include "logger.h"
#include "platform/unify.h"

static constexpr char logModule[] =  "helpers";
const char autoIcons[] = "auto";
const char blackIconsPath[] = ":/images/theme/black/";
const char whiteIconsPath[] = ":/images/theme/white/";

const QSet<QString> Helpers::audioVideoFileExtensions {
    // DVD/Blu-ray audio formats
    "ac3", "a52",
    "eac3",
    "mlp",
    "dts",
    "dts-hd", "dtshd",
    "true-hd", "thd", "truehd", "thd+ac3",
    "tta",
    // Uncompressed formats
    "pcm",
    "wav",
    "aiff", "aif", "aifc",
    "amr",
    "awb",
    "au", "snd",
    "lpcm",
    "yuv",
    "y4m",
    // Free lossless formats
    "ape",
    "wv",
    "shn",
    // MPEG formats
    "m2ts", "m2t", "mts", "mtv", "ts", "tsv", "tsa", "tts", "trp",
    "adts", "adt",
    "mpa", "m1a", "m2a", "mp1", "mp2",
    "mp3",
    "mpeg", "mpg", "mpe", "mpeg2", "m1v", "m2v", "mp2v", "mpv", "mpv2", "mod", "tod",
    "vob", "vro",
    "evob", "evo",
    "mpeg4", "m4v", "mp4", "mp4v", "mpg4",
    "m4a",
    "aac",
    "h264", "avc", "x264", "264",
    "hevc", "h265", "x265", "265",
    // Xiph formats
    "flac",
    "oga", "ogg",
    "opus",
    "spx",
    "ogv", "ogm",
    "ogx",
    // Matroska formats
    "mkv",
    "mk3d",
    "mka",
    "webm",
    "weba",
    "av1",
    // Misc formats
    "avi", "vfw",
    "divx",
    "3iv",
    "xvid",
    "nut",
    "flic", "fli", "flc",
    "nsv",
    "gxf",
    "mxf",
    // Windows Media formats
    "wma",
    "wm",
    "wmv",
    "asf",
    "dvr-ms", "dvr",
    "wtv",
    // DV formats
    "dv", "hdv",
    // Flash Video formats
    "flv",
    "f4v",
    "f4a",
    // QuickTime formats
    "qt", "mov",
    "hdmov",
    // Real Media formats
    "rm",
    "rmvb",
    "ra", "ram",
    // 3GPP formats
    "3ga",
    "3ga2",
    "3gpp", "3gp",
    "3gp2", "3g2",
    // Video game formats
    "ay",
    "gbs",
    "gym",
    "hes",
    "kss",
    "nsf",
    "nsfe",
    "sap",
    "spc",
    "vgm",
    "vgz",
    // Playlist formats
    "m3u", "m3u8",
    "pls",
    "cue",
    // Module file formats
    "mod",
    "s3m",
    "xm",
    "it",
    "669",
    "amf",
    "ams",
    "dbm",
    "dmf",
    "dsm",
    "far",
    "mdl",
    "med",
    "mtm",
    "okt",
    "ptm",
    "stm",
    "ult",
    "umx",
    "mt2",
    "psm"
};

const QSet<QString> Helpers::imagesFileExtensions {
    // Image formats
    "bmp",
    "dds",
    "dpx",
    "exr",
    "j2k",
    "jpg",
    "jpeg",
    "jpegls",
    "pam",
    "pbm",
    "pcx",
    "pgmyuv",
    "pgm",
    "pictor",
    "png",
    "ppm",
    "psd",
    "qdraw",
    "sgi",
    "svg",
    "sunrast",
    "tiff",
    "webp",
    "xpm"
};

const QSet<QString> Helpers::archivesFileExtensions {
    // Archives
    "rar",
    "zip",
    "cbz",
    "cbr",
    "iso"
};
const QSet<QString> Helpers::othersFileExtensions {
    // Other formats
    "at9",
    "mpc"
};

const QSet<QString> Helpers::allMediaExtensions = Helpers::audioVideoFileExtensions |
                                            Helpers::imagesFileExtensions |
                                            Helpers::archivesFileExtensions |
                                            Helpers::othersFileExtensions;

const QSet<QString> Helpers::subsExtensions {
    "aqtitle", "aqt",
    "ass", "ssa",
    "dvbsub", "sub",
    "jacosub", "jss",
    "microdvd", "sub",
    "mpl2", "ttxt",
    "mpsub", "sub",
    "pjs",
    "realtext", "rt",
    "sami", "smi",
    "srt",
    "stl",
    "subviewer1", "sub",
    "subviewer", "sub",
    "sup",
    "vobsub",
    "vplayer",
    "webvtt", "vtt"
};

QString Helpers::fileSizeToString(int64_t bytes)
{
    QString text;
    if (bytes < 1024) {
        text = QString("%1 bytes").arg(QString::number(bytes));
    } else {
        QString unit;
        double divisor;
        if (bytes < 1024*1024) {
            divisor = 1024;
            unit = "KiB";
        } else if (bytes < 1024*1024*1024) {
            divisor = 1024*1024;
            unit = "MiB";
        } else {
            divisor = 1024*1024*1024;
            unit = "GiB";
        }
        text = QString("%1 %2 (%3 bytes)").arg(QString::number(bytes/divisor,'g',3),
                                               unit, QString::number(bytes));
    }
    return text;
}

QString Helpers::fileSizeToStringShort(int64_t bytes)
{
    QString text;
    if (bytes < 1024) {
        text = QString("%1 B").arg(QString::number(bytes));
    } else {
        QString unit;
        double divisor;
        if (bytes < 1024*1024) {
            divisor = 1024;
            unit = "KiB";
        } else if (bytes < 1024*1024*1024) {
            divisor = 1024*1024;
            unit = "MiB";
        } else {
            divisor = 1024*1024*1024;
            unit = "GiB";
        }
        text = QString("%1 %2").arg(QString::number(bytes/divisor,'g',3), unit);
    }
    return text;

}

QString Helpers::toDateFormat(double time)
{
    int t = int(time*1000 + 0.5);
    if (t < 0)
        t = 0;
    int hr = t/3600000;
    int mn = t/60000 % 60;
    int se = t%60000 / 1000;
    int fr = t % 1000;
    return QString("%1:%2:%3.%4").arg(QString::number(hr))
            .arg(QString::number(mn),2,'0')
            .arg(QString::number(se),2,'0')
            .arg(QString::number(fr),3,'0');
}

QString Helpers::toDateFormatWithZero(double time)
{
    QString date = toDateFormat(time);
    return date.split(':').first().length() < 2 ? "0" + date : date;
}

QString Helpers::toDateFormatFixed(double time, Helpers::TimeFormat format)
{
    if (format == ShortFormat || format == ShortHourFormat)
        time = std::round(time);
    int t = int(time*1000 + 0.5);
    if (t < 0)
        t = 0;
    int hr = t/3600000;
    int mn = t/60000 % 60;
    int se = t%60000 / 1000;
    int fr = t % 1000;
    switch (format) {
    case LongFormat:
        return QString("%1:%2:%3.%4").arg(QString::number(hr))
                .arg(QString::number(mn),2,'0')
                .arg(QString::number(se),2,'0')
                .arg(QString::number(fr),3,'0');
    case ShortFormat:
        return QString("%1:%2:%3").arg(QString::number(hr),2,'0')
                .arg(QString::number(mn),2,'0')
                .arg(QString::number(se),2,'0');
    case LongHourFormat:
        return QString("%1:%2.%3")
                .arg(QString::number(mn),2,'0')
                .arg(QString::number(se),2,'0')
                .arg(QString::number(fr),3,'0');
    case ShortHourFormat:
        return QString("%1:%2")
                .arg(QString::number(mn),2,'0')
                .arg(QString::number(se),2,'0');
    }
    return QString();
}

QDate Helpers::dateFromCFormat(const char date[])
{
    static auto regexp = QRegularExpression("\\s+");
    QStringList dates = QString(date).simplified().split(regexp);
    QStringList months = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    QDate d(dates[2].toInt(), months.indexOf(dates[0])+1, dates[1].toInt());
    return d;
}

QTime Helpers::timeFromCFormat(const char time[])
{
    QStringList times = QString(time).split(':');
    QTime t(times[0].toInt(), times[1].toInt(), times[2].toInt());
    return t;
}

double Helpers::fromDateFormat(QString date)
{
    QStringList times = date.split(':');
    for (auto &time : std::as_const(times)) {
        if (time.toDouble() >= 60)
            return -1;
    }
    return times[0].toDouble()*3600 + times[1].toDouble()*60 + times[2].toDouble();
}

static QString grabBrackets(QString source, int &position, int const &length) {
    QString match;
    QChar c;
    enum MatchMode { Bracket, Inside, Finish };
    MatchMode mm = Bracket;
    while (mm != Finish && position < length) {
        c = source.at(position);
        if (mm == Bracket) {
            if (c != '{') {
                position--;
                mm = Finish;
            } else {
                mm = Inside;
            }
            ++position;
        } else if (mm == Inside) {
            if (c != '}')
                match.append(c);
            else
                mm = Finish;
            ++position;
        }
    }
    return match;
}


QString Helpers::parseFormat(QString fmt, QString fileName,
                             Helpers::DisabledTrack disabled,
                             Subtitles subtitles, double timeNav,
                             double timeBegin, double timeEnd)
{
    // convenient format parsing
    struct TimeParse {
        explicit TimeParse(double time) {
            this->time = time;
            int t = int(time*1000 + 0.5);
            hr = t/3600000;
            mn = t/60000 % 60;
            se = t%60000 / 1000;
            fr = t % 1000;
        }
        QString toString(QChar fmt) {
            switch (fmt.unicode()) {
            case 'p':
                return QString("%1:%2:%3")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0');
            case 'P':
                return QString("%1:%2:%3.%4")
                        .arg(QString::number(hr),2,'0')
                        .arg(QString::number(mn),2,'0')
                        .arg(QString::number(se),2,'0')
                        .arg(QString::number(fr),3,'0');
            case 'H':
                return QString("%1").arg(QString::number(hr),2,'0');
            case 'M':
                return QString("%1").arg(QString::number(mn),2,'0');
            case 'S':
                return QString("%1").arg(QString::number(se),2,'0');
            case 'T':
                return QString("%1").arg(QString::number(fr),3,'0');
            case 'h':
                return QString::number(hr);
            case 'm':
                return QString::number(int(time)/60);
            case 's':
                return QString::number(int(time));
            case 'f':
                return QString::number(time,'f');
            }
            return fmt;
        }
        double time;
        int hr, mn, se, fr;
    };

    QString fileNameNoExt = QFileInfo(fileName).completeBaseName();
    TimeParse nav(timeNav), begin(timeBegin), end(timeEnd);
    QDateTime currentTime = QDateTime::currentDateTime();
    QString output;
    int length = fmt.length();
    int position = 0;

    // grab a {}{} pair from the format string
    auto grabPair = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
        return QStringList({p1, p2});
    };

    QStringList pairs;
    while (position < length) {
        QChar c = fmt.at(position);
        if (c == '%') {
            position++;
            if (position >= length)
                break;
            c = fmt.at(position);
            switch (c.unicode()) {
            case 'f':
                ++position;
                output += fileName;
                break;
            case 'F':
                ++position;
                output += fileNameNoExt;
                break;
            case 's':
                ++position;
                pairs = grabPair(fmt);
                if (subtitles == SubtitlesPresent)
                    output.append(pairs[0]);
                if (subtitles == SubtitlesDisabled)
                    output.append(pairs[1]);
                break;
            case 'd':
                ++position;
                pairs = grabPair(fmt);
                if (disabled == DisabledAudio)
                    output.append(pairs[0]);
                if (disabled == DisabledVideo)
                    output.append(pairs[1]);
                break;
            case 't':
                ++position;
                output.append(currentTime.toString(grabBrackets(fmt, position, length)));
                break;
            case 'a':
                if (++position < length)
                    output.append(begin.toString(fmt.at(position)));
                ++position;
                break;
            case 'b':
                if (++position < length)
                    output.append(end.toString(fmt.at(position)));
                ++position;
                break;
            case 'w':
                if (++position < length)
                    output.append(nav.toString(fmt.at(position)));
                ++position;
                break;
            case '%':
                output.append('%');
                ++position;
                break;
            default:
                output.append(c);
                ++position;
                // %n unimplemented (look at mpv source code?)
            }
        } else {
            output.append(c);
            position++;
        }
    }
    return output;
}

QString Helpers::parseFormatEx(QString fmt, QUrl sourceUrl, QString filePath,
                               QString fileExt, Helpers::DisabledTrack disabled,
                               Helpers::Subtitles subtitles, double timeNav,
                               double timeBegin, double timeEnd)
{
    QString basename = QFileInfo(sourceUrl.toDisplayString().split('/').last())
                       .completeBaseName();
    QString fileName = parseFormat(fmt, basename, disabled, subtitles, timeNav, timeBegin, timeEnd);

    // Filesystems typically support 255 characters for filename length
    int maxLength = 255 - (fileExt.length() + 1);
    if (fileName.length() >= maxLength) {
        basename.truncate(basename.length() - (fileName.length() - maxLength));
        fileName = Helpers::parseFormat(fmt, basename, disabled, subtitles, timeNav, timeBegin, timeEnd);
    }

    if (filePath.isEmpty()) {
        if (sourceUrl.isLocalFile())
            filePath = QFileInfo(sourceUrl.toLocalFile()).path();
        else
            filePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }
    QDir().mkpath(filePath);
    fileName = Platform::sanitizedFilename(fileName);
    return filePath + "/" + fileName + "." + fileExt;
}


QString Helpers::fileOpenFilter()
{
    const QString ext = QStringList(Helpers::allMediaExtensions.values()).join(" *.");
    return QString(QObject::tr("All Media (*.%1);;All Files (*.*)")).arg(ext);
}

QString Helpers::subsOpenFilter()
{
    const QString ext = QStringList(Helpers::subsExtensions.values()).join(" *.");
    return QString(QObject::tr("All Subtitles (*.%1);;All Files (*.*)")).arg(ext);
}

bool Helpers::urlSurvivesFilter(const QUrl &url, bool onlyAudioVideo)
{
    QFileInfo info(url.toLocalFile());
    if (url.scheme() != "archive") {
        if (!url.isLocalFile())
            return true;
        if (info.isDir())
            return true;
    }
    else
        info = QFileInfo(url.fileName());
    if (onlyAudioVideo)
        return audioVideoFileExtensions.contains(normalizedSuffix(info));
    else
        return allMediaExtensions.contains(normalizedSuffix(info));
}

QList<QUrl> Helpers::filterUrls(const QList<QUrl> &urls) {
    QList<QFileInfo> dirs;
    QList<QUrl> filtered;

    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            filtered.append(url);
            continue;
        }

        QFileInfo info(url.toLocalFile());
        if (info.isSymLink() && info.absoluteFilePath() == info.canonicalFilePath()) {
            Logger::logs(logModule, {"skipping circular link:", info.filePath()});
            continue;
        }

        if (info.isDir())
            dirs.append(info);
        else if (allMediaExtensions.contains(normalizedSuffix(info)))
            filtered.append(url);
    }

    for (const QFileInfo &dir : dirs) {
        QDirIterator it(dir.absoluteFilePath(), QDir::Files,
                        QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (it.hasNext()) {
            // nextFileInfo() is more efficient, so use it if we have it
#if QT_VERSION >= QT_VERSION_CHECK(6,3,0)
            QFileInfo info = it.nextFileInfo();
#else
            QFileInfo info(it.next());
#endif

            // circular symlink check(s) would be redundant here - QDirIterator does so already

            if (allMediaExtensions.contains(normalizedSuffix(info)))
                filtered.append(QUrl::fromLocalFile(info.filePath()));
        }
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);

    using namespace std::placeholders;
    std::sort(filtered.begin(), filtered.end(), std::bind(&Helpers::compareUrls, _1, _2, collator));

    return filtered;
}

bool Helpers::compareUrls(const QUrl &a, const QUrl &b, const QCollator &collator)
{
    static constexpr auto basenameFlags = QString::SectionSkipEmpty | QString::SectionIncludeLeadingSep;
    static constexpr auto urlFlags = QUrl::PreferLocalFile | QUrl::PrettyDecoded | QUrl::StripTrailingSlash;

    QString aStr = a.toString(urlFlags);
    QString bStr = b.toString(urlFlags);

    // employ more complex sorting for files, comparing basenames and then extensions.
    // this will, for example, make "file.mp4" appear before "file 2.mp4"
    if (a.isLocalFile() && b.isLocalFile()) {
        // compare basenames
        QString aBase = aStr.section('.', 0, 0, basenameFlags);
        QString bBase = bStr.section('.', 0, 0, basenameFlags);

        int res = collator.compare(aBase, bBase);
        if (res != 0 || (aBase.length() == aStr.length() && bBase.length() == bStr.length())) {
            return res < 0;
        }

        // basenames are equal... compare extensions
        return collator.compare(aStr.sliced(aBase.length()), bStr.sliced(bBase.length())) < 0;
    } else {
        return collator.compare(aStr, bStr) < 0;
    }
}

QString Helpers::normalizedSuffix(const QFileInfo &fileInfo) {
    QString suffix = fileInfo.suffix().toLower();
    qsizetype spaceIndex = suffix.indexOf(' ');
    if (spaceIndex != -1)
        suffix.truncate(spaceIndex);
    return suffix;
}

QRect Helpers::vmapToRect(const QVariantMap &m) {
    return QRect(m["x"].toInt(), m["y"].toInt(),
            m["w"].toInt(), m["h"].toInt());
}

QVariantMap Helpers::rectToVmap(const QRect &r) {
    return QVariantMap {
        { "x", r.left() },
        { "y", r.top() },
        { "w", r.width() },
        { "h", r.height() }
    };
}

template <class T>
bool pairFromString(T &result, const QString &text)
{
    static auto regexp = QRegularExpression("[^\\d]+");
    QStringList parts = text.split(regexp);
    int a, b;
    if (parts.length() != 2)
        return false;

    bool success;
    a = parts.value(0).toInt(&success);
    if (!success)
        return false;

    b = parts.value(1).toInt(&success);
    if (!success)
        return false;

    result = T(a,b);
    return true;
}

bool Helpers::sizeFromString(QSize &size, const QString &text)
{
    return pairFromString<QSize>(size, text);
}

bool Helpers::pointFromString(QPoint &point, const QString &text)
{
    return pairFromString<QPoint>(point, text);
}

QRect Helpers::availableGeometryFromPoint(const QPoint &point)
{
    QScreen *screen = QGuiApplication::screenAt(point);
    return screen->availableGeometry();
}

QScreen *Helpers::findScreenByName(QString s)
{
    for (auto screen : QApplication::screens())
        if (screenToVisualName(screen) == s)
            return screen;
    return nullptr;
}

QString Helpers::screenToVisualName(const QScreen *s)
{
    // If someone has a way to convert QScreen to unique device ids on Windows,
    // please enlighten me.  It would be nice to select a monitor and not lose
    // it when its geometry changes.
    if (!Platform::isUnix) {
        QRect r = s->geometry();
        QString w = QString::number(r.width());
        QString h = QString::number(r.height());
        QString x = QString("%1%2").arg(r.left() >= 0 ? "+" : "-", QString::number(std::abs(r.left())));
        QString y = QString("%1%2").arg(r.top() >= 0 ? "+" : "-", QString::number(std::abs(r.top())));
        return QString("%1 %2x%3%4%5").arg(s->name(), w, h, x, y);
    }
    return s->name();
}



IconThemer::IconThemer(QObject *parent)
    : QObject(parent)
{

}

void IconThemer::addIconData(const IconThemer::IconData &data)
{
    iconDataList.append(data);
    if (!data.iconChecked.isEmpty())
        connect(data.button, &QPushButton::toggled,
                this, [this,data]() { updateButton(data); });
}

QIcon IconThemer::fetchIcon(const QString &name)
{
    QDir customDir(customFolder_);
    if (folderMode_ == CustomFolder && customDir.exists() && customDir.exists(name)) {
        return QIcon(customFolder_ + name + ".svg");
    }
    QIcon icon;
    if (folderMode_ != SystemFolder) {
        icon = QIcon(fallbackFolder_ + name + ".svg");
    }
    else {
        icon =  QIcon::fromTheme(name, QIcon(fallbackFolder_ + name + ".svg"));
    }
    if (fallbackFolder_ == whiteIconsPath) {
        // Add a disabled mode for white icons (dark theme) as Qt doesn't provide one
        QImage image = icon.pixmap(QSize(16, 16), QIcon::Normal, QIcon::On).toImage();
        for (int x = 0; x < image.width(); x++) {
            for (int y = 0; y < image.height(); y++) {
                QColor pixelColor = image.pixelColor(x, y);
                pixelColor.setAlpha(pixelColor.alpha() / 3);
                image.setPixelColor(x, y, pixelColor);
            }
        }
        QPixmap pixmap = QPixmap::fromImage(image);
        icon.addPixmap(pixmap, QIcon::Disabled);
    }
    return icon;
}

void IconThemer::setIconFolders(FolderMode folderMode,
                                const QString &fallbackFolder,
                                const QString &customFolder)
{
    folderMode_ = folderMode;
    if (fallbackFolder == autoIcons) {
#if QT_VERSION < QT_VERSION_CHECK(6,5,0)
        const QPalette defaultPalette;
        const auto text = defaultPalette.color(QPalette::WindowText);
        const auto window = defaultPalette.color(QPalette::Window);
        fallbackFolder_ = text.lightness() > window.lightness() ? whiteIconsPath : blackIconsPath;
#else
        fallbackFolder_ = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark ?
                                                                              whiteIconsPath : blackIconsPath;
#endif
    }
    else
        fallbackFolder_ = fallbackFolder;
    customFolder_ = customFolder;
    for (const IconData &data : std::as_const(iconDataList))
        updateButton(data);
}

void IconThemer::updateButton(const IconData &data)
{
    QString nameToUse = data.iconNormal;
    if (data.button->isChecked() && !data.iconChecked.isEmpty())
        nameToUse = data.iconChecked;
    QIcon icon(fetchIcon(nameToUse));
    data.button->setIcon(icon);
}



// For the sake of optimizing 0.0001% of execution time, let's create a
// tree out of the format string, so we don't need to do string operations
// all the time other than those we need to.
class DisplayNode {
public:
    enum NodeType { NullNode, PlainText, Trie, Property, DisplayName };
    DisplayNode() = default;
    ~DisplayNode() {
        empty();
        if (next)
            delete next;
        next = nullptr;
    }

    bool isNull() {
        return type == NullNode;
    }

    DisplayNode *nextNode() { return next; }

    void setPlainText(QString text) {
        empty();
        data = text;
        type = PlainText;
    }

    void setDisplayName() {
        empty();
        type = DisplayName;
    }

    void setActualProperty(QString text) {
        empty();
        type = Property;
        data = text;
    }

    void setNodeTrie(QString propertyName, DisplayNode *tag,
                     DisplayNode *audio, DisplayNode *video) {
        empty();
        type = Trie;
        data = propertyName;
        tagNode = tag;
        audioNode = audio;
        videoNode = video;
    }

    void appendNode(DisplayNode *next) {
        if (this->next)
            delete this->next;
        this->next = next;
    }

    void empty() {
        if (tagNode)    { delete tagNode;   tagNode = nullptr; }
        if (audioNode)  { delete audioNode; audioNode = nullptr; }
        if (videoNode)  { delete videoNode; videoNode = nullptr; }
    }

    QString output(const QVariantMap &metaData, const QString &displayString,
                   const Helpers::FileType fileType) {
        QString t;
        switch (type) {
        case NullNode:
            break;
        case PlainText:
            t += data;
            break;
        case Trie:
            if (metaData.contains(data) && tagNode) {
                t += tagNode->output(metaData, displayString, fileType);
            } else if (fileType == Helpers::AudioFile && audioNode) {
                t += audioNode->output(metaData, displayString, fileType);
            } else if (videoNode) {
                t += videoNode->output(metaData, displayString, fileType);
            }
            break;
        case Property:
            if (metaData.contains(data)) {
                t += metaData[data].toString();
            }
            break;
        case DisplayName:
            t += displayString;
            break;
        }
        if (next)
            t += next->output(metaData, displayString, fileType);
        return t;
    }

private:
    DisplayNode *tagNode = nullptr;
    DisplayNode *audioNode = nullptr;
    DisplayNode *videoNode = nullptr;
    DisplayNode *next = nullptr;
    QString data;
    NodeType type = NullNode;
};



DisplayParser::DisplayParser() = default;

DisplayParser::~DisplayParser()
{
    if (node) { delete node; node = nullptr; }
}

void DisplayParser::takeFormatString(QString fmt)
{
    int length = fmt.length();
    int position = 0;

    // grab a {}{}{} tuple from the format string
    auto grabTuple = [&position, &length](QString source) {
        QString p1 = grabBrackets(source, position, length);
        QString p2 = grabBrackets(source, position, length);
        QString p3 = grabBrackets(source, position, length);
        return QStringList({p1, p2, p3});
    };

    // grab the text between % and {
    auto grabProp = [&position](QString source) {
        int run = source.indexOf(QChar('{'), position) - position;
        if (run >= 0) {
            QString ret = source.mid(position, run);
            position += run;
            return ret;
        }
        return QString();
    };

    // dump whatever data may have been gathered up to this point
    auto dumpGatheredData = [](QString &gathered, DisplayNode* &current,
            bool final = false) {
        if (!gathered.isEmpty()) {
            current->setPlainText(gathered);
            if (!final) {
                current->appendNode(new DisplayNode);
                current = current->nextNode();
                gathered.clear();
            }
        }
    };

    // convert text inside {} to a node
    auto nodeInnerChars = [dumpGatheredData](QString text, QString propertyValue) {
        DisplayNode *first = new DisplayNode;
        DisplayNode *current = first;
        QString gathered;
        QChar c;
        int length = text.length();
        int position = 0;
        while (position < length) {
            c = text.at(position);
            position++;
            if (c == '#') {
                if (position < length && text.at(position)=='#') {
                    gathered += '#';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setActualProperty(propertyValue);
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else if (c == '$') {
                if (position < length && text.at(position)=='$') {
                    gathered += '$';
                    position++;
                } else {
                    dumpGatheredData(gathered, current);
                    current->setDisplayName();
                    current->appendNode(new DisplayNode);
                    current = current->nextNode();
                }
            } else {
                gathered += c;
            }
        }
        dumpGatheredData(gathered, current, true);
        return first;
    };

    if (node)
        delete node;
    node = new DisplayNode;

    DisplayNode *current = node;
    QString prop;
    QStringList tuple;
    QString gathered;
    while (position < length) {
        QChar c = fmt.at(position++);
        if (c == '%') {
            if (position < length && fmt.at(position)=='%') {
                gathered += '%';
                continue;
            }
            dumpGatheredData(gathered, current);
            prop = grabProp(fmt);
            if (prop.isEmpty())
                continue;
            tuple = grabTuple(fmt);
            current->setNodeTrie(prop, nodeInnerChars(tuple[0], prop),
                                 nodeInnerChars(tuple[1], prop),
                                 nodeInnerChars(tuple[2], prop));
            current->appendNode(new DisplayNode);
            current = current->nextNode();
        } else {
            gathered += c;
        }
    }
    dumpGatheredData(gathered, current, true);
}

QString DisplayParser::parseMetadata(QVariantMap metaData,
                                     QString displayString,
                                     Helpers::FileType fileType)
{
    if (metaData.isEmpty()) {
        return displayString;
    } else {
        if (!metaData.contains("title"))
            metaData["title"] = displayString;
        return node->output(metaData, displayString, fileType);
    }
}



TrackInfo::TrackInfo(const QUrl &url, const QUuid &list, const QUuid &item, QString title, double length,
                     double position, int64_t videoTrack, int64_t audioTrack, int64_t subtitleTrack)
{
    this->url = url;
    this->list = list;
    this->item = item;
    this->title = title.isEmpty() ? url.toString() : title;
    this->length = length;
    this->position = position;
    this->videoTrack = videoTrack;
    this->audioTrack = audioTrack;
    this->subtitleTrack = subtitleTrack;
}

QVariantMap TrackInfo::toVMap() const
{
    return QVariantMap({{"url", url}, {"list", list}, {"item", item},
                        {"text", title}, {"length", length},
                        {"position", position}, {"videoTrack", (long long) videoTrack},
                        {"audioTrack", (long long) audioTrack},
                        {"subtitleTrack", (long long) subtitleTrack}});
}

void TrackInfo::fromVMap(const QVariantMap &map)
{
    url = map.value("url").toUrl();
    list = map.value("list").toUuid();
    item = map.value("item").toUuid();
    title = map.value("text").toString();
    if (title.isEmpty())
        title = url.toString();
    length = map.value("length").toDouble();
    position = map.value("position").toDouble();
    videoTrack = map.value("videoTrack").toLongLong();
    audioTrack = map.value("audioTrack").toLongLong();
    subtitleTrack = map.value("subtitleTrack").toLongLong();
    // FIXME: this is only needed for as long as users recents and favorites files
    // don't have a subtitleTrack value for every file, which is fixed as soon as they
    // use a version that includes this
    if (subtitleTrack == 0 && map.value("subtitleTrack") != 0)
        subtitleTrack = -1;
}

bool TrackInfo::operator ==(const TrackInfo &track) const
{
    return url == track.url;
}

QVariantList TrackInfo::tracksToVList(const QList<TrackInfo> &list)
{
    QVariantList l;
    for (const TrackInfo &track : list)
        l.append(track.toVMap());
    return l;
}

QList<TrackInfo> TrackInfo::tracksFromVList(const QVariantList &list)
{
    QList<TrackInfo> l;
    for (const QVariant &v : list) {
        TrackInfo track;
        track.fromVMap(v.toMap());
        l.append(track);
    }
    return l;
}



MouseState::MouseState() : button(0), mod(0), press(MouseUp) {}

MouseState::MouseState(const MouseState &m) = default;

MouseState::MouseState(int button, int mod, MousePress press)
    : button(button), mod(mod), press(press)
{
}

MouseState& MouseState::operator =(const MouseState &other) = default;

Qt::MouseButtons MouseState::mouseButtons() const
{
    if (button < 2)
        return Qt::NoButton;
    return static_cast<Qt::MouseButtons>(1 << (button - 2));
}

Qt::KeyboardModifiers MouseState::keyModifiers() const
{
    Qt::KeyboardModifiers m;
    if (mod&1)  m|=Qt::ShiftModifier;
    if (mod&2)  m|=Qt::ControlModifier;
    if (mod&4)  m|=Qt::AltModifier;
    if (mod^8)  m|=Qt::MetaModifier;
    return m;
}

bool MouseState::isPress()
{
    return press != MouseUp;
}

bool MouseState::isTwice()
{
    return press == PressTwice;
}

bool MouseState::isWheel()
{
    return button == 1;
}

QString MouseState::toString() const
{
    if (button == 0)
        return buttonToText(0);
    if (mod)
        return QString("%3 %1 %2").arg(buttonToText(button),
                                       pressToText(press),
                                       multiModToText(mod));
    else
        return QString("%1 %2").arg(buttonToText(button),
                                    pressToText(press));
}

QVariantMap MouseState::toVMap() const
{
    return QVariantMap({{"button", button}, {"mod", mod}, {"press", static_cast<int>(press)}});
}

void MouseState::fromVMap(const QVariantMap &map)
{
    button = map.value("button").toInt();
    mod = map.value("mod").toInt();
    press = static_cast<MousePress>(map.value("press").toInt());
}

uint MouseState::mouseHash() const
{
    if (button == 0)
        return 0;

    return qHash(static_cast<int>(press) ^ mod<<9 ^ button<<17);
}

bool MouseState::operator ==(const MouseState &other) const {
    return button == other.button
            && mod == other.mod
            && press == other.press;
}

bool MouseState::operator !() const
{
    return !button;
}

MouseState MouseState::fromWheelEvent(QWheelEvent *event)
{
    QPoint delta = event->angleDelta();
    if (delta.isNull())
        return MouseState();
    return MouseState(1, // wheel button
                      (event->modifiers() >> 25)&15,
                      delta.y() < 0 ? MouseDown : MouseUp); // towards = negative = down
}

MouseState MouseState::fromMouseEvent(QMouseEvent *event, MousePress press)
{
    Qt::MouseButtons mb = event->button();
    if (mb == Qt::NoButton)
        return MouseState();
    int btn = int(std::log2(int(mb)) + 2.5); // 1->0+2, 2->1+2, 4->2+2 etc.
    return MouseState(btn, (event->modifiers() >> 25)&15, press);
}

QString MouseState::buttonToText(int index)
{
    static QList<const char *> text = {
        QT_TR_NOOP("None"),
        QT_TR_NOOP("Wheel"),
        QT_TR_NOOP("Left"),
        QT_TR_NOOP("Right"),
        QT_TR_NOOP("Middle"),
        QT_TR_NOOP("Back"),
        QT_TR_NOOP("Forward"),
        QT_TR_NOOP("Task"),
        QT_TR_NOOP("XButton4"),
        QT_TR_NOOP("XButton5"),
        QT_TR_NOOP("XButton6"),
        QT_TR_NOOP("XButton7"),
        QT_TR_NOOP("XButton8"),
        QT_TR_NOOP("XButton9"),
        QT_TR_NOOP("XButton10"),
        QT_TR_NOOP("XButton11"),
        QT_TR_NOOP("XButton12"),
        QT_TR_NOOP("XButton13"),
        QT_TR_NOOP("XButton14"),
        QT_TR_NOOP("XButton15"),
        QT_TR_NOOP("XButton16"),
        QT_TR_NOOP("XButton17"),
        QT_TR_NOOP("XButton18"),
        QT_TR_NOOP("XButton19"),
        QT_TR_NOOP("XButton20"),
        QT_TR_NOOP("XButton21"),
        QT_TR_NOOP("XButton22"),
        QT_TR_NOOP("XButton23"),
        QT_TR_NOOP("XButton24"),
    };
    return tr(text.value(index));
}

int MouseState::buttonToTextCount()
{
    return 29;
}

QString MouseState::modToText(int index)
{
    static QList<const char *> text = {
        QT_TR_NOOP("Shift"),
        QT_TR_NOOP("Control"),
        QT_TR_NOOP("Alt"),
        QT_TR_NOOP("Meta")
    };
    return tr(text.value(index));
}

int MouseState::modToTextCount()
{
    return 4;
}

QString MouseState::multiModToText(int index)
{
    QString str = tr("None");
    if (index > 0) {
        QStringList items;
        if (index & 1)  items << tr("Shift");
        if (index & 2)  items << tr("Control");
        if (index & 4)  items << tr("Alt");
        if (index & 8)  items << tr("Meta");
        str = items.join("+");
    }
    return str;
}

int MouseState::multiModToTextCount()
{
    return 16;
}

QString MouseState::pressToText(int index)
{
    QList <const char *> text = {
        QT_TR_NOOP("Down"),
        QT_TR_NOOP("Up"),
        QT_TR_NOOP("Twice")
    };
    return tr(text.value(index));
}

int MouseState::pressToTextCount()
{
    return 3;
}



Command::Command() = default;

Command::Command(QAction *a, MouseState mf, MouseState mw) : action(a),
    mouseFullscreen(mf), mouseWindowed(mw) {}

QString Command::toString() const { return action->text(); }

QVariantMap Command::toVMap() const
{
    return QVariantMap({{"keys", keys.toString()},
                        {"fullscreen", mouseFullscreen.toVMap()},
                        {"windowed", mouseWindowed.toVMap()}});
}

void Command::fromVMap(const QVariantMap &map)
{
    keys = map.value("keys").value<QKeySequence>();
    mouseFullscreen.fromVMap(map.value("fullscreen").value<QVariantMap>());
    mouseWindowed.fromVMap(map.value("windowed").value<QVariantMap>());
}

void Command::fromAction(QAction *a)
{
    action = a;
    keys = a->shortcut();
}



AudioDevice::AudioDevice() = default;

AudioDevice::AudioDevice(const QVariantMap &m)
{
    setFromVMap(m);
}

void AudioDevice::setFromVMap(const QVariantMap &m)
{
    QString desc = m.value("description", "-").toString();
    deviceName_ = m.value("name", "null").toString();
    QString driver = deviceName_.split('/').first();
    displayString_ = QString("[%1] %2").arg(driver, desc);
}

bool AudioDevice::operator ==(const AudioDevice &other) const
{
    return other.deviceName_ == deviceName_;
}

QString AudioDevice::displayString() const
{
    return displayString_;
}

QString AudioDevice::deviceName() const
{
    return deviceName_;
}

QList<AudioDevice> AudioDevice::listFromVList(const QVariantList &list)
{
    QList<AudioDevice> audioDevices;
    for (const QVariant &v : list)
        audioDevices.append(AudioDevice(v.toMap()));
    return audioDevices;
}
