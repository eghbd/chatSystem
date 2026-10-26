// 本文件：录音与播放器。以单例形式提供语音消息的录制和播放能力。

#ifndef SOUNDRECORDER_H
#define SOUNDRECORDER_H

#include <QObject>
#include <QStandardPaths>
#include <QFile>
#include <QAudioSource>
#include <QAudioSink>
#include <QMediaDevices>

// 录音/播放器：负责把麦克风声音录成 pcm 文件，并把 pcm 数据播放出来
class SoundRecorder : public QObject
{
    Q_OBJECT
public:
    // 录音文件的保存路径（pcm 格式）
    const QString RECORD_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sound/tmpRecord.pcm";
    // 播放时使用的临时文件路径（pcm 格式）
    const QString PLAY_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sound/tmpPlay.pcm";

public:
    // 获取录音器单例
    static SoundRecorder* getInstance();

    /////////////////////////////////////////////////
    /// 录制语音语音
    /////////////////////////////////////////////////
    // 开始录制
    bool startRecord();
    // 停止录制
    void stopRecord();

private:
    static SoundRecorder* instance;
    // 单例对象，构造函数私有化
    explicit SoundRecorder(QObject *parent = nullptr);

    // 录音时写入的 pcm 文件
    QFile soundFile;
    // 录音输入设备
    QAudioSource* audioSource = nullptr;

    /////////////////////////////////////////////////
    /// 播放语音
    /////////////////////////////////////////////////
public:
    // 开始播放
    void startPlay(const QByteArray& content);
    // 停止播放
    void stopPlay();

private:
    // 播放语音的输出设备
    QAudioSink *audioSink = nullptr;
    // 系统音频设备管理器
    QMediaDevices *outputDevices = nullptr;
    // 当前默认的音频输出设备
    QAudioDevice outputDevice;
    // 播放时读取的 pcm 文件
    QFile inputFile;

signals:
    // 录制完毕后发送这个信号
    void soundRecordDone(const QString& path);
    // 播放完毕发送这个信号
    void soundPlayDone();

};

#endif // SOUNDRECORDER_H
