// Copyright 2026 QuickDesk Authors
// Video frame provider for GPU rendering in Qt6

#ifndef QUICKDESK_COMPONENT_VIDEOFRAMEPROVIDER_H
#define QUICKDESK_COMPONENT_VIDEOFRAMEPROVIDER_H

#include <QObject>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <QSize>

#include "../manager/SharedMemoryManager.h"

namespace quickdesk {

/**
 * @brief Video frame provider for efficient GPU rendering
 * 
 * Qt6 version using QVideoSink (replaces Qt5's QAbstractVideoSurface).
 * Receives video frames from SharedMemoryManager and pushes them to
 * a QVideoSink obtained from QML's VideoOutput component.
 * 
 * Usage in QML:
 *   VideoOutput {
 *       id: videoOutput
 *   }
 *   VideoFrameProvider {
 *       id: frameProvider
 *       videoSink: videoOutput.videoSink
 *       connectionId: "conn_1"
 *       sharedMemoryManager: clientManager.sharedMemoryManager
 *   }
 */
class VideoFrameProvider : public QObject {
    Q_OBJECT
    
    Q_PROPERTY(QVideoSink* videoSink READ videoSink WRITE setVideoSink NOTIFY videoSinkChanged)
    Q_PROPERTY(QString connectionId READ connectionId WRITE setConnectionId NOTIFY connectionIdChanged)
    Q_PROPERTY(SharedMemoryManager* sharedMemoryManager READ sharedMemoryManager 
               WRITE setSharedMemoryManager NOTIFY sharedMemoryManagerChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QSize frameSize READ frameSize NOTIFY frameSizeChanged)
    Q_PROPERTY(int frameRate READ frameRate NOTIFY frameRateChanged)

public:
    explicit VideoFrameProvider(QObject* parent = nullptr);
    ~VideoFrameProvider() override;

    QVideoSink* videoSink() const { return m_videoSink; }
    void setVideoSink(QVideoSink* sink);

    QString connectionId() const { return m_connectionId; }
    void setConnectionId(const QString& connectionId);

    SharedMemoryManager* sharedMemoryManager() const { return m_sharedMemoryManager; }
    void setSharedMemoryManager(SharedMemoryManager* manager);

    bool isActive() const { return m_active; }
    void setActive(bool active);

    QSize frameSize() const { return m_frameSize; }
    int frameRate() const { return m_frameRate; }

public slots:
    /**
     * @brief Push a new frame to the video sink
     * Called when a new frame is available in shared memory
     */
    void pushFrame();

    /**
     * @brief Handle videoFrameReady notification
     * @param frameIndex The new frame index
     */
    void onVideoFrameReady(quint32 frameIndex);

signals:
    void videoSinkChanged();
    void connectionIdChanged();
    void sharedMemoryManagerChanged();
    void activeChanged();
    void frameSizeChanged();
    void frameRateChanged();
    void frameReceived();

private:
    void updateFrameRate();

    QVideoSink* m_videoSink = nullptr;
    QString m_connectionId;
    SharedMemoryManager* m_sharedMemoryManager = nullptr;
    bool m_active = true;
    QSize m_frameSize;
    int m_frameRate = 0;
    
    // Frame rate calculation
    qint64 m_lastFrameTime = 0;
    int m_frameCount = 0;
    qint64 m_frameRateStartTime = 0;
};

} // namespace quickdesk

#endif // QUICKDESK_COMPONENT_VIDEOFRAMEPROVIDER_H
