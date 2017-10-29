// generic helper module for general-use static functions
#ifndef HELPERS_H
#define HELPERS_H
#include <QObject>
#include <QWidget>
#include <QList>
#include <QUrl>
#include <QUuid>
#include <QOpenGLWidget>
#include <QDate>
#include <QTime>
#include <QDir>

class QPushButton;
class QFileDialog;
class QLocalServer;
class QLocalSocket;
class QOpenGLTexture;

namespace Helpers {
    QString toDateFormat(double time);
    QDate dateFromCFormat(const char date[]);
    QTime timeFromCFormat(const char time[]);
    enum DisabledTrack { NothingDisabled, DisabledAudio, DisabledVideo };
    enum Subtitles { NoSubtitles, SubtitlesPresent, SubtitlesDisabled, };
    enum FileType { AudioFile, VideoFile };
    enum ScreenshotRender { VideoRender, SubsRender, WindowRender };
    QString parseFormat(QString fmt, QString fileName, DisabledTrack disabled,
                        Subtitles subtitles, double timeNav, double timeBegin,
                        double timeEnd);
    QRect vmapToRect(const QVariantMap &m);
    QVariantMap rectToVmap(const QRect &r);
    bool sizeFromString(QSize &size, const QString &text);
    bool pointFromString(QPoint &point, const QString &text);

    enum TitlePrefix { PrefixFullPath, PrefixFileName, NoPrefix };
    enum MpvWidgetType { NullWidget, EmbedWidget, GlCbWidget, VulkanCbWidget };
    enum ControlHiding { NeverShown, ShowWhenMoving, ShowWhenHovering,
                         AlwaysShow };
    enum AfterPlayback { DoNothingAfter, RepeatAfter, PlayNextAfter,
                         ExitAfter, StandByAfter, HibernateAfter,
                         ShutdownAfter, LogOffAfter, LockAfter };
}

class IconThemer : public QObject {
    Q_OBJECT
public:
    class IconData {
    public:
        IconData(QPushButton *b, QString on, QString off = QString()) :
             button(b), iconNormal(on), iconChecked(off) {}
        QPushButton *button; QString iconNormal; QString iconChecked;
    };
    explicit IconThemer(QObject *parent = 0);
    void addIconData(const IconData &data);
    QIcon fetchIcon(const QString &name);

public slots:
    void setIconFolders(const QString &fallbackFolder, const QString &customFolder);

private:
    QList<IconData> iconDataList;
    QString fallback;
    QString custom;
};

class LogoDrawer : public QObject {
    Q_OBJECT
public:
    explicit LogoDrawer(QObject *parent = 0);
    ~LogoDrawer();
    void setLogoUrl(const QString &filename);
    void setLogoBackground(const QColor &color);
    void resizeGL(int w, int h);
    void paintGL(QOpenGLWidget *widget);

signals:
    void logoSize(QSize size);

private:
    void regenerateTexture();

private:
    QRectF logoLocation;
    QImage logo;
    QString logoUrl;
    QColor logoBackground;
};

class LogoWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit LogoWidget(QWidget *parent = 0);
    ~LogoWidget();
    void setLogo(const QString &filename);
    void setLogoBackground(const QColor &color);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

private:
    LogoDrawer *logoDrawer = nullptr;
    QString logoUrl;
    QColor logoBackground;
};

class DisplayNode;
class DisplayParser {
public:
    DisplayParser();
    ~DisplayParser();

    void takeFormatString(QString fmt);
    QString parseMetadata(QVariantMap metaData, QString displayString,
                          Helpers::FileType fileType);
private:
    DisplayNode *node = nullptr;
};

class TrackInfo {
public:
    TrackInfo() {}
    TrackInfo(const QUrl &url, const QUuid &list, const QUuid &item, QString text, double length, double position);
    QUrl url;
    QUuid list;
    QUuid item;
    QString text;
    double length;
    double position;
    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &map);
    bool operator ==(const TrackInfo &track) const;

    static QVariantList tracksToVList(const QList<TrackInfo> &list);
    static QList<TrackInfo> tracksFromVList(const QVariantList &list);
};

class MouseState {
public:
    enum MouseButtons { None, Wheel, Left, Right, Middle, Back,
        Forward, Task, XButton4, XButton5, XButton6, XButton7,
        XButton8, XButton9, XButton10, XButton11, XButton12,
        XButton13, XButton14, XButton15, XButton16, XButton17,
        XButton18, XButton19,XButton20, XButton21, XButton22,
        XButton23, XButton24 };
    enum MousePress { MouseDown, MouseUp, PressTwice };

    MouseState();
    MouseState(const MouseState &m);
    MouseState(int button, int mod, MousePress press);
    MouseState operator =(const MouseState &other);

    // Components
    int button;
    int mod;
    MousePress press;

    // to Qt notation functions
    Qt::MouseButtons mouseButtons() const;
    Qt::KeyboardModifiers keyModifiers() const;
    bool isPress();
    bool isTwice();
    bool isWheel();

    // I/O functions
    QString toString() const;
    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &map);

    // Hashing-related functions
    uint mouseHash() const;
    bool operator ==(const MouseState &other) const;
    bool operator !() const;        // le saef bull eyediom faec

    // Conversion functions
    static MouseState fromWheelEvent(QWheelEvent *event);
    static MouseState fromMouseEvent(QMouseEvent *event, MousePress press);

    // Display mapping vars
    static QStringList buttonToText;
    static QStringList modToText;
    static QStringList multiModToText;
    static QStringList pressToText;
};

inline uint qHash(const MouseState &m, uint seed) {
    Q_UNUSED(seed);
    return m.mouseHash();
}

typedef QHash<MouseState, QAction*> MouseStateMap;

class Command {
public:
    Command();
    Command(QAction *a, MouseState mf, MouseState mw);

    // Components
    QAction *action = nullptr;
    QKeySequence keys;      // taken from the QAction in constructor
    MouseState mouseFullscreen;
    MouseState mouseWindowed;

    // I/O functions
    QString toString() const;
    QVariantMap toVMap() const;
    void fromVMap(const QVariantMap &map);

    // Conversion functions
    void fromAction(QAction *a);
};



class AudioDevice {
public:
    AudioDevice();
    AudioDevice(const QVariantMap &m);
    void setFromVMap(const QVariantMap &m);

    bool operator ==(const AudioDevice &other) const;
    QString displayString() const;
    QString deviceName() const;

    static QList<AudioDevice> listFromVList(const QVariantList &list);

private:
    QString displayString_;
    QString deviceName_;
};




#endif // HELPERS_H
