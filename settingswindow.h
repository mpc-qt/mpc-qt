#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QWidget>
#include <QTreeWidgetItem>

class settings {
    // Player page
    enum OpenMode { OpenSame, OpenInNew };
    OpenMode playerOpenMode;
    bool useTrayIcon;
    bool showOSD;
    bool disableDiscMenu;
    enum TitleBarMode { FullPath, FileNameOnly, DontPrefix };
    TitleBarMode titleBar;
    bool replaceFileNameInTitleBar;
    bool rememberLastPlaylist;
    bool rememberWindowPosition;
    bool rememberWindowSize;
    bool rememberPanScanZoom;

    // TODO: formats, keys

    // Logo page
    enum LogoDisplay { InternalLogo, ExternalLogo };
    LogoDisplay logoDisplayed;
    QString logoLocation;

    // Playback page
    enum PlaybackProgression { PlayTimes, PlayForever };
    PlaybackProgression playbackProgress;
    int playbackTimes;
    bool rewindPlaylist;
    int volumeStep;
    int speedStep;
    QString subtitleTracks;
    QString audioTracks;
    enum AutoZoomMethod { Zoom50, Zoom100, Zoom200, AutoFit, AutoFitLarger };
    AutoZoomMethod zoomMethod;
    int autoFitFactor;
    bool autoZoom;
    bool autoloadAudio;
    bool autoloadSubtitles;
    int audioBalance;
    int audioVolume;

    // Output page
    enum VideoBackend { AutoBackend, GLXBackend, WaylandBackend, EGLBackend };
    VideoBackend videoBackend;
    enum FBDepth { DepthOf8, DepthOf10, DepthOf12, DepthOf16, DepthOf16F,
                   DepthOf32f };
    FBDepth framebufferDepth;
    bool framebufferAlpha;
    enum AlphaMode { BlendAlpha, UseAlpha, IgnoreAlpha };
    AlphaMode alphaMode;
    double sharpen;

    bool dither;
    int ditherDepth;
    enum DitherType { Fruit, Ordered, NoDithering };
    DitherType ditherType;
    bool temporalDither;
    int temporalPeriod;

    bool downscaleCorrectly;
    bool scaleInLinearLight;
    bool temporalInterpolation;
    bool sigmoidizedUpscaling;
    double sigmoidCenter;
    double sigmoidSlope;

    enum ScaleScalar { Unset = -1, Bilinear, BicubicFast, Oversample,
                       Spline16, Spline36, Spling64, Sinc, Lanczos, Gingseng,
                       Jinc, EwaLanczos, EwaHanning, EwaGinseng,
                       EwaLanczosSharp, HaasnSoft, Bicubic, BcSpline,
                       CatmullRom, Mitchell, Robidoux, RobidouxSharp,
                       EwaRobidoux, EwaRobidouxSharp, Box, Nearest, Triangle,
                       Gaussian };
    ScaleScalar scaleScalar;
    double scaleParam1;
    double scaleParam2;
    double scaleRadius;
    double scaleAntiRing;
    double scaleBlur;
    double scaleWindowParam;
    bool scaleParam1Set;
    bool scaleParam2Set;
    bool scaleRadiusSet;
    bool scaleAntiRingSet;
    bool scaleBlurSet;
    bool scaleWindowParamSet;
    enum ScaleWindow { BoxWindow = 0, TriangleWindow, BartlettWindow,
                       HanningWindow, HammingWindow, QuadricWindow,
                       WelchWindow, KaiserWindow, BlackmanWindow,
                       GaussianWindow, SincWindow, JincWindow, SphinxWindow };
    ScaleWindow scaleWindow;
    bool scaleWindowSet;
    bool scaleClamp;

    ScaleScalar dscaleScalar;
    double dscaleParam1;
    double dscaleParam2;
    double dscaleRadius;
    double dscaleAntiRing;
    double dscaleBlur;
    double dscaleWindowParam;
    bool dscaleParam1Set;
    bool dscaleParam2Set;
    bool dscaleRadiusSet;
    bool dscaleAntiRingSet;
    bool dscaleBlurSet;
    bool dscaleWindowParamSet;
    ScaleWindow dscaleWindow;
    bool dscaleWindowSet;
    bool dscaleClamp;

    ScaleScalar cscaleScalar;
    double cscaleParam1;
    double cscaleParam2;
    double cscaleRadius;
    double cscaleAntiRing;
    double cscaleBlur;
    double cscaleWindowParam;
    bool cscaleParam1Set;
    bool cscaleParam2Set;
    bool cscaleRadiusSet;
    bool cscaleAntiRingSet;
    bool cscaleBlurSet;
    bool cscaleWindowParamSet;
    ScaleWindow cscaleWindow;
    bool cscaleWindowSet;
    bool cscaleClamp;

    enum TimeScalar { tOversample, tSpline16, tSpline64, tSinc, tLanczos,
                      tGingseng, tCatmullRom, tMitchell, tRobidoux,
                      tRobidouxSharp, tBox, tNearest, tTriangle, tGaussian };
    TimeScalar tscaleScalar;
    double tscaleParam1;
    double tscaleParam2;
    double tscaleRadius;
    double tscaleAntiRing;
    double tscaleBlur;
    double tscaleWindowParam;
    bool tscaleParam1Set;
    bool tscaleParam2Set;
    bool tscaleRadiusSet;
    bool tscaleAntiRingSet;
    bool tscaleBlurSet;
    bool tscaleWindowParamSet;
    ScaleWindow tscaleWindow;
    bool tscaleWindowSet;
    bool tscaleClamp;

    enum Prescalar { NoPrescalar, SuperXbr, Nnedi3 };
    Prescalar prescalar;
    int prescalarPasses;
    double prescalarThreshold;

    double superxbrSharpness;
    double superxbrEdgeStrength;
    enum Nnedi3Neurons { NeuronsOf16, NeuronsOf32, NeuronsOf64, NeuronsOf128 };
    Nnedi3Neurons nnedi3Neurons;
    enum Nnedi3Window { WindowOf8x4, WindowOf8x6 };
    Nnedi3Window nnedi3Window;
    enum Nnedi3UploadMethod { UboUploading, ShaderUploading };

    bool debanding;
    int debandIterations;
    double debandThreshold;
    double debandRange;
    double debandGrain;

    double gamma;
    enum TargetPrim { AutoPrim, BT470MPrim, BT601525Prim, BT601625Prim,
                      BT709Prim, BT2020Prim, ApplePrim, AdobePrim,
                      ProPhotoPrim, CIE1931Prim };
    TargetPrim targetPrim;
    enum TargetTrc { AutoTrc, BT1886Trc, sRGBTrc, LinearTrc, Gamma18Trc,
                     Gamma22Trc, Gamma28Trc, ProPhotoTrc };
    bool iccAutodetect;
    QString iccLocation;

    enum AudioRenderer { PulseAudio, Alsa, Oss, NullAudio };
    AudioRenderer audioRenderer;
    QString pulseHost;
    QString pulseSink;
    int pulseBuffer;
    bool pulseLatencyHack;
    QString alsaMixer;
    QString alsaMixerDeice;
    QString alsaMixerName;
    int alsaMixerIndex;
    bool alsaResample;
    bool alsaIgnoreChmap;
    QString OssDspDevice;
    QString OssMixerDevice;
    QString OssMixerChannel;
    bool nullIsUntimed;
    int nullOutburst;
    double nullBufferLength;

    // Shaders page
    QStringList shaderFiles;
    QStringList preShaders;
    QStringList postShaders;
    QMap<QString, QPair<QStringList, QStringList>> shaderPresets;

    // Fullscreen page
    QString fullscreenMonitor;
    bool fullscreenLaunch;
    bool exitFullscreenAtEnd;
    bool hidePanelsInFullscreen;
    bool hideControlsInFullscreen;
    enum FullscreenShow { NeverShow, ShowWhenMoving, ShowWhenHovering };
    int controlShowDuration;

    struct XrandrMode {
        double from;
        double to;
        bool enabled;
        QString modeLine;
    };
    QList<XrandrMode> xrandrModes;
    int xrandrChangeDelay;
    bool switchXrandrBackToOldMode;
    bool restoreXrandrOnExit;

    // Sync Page
    enum FramedropMode { NeverDrop, VideoDrops, DecoderDrops,
                         DecoderAndVideoDrops };
    FramedropMode framedroppingMode;
    enum DecoderDropMode { DecoderNeverDrops, DecoderDefault,
                           DropOnNonReferenceFrames, BiDirectionalDrops,
                           DropOnNonKeyFrames, DropAtAnyTime };
    DecoderDropMode decoderDroppingMode;
    enum SyncMode { SyncToAudio, ResampleAudio, ResampleAudioAndDrop,
                    RepeatVideo, RepeatAudio };
    SyncMode syncMode;
    double audioDropSize;
    double maxAudioChange;
    double maxVideoChange;

    // Subtitles Page
    enum SubtitlePlacementX { SubtitlesOnLeft, SubtitlesInCenter,
                              SubtitlesOnRight };
    enum SubtitlePlacementY { SubtitlesAtTop, SubtitlesAtCenter,
                              SubtitlesAtBottom };
    SubtitlePlacementX subPlacementX;
    SubtitlePlacementY subPlacementY;
    bool overrideSubPlacement;
    int subtitlePosition;
    bool subtitlesUseMargins;

    bool subtitlesInGrayscale;
    bool subtitlesWithFixedTiming;
    bool subtitlesClearOnSeek;
    enum AssOverride { NoOverride, Override, OverrideOnlyZoomSigns,
                       ForcedOverride };
    AssOverride assOverrideMode;

    // Subtitles Defaults Page
    QString subsFont;
    bool fontBold;
    double fontSize;
    double borderSize;
    double borderShadowOffset;

    enum SubsAlignment { Top, TopRight, Right, BottomRight, Bottom,
                         BottomLeft, Left, TopLeft, Center };
    SubsAlignment subsAlignment;
    int subsMarginX;
    int subsMarginY;
    bool subsAreRelativeToVideoFrame;
    QColor subsColor;
    QColor borderColor;
    QColor shadowColor;

    // Subtitles Misc Page
    bool preferDefaultSubs;
    bool preferExternalSubs;
    bool ignoreEmbeddedSubs;
    QString subtitleDatabase;

    // Tweaks Page
    bool seekFast;
    bool showChapterMarks;
    bool openNextFile;
    bool showTimeTooltip;
    enum TimeTooltipLocation { AboveSeekbar, BelowSeekbar };
    TimeTooltipLocation timeTooltipLocation;
    QString osdFont;
    int osdSize;

    // Misc Page
    int brightness;
    int contrast;
    int hue;
    int saturation;
};

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget *parent = 0);
    ~SettingsWindow();

private slots:
    void on_pageTree_itemSelectionChanged();

private:
    Ui::SettingsWindow *ui;
};

#endif // SETTINGSWINDOW_H
