// Copyright 2026 QuickDesk Authors
// Shared memory manager for video frame transfer

#ifndef QUICKDESK_MANAGER_SHAREDMEMORYMANAGER_H
#define QUICKDESK_MANAGER_SHAREDMEMORYMANAGER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QSharedMemory>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <map>
#include <memory>
#include <string>

namespace quickdesk {

// Magic number for shared frame memory: "QDVF" (QuickDesk Video Frame)
constexpr quint32 kSharedFrameMagic = 0x51444656;

// Current version of the shared frame header
constexpr quint32 kSharedFrameVersion = 1;

// Pixel format enumeration (must match C++ client)
enum class SharedFrameFormat : quint32 {
    Unknown = 0,
    BGRA = 1,  // Blue-Green-Red-Alpha (Windows default)
    RGBA = 2,  // Red-Green-Blue-Alpha
};

// Header structure for shared memory video frames (64 bytes)
// Must match the C++ client's SharedFrameHeader exactly
#pragma pack(push, 1)
struct SharedFrameHeader {
    quint32 magic;        // Magic number (kSharedFrameMagic)
    quint32 version;      // Header version (kSharedFrameVersion)
    quint32 width;        // Frame width in pixels
    quint32 height;       // Frame height in pixels
    quint32 format;       // Pixel format (SharedFrameFormat)
    quint32 frame_index;  // Incrementing frame counter
    quint64 timestamp_us; // Frame timestamp in microseconds
    quint32 data_size;    // Size of frame data in bytes
    quint32 reserved[7];  // Reserved for future use
};
#pragma pack(pop)

static_assert(sizeof(SharedFrameHeader) == 64,
              "SharedFrameHeader must be exactly 64 bytes");

/**
 * @brief Frame data structure for efficient GPU rendering
 * Contains raw frame data and metadata without copying
 */
struct FrameData {
    bool valid = false;
    quint32 width = 0;
    quint32 height = 0;
    quint32 frameIndex = 0;
    quint64 timestampUs = 0;
    SharedFrameFormat format = SharedFrameFormat::Unknown;
    const uchar* data = nullptr;  // Pointer to frame data (valid only while locked)
    quint32 dataSize = 0;
    quint32 stride = 0;  // Bytes per line (width * 4 for BGRA)
};

/**
 * @brief Per-connection shared memory handle using QSharedMemory
 */
struct SharedMemoryHandle {
    QString connectionId;
    QString sharedMemoryName;
    quint32 lastFrameIndex = 0;
    std::unique_ptr<QSharedMemory> sharedMemory;
    bool isLocked = false;  // Track lock state for lockFrame/unlockFrame
};

/**
 * @brief Manages shared memory regions for video frame transfer
 * 
 * Each connection to a remote host has its own shared memory region.
 * This class handles attaching to, reading from, and detaching from
 * these shared memory regions.
 * 
 * Uses QSharedMemory with setNativeKey() for cross-platform compatibility
 * with the C++ client which uses platform-specific shared memory APIs.
 */
class SharedMemoryManager : public QObject {
    Q_OBJECT

public:
    explicit SharedMemoryManager(QObject* parent = nullptr);
    ~SharedMemoryManager() override;

    /**
     * @brief Attach to shared memory for a connection
     * @param connectionId The connection identifier
     * @param sharedMemoryName The native name of the shared memory region
     * @return true if successfully attached
     */
    bool attach(const QString& connectionId, const QString& sharedMemoryName);

    /**
     * @brief Detach from shared memory for a connection
     * @param connectionId The connection identifier
     */
    void detach(const QString& connectionId);

    /**
     * @brief Detach from all shared memory regions
     */
    void detachAll();

    /**
     * @brief Check if attached to shared memory for a connection
     * @param connectionId The connection identifier
     * @return true if attached
     */
    bool isAttached(const QString& connectionId) const;

    /**
     * @brief Read the current frame from shared memory (creates QImage copy)
     * @param connectionId The connection identifier
     * @return The frame as QImage, or null image if failed
     * @note This method is less efficient, prefer readVideoFrame() for GPU rendering
     */
    QImage readFrame(const QString& connectionId);

    /**
     * @brief Read the current frame as QVideoFrame for GPU rendering
     * @param connectionId The connection identifier
     * @return QVideoFrame ready for VideoOutput, or invalid frame if failed
     * @note This is the preferred method for efficient GPU rendering
     */
    QVideoFrame readVideoFrame(const QString& connectionId);

    /**
     * @brief Lock shared memory and get frame data without copying
     * @param connectionId The connection identifier
     * @return FrameData with pointer to raw data, call unlockFrame() when done
     * @note The returned data pointer is only valid until unlockFrame() is called
     */
    FrameData lockFrame(const QString& connectionId);

    /**
     * @brief Unlock shared memory after reading frame data
     * @param connectionId The connection identifier
     */
    void unlockFrame(const QString& connectionId);

    /**
     * @brief Get the last frame index for a connection
     * @param connectionId The connection identifier
     * @return The last frame index, or 0 if not attached
     */
    quint32 lastFrameIndex(const QString& connectionId) const;

    /**
     * @brief Check if a new frame is available (frame index changed)
     * @param connectionId The connection identifier
     * @param currentFrameIndex The current frame index from notification
     * @return true if the frame is new
     */
    bool isNewFrame(const QString& connectionId, quint32 currentFrameIndex) const;

    /**
     * @brief Get frame dimensions for a connection
     * @param connectionId The connection identifier
     * @return QSize with width and height, or empty size if not available
     */
    QSize frameSize(const QString& connectionId) const;

signals:
    void attachmentChanged(const QString& connectionId, bool attached);
    void frameReadError(const QString& connectionId, const QString& error);

private:
    // Use std::map with unique_ptr to avoid copy issues
    std::map<std::string, std::unique_ptr<SharedMemoryHandle>> m_handles;

    void closeHandle(SharedMemoryHandle& handle);
};

} // namespace quickdesk

#endif // QUICKDESK_MANAGER_SHAREDMEMORYMANAGER_H
