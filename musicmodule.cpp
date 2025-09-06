#include "musicmodule.h"

#include <QDir>
#include <QDebug>

MusicPlayer::MusicPlayer(QObject *parent)
    : QObject(parent)
{
    player = new QMediaPlayer(this);
    playlist = new QMediaPlaylist(this);
    player->setPlaylist(playlist);
    playlist->setPlaybackMode(QMediaPlaylist::Loop);

    log("MusicPlayer 初始化完成");

    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status){
        switch(status) {
        case QMediaPlayer::LoadedMedia:
            log("媒体加载完成");
            break;
        case QMediaPlayer::EndOfMedia:
            log("当前歌曲播放结束，准备播放下一首");
            break;
        case QMediaPlayer::InvalidMedia:
            log("媒体无效");
            break;
        default: break;
        }
    });

    // 在构造函数或初始化播放器时
    connect(player, SIGNAL(metaDataChanged()), this, SLOT(updateSongInfo()));


    connect(player, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
        switch(state) {
        case QMediaPlayer::PlayingState:
            log("开始播放");
            break;
        case QMediaPlayer::PausedState:
            log("暂停播放");
            break;
        case QMediaPlayer::StoppedState:
            log("停止播放");
            break;
        }
    });

    connect(playlist, &QMediaPlaylist::currentIndexChanged, this, [=](int index){
        log(QString("播放列表切换到索引 %1").arg(index));
    });


    // QMediaPlayer 总时间变化时发射信号
    connect(player, &QMediaPlayer::durationChanged, this, &MusicPlayer::durationChanged);
    // QMediaPlayer 播放位置变化时发射信号
    connect(player, &QMediaPlayer::positionChanged, this, &MusicPlayer::positionChanged);

}

// -------------------- 磁盘目录扫描 --------------------
void MusicPlayer::scanDiskFolder(const QString &folderPath)
{
    QDir dir(folderPath);
    QStringList filters;
    filters << "*.mp3" << "*.MP3";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    // 不要清空 playlist，这样不会覆盖之前添加的歌曲
    for (const QFileInfo &fileInfo : fileList) {
        playlist->addMedia(QUrl::fromLocalFile(fileInfo.absoluteFilePath()));
        log("添加磁盘文件到播放列表:" + fileInfo.fileName());
    }

    log(QString("磁盘扫描完成，共 %1 首歌曲").arg(fileList.size()));
}


// -------------------- 资源列表扫描 --------------------
void MusicPlayer::scanResourceFolder(const QStringList &resourceFiles)
{
    for(const QString &resPath : resourceFiles) {
        playlist->addMedia(QUrl(resPath));
        log("添加资源文件到播放列表:" + resPath);
    }

    log(QString("资源扫描完成，共 %1 首歌曲").arg(resourceFiles.size()));
}

void MusicPlayer::debugIniFileContent(const QString &filePath)
{
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        qDebug() << "[MusicPlayer] INI文件内容:";
        int lineNum = 1;
        while (!in.atEnd()) {
            QString line = in.readLine();
            qDebug() << QString("[%1] %2").arg(lineNum, 2).arg(line);
            lineNum++;
        }
        file.close();
    }
}

void MusicPlayer::loadFromConfig(const QString &configFilePath)
{
//    qDebug() << "[MusicPlayer] 尝试读取配置文件:" << configFilePath;

//    debugIniFileContent(configFilePath);

    QString realPath;
    QFile file(configFilePath);

    if (file.exists()) {
        realPath = configFilePath;
        qDebug() << "[MusicPlayer] 配置文件在磁盘";
    } else {
        qDebug() << "[MusicPlayer] 配置文件不存在，加载失败";
        return;
    }

    // 使用 QSettings 读取 ini 文件
    QSettings settings(realPath, QSettings::IniFormat);
//    qDebug() << "[MusicPlayer] all keys:" << settings.allKeys();

    // **在这里统一清空播放列表**
    playlist->clear();

    // -------------------- 扫描磁盘目录 --------------------
    QString foldersStr = settings.value("Disk/folders", "").toString();
    qDebug() << "[MusicPlayer] Disk/folders 值:" << foldersStr;

    if (!foldersStr.isEmpty()) {
        QStringList folders = foldersStr.split(",", QString::SkipEmptyParts);
        for (const QString &folder : folders) {
            QString path = folder.trimmed();
            qDebug() << "[MusicPlayer] 正在扫描磁盘目录:" << path;
            scanDiskFolder(path);
        }
    } else {
        qDebug() << "[MusicPlayer] 没有找到磁盘目录";
    }

    // -------------------- 扫描资源文件 --------------------
    QString resFilesStr = settings.value("Resource/files", "").toString().trimmed();
    qDebug() << "[MusicPlayer] Resource/files 原始值:" << resFilesStr;

    if (!resFilesStr.isEmpty()) {
        QStringList resFiles = resFilesStr.split(",", QString::SkipEmptyParts);
        QStringList finalPaths;

        for (QString rawPath : resFiles) {
            rawPath = rawPath.trimmed();

            QString fixedPath;
            // 如果是以 /src 开头，自动转换成 qrc:/ 路径
            if (rawPath.startsWith(":/src", Qt::CaseInsensitive)) {
                fixedPath = "qrc" + rawPath;  // /src/music/xxx.mp3 -> qrc:/src/music/xxx.mp3
            }
            // 如果已经是 qrc:/ 直接用
            else if (rawPath.startsWith("qrc:/", Qt::CaseInsensitive)) {
                fixedPath = rawPath;
            }
            // 否则当作本地磁盘路径
            else {
                fixedPath = rawPath;
            }

            qDebug() << "[MusicPlayer] 正在加载资源文件:" << fixedPath;

            if (!QFile::exists(fixedPath)) {
                qDebug() << "[Warning] 资源文件不存在:" << fixedPath;
            }

            finalPaths.append(fixedPath);
        }

        scanResourceFolder(finalPaths);

        qDebug() << "[MusicPlayer] 资源扫描完成，共" << finalPaths.size() << "首歌曲";
    } else {
        qDebug() << "[MusicPlayer] 没有找到资源文件";
    }

    qDebug() << "[MusicPlayer] 当前播放列表总计" << playlist->mediaCount() << "首歌曲";
    qDebug() << "[MusicPlayer] 配置文件加载完成";
}



void MusicPlayer::play() {
    if(playlist->isEmpty()) {
        log("播放失败：播放列表为空");
        return;
    }
    player->play();
}

/**
 * @brief 暂停当前播放的音频
 */
void MusicPlayer::pause()
{
    // 调试输出：播放暂停
    qDebug() << "[MusicPlayer] 播放暂停";

    // 写入日志系统，便于后续查看操作记录
    log("播放暂停");

    // 调用 QMediaPlayer 的 pause() 暂停播放
    player->pause();
}

/**
 * @brief 停止播放
 */
void MusicPlayer::stop()
{
    // 调试输出：播放停止
    qDebug() << "[MusicPlayer] 播放停止";

    // 记录到日志
    log("播放停止");

    // 停止播放并将播放进度归零
    player->stop();
}

/**
 * @brief 切换到播放列表中的下一首歌曲
 */
void MusicPlayer::next()
{
    // 记录切换前的歌曲索引
    int currentIndex = playlist->currentIndex();
    // 播放列表切换到下一首
    playlist->next();
    // 获取切换后的新索引
    int newIndex = playlist->currentIndex();

    // 写入日志，记录操作历史
    log(QString("切换到下一首: %1 -> %2").arg(currentIndex).arg(newIndex));
}

/**
 * @brief 切换到播放列表中的上一首歌曲
 */
void MusicPlayer::previous()
{
    // 记录切换前的歌曲索引
    int currentIndex = playlist->currentIndex();
    // 播放列表切换到上一首
    playlist->previous();
    // 获取切换后的新索引
    int newIndex = playlist->currentIndex();

    // 写入日志，记录操作历史
    log(QString("切换到上一首: %1 -> %2").arg(currentIndex).arg(newIndex));
}

int MusicPlayer::getVolume()
{
    if (!player) return 0;
    return player->volume();
}

/**
 * @brief 设置播放器音量
 * @param vol 新的音量值 (0 - 100)
 */
void MusicPlayer::setVolume(int vol)
{
    // 设置新的音量
    player->setVolume(vol);
    // 写入日志
    log(QString("设置音量为 %1").arg(vol));
}

// ------------------- 播放/暂停切换 -------------------
void MusicPlayer::togglePlay()
{
    // 判断当前播放器状态
    if (player->state() == QMediaPlayer::PlayingState) {
        // 当前正在播放，执行暂停
        log("播放暂停");
        player->pause();
    } else {
        // 当前暂停或停止，执行播放
        log("开始播放");
        player->play();
    }
}

// ------------------- 播放模式切换 -------------------
void MusicPlayer::togglePlaybackMode()
{
    if (!playlist) return;

    // 使用静态变量记录当前模式，三种循环模式：
    // 0 = 顺序播放
    // 1 = 单曲循环
    // 2 = 随机播放
    static int mode = 0;
    mode = (mode + 1) % 3;
    switch(mode){
        case 0:
            playlist->setPlaybackMode(QMediaPlaylist::Sequential);
            log("播放模式: 顺序播放");
            break;
        case 1:
            playlist->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
            log("播放模式: 单曲循环");
            break;
        case 2:
            playlist->setPlaybackMode(QMediaPlaylist::Random);
            log("播放模式: 随机播放");
            break;
    }
}

// ------------------- 获取当前媒体信息 -------------------
void MusicPlayer::updateSongInfo()
{
    if (!player) return;

    QString title = player->metaData(QMediaMetaData::Title).toString();
    QString artist = player->metaData(QMediaMetaData::Author).toString();
    QString composer = player->metaData(QMediaMetaData::Composer).toString();

    QPixmap cover;
    QVariant coverVar = player->metaData(QMediaMetaData::CoverArtImage);
    if (coverVar.isValid()) cover = coverVar.value<QPixmap>();

    // 歌名为空时用文件名
    if (title.isEmpty()) {
        QString currentFile = m_currentFile; // 你播放的文件路径
        QFileInfo fi(currentFile);
        title = fi.baseName();
    }

    qDebug() << "[MusicPlayer] 歌名:" << title;
    qDebug() << "[MusicPlayer] 歌手:" << artist;
    qDebug() << "[MusicPlayer] 作曲:" << composer;
    if (!cover.isNull())
        qDebug() << "[MusicPlayer] 封面已加载";
    else
        qDebug() << "[MusicPlayer] 没有封面";

    // 发射信号给 UI 或其他模块
    emit songInfoUpdated(title, artist, composer, cover);
}

void MusicPlayer::seek(int position)
{
    player->setPosition(position);
}

void MusicPlayer::log(const QString &msg)
{
    qDebug() << "[MusicPlayer]" << msg;
}
