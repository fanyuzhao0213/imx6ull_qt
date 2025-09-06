#ifndef MUSICMODULE_H
#define MUSICMODULE_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QMediaMetaData>
#include <QPixmap>


/*
 * MusicPlayer
 * 功能：
 *  - 支持扫描磁盘目录 MP3
 *  - 支持扫描 Qt 资源目录 MP3
 *  - 支持从配置文件加载
 *  - 播放 / 暂停 / 停止 / 上一首 / 下一首
 *  - 设置音量
 *  - 播放完自动切下一首
 */
class MusicPlayer : public QObject
{
    Q_OBJECT
public:
    explicit MusicPlayer(QObject *parent = nullptr);

    // 扫描磁盘目录 MP3 文件
    void scanDiskFolder(const QString &folderPath);

    // 扫描资源列表
    void scanResourceFolder(const QStringList &resourceFiles);

    // 从资源索引文件加载
    void scanResourceFolderFromIndex(const QString &indexFilePath);

    // 从配置文件加载磁盘目录和资源文件
    void loadFromConfig(const QString &configFilePath);

    void play();
    void pause();
    void stop();
    void next();
    void previous();
    int getVolume();    // 获取当前音量
    void setVolume(int vol);
    void togglePlay();   // 播放/暂停切换
    void togglePlaybackMode();//播放模式
    void debugIniFileContent(const QString &filePath);

    void seek(int position); // 调整播放位置
signals:
    // 播放器信号，用于 UI 更新
    void durationChanged(qint64 duration);       // 总时间变化
    void positionChanged(qint64 position);       // 当前播放位置变化
    void songInfoUpdated(const QString &title,
                         const QString &artist,
                         const QString &composer,
                         const QPixmap &cover);
private slots:
    void updateSongInfo();          // 内部槽函数，用于获取当前媒体信息
private:
    QMediaPlaylist *playlist;       //播放列表
    QMediaPlayer *player;           //播放器

    QString m_currentFile;
    void log(const QString &msg);
};


#endif // MUSICMODULE_H
