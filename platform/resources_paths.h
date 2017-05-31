// We shall add more platform-specific paths here, e.g. for icons, themes, skins et cetera

#if defined(Q_OS_LINUX)

#define APP_LANG_PATH QString("/usr/share/mpc-qt/translations/")

#elif defined(Q_OS_OSX)

#define APP_LANG_PATH QApplication::applicationDirPath() + QString("/../Resources/translations/")

#elif defined(Q_OS_WIN)

#define APP_LANG_PATH QApplication::applicationDirPath() + QString("\\translations\\")

#endif
