#ifndef PROPERTIESWINDOW_H
#define PROPERTIESWINDOW_H

#include <QDateTime>
#include <QDialog>
#include <QVariantList>
#include <QVariantMap>

namespace Ui {
class PropertiesWindow;
}

class PropertiesWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PropertiesWindow(QWidget *parent = nullptr);
    ~PropertiesWindow();

signals:
    void artistAndTitleChanged(QString artistAndTitle, QString filename);

public slots:
    void setFileName(const QString &filename);
    void setFileFormat(const QString &format);
    void setFileSize(const int64_t &bytes);
    void setMediaLength(double time);
    void setVideoSize(const QSize &sz);
    void setFileModifiedTime(const QUrl &file);
    void setTracks(const QVariantList &tracks);
    void setMediaTitle(const QString &title);
    void setFilePath(const QString &path);
    void setMetaData(QVariantMap data);
    void setChapters(const QVariantList &chapters);

private slots:
    void on_save_clicked();
    void updateSaveVisibility();

private:
    void updateLastTab();
    QString generalDataText();
    QString sectionText(const QString &header, const QVariantMap &fields);

    Ui::PropertiesWindow *ui;

    QString filename;
    QVariantMap generalData;
    QString trackText;
    QString chapterText;
};

#endif // PROPERTIESWINDOW_H
