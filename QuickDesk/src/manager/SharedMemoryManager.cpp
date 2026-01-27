// Copyright 2026 QuickDesk Authors

#include "SharedMemoryManager.h"
#include "infra/log/log.h"

namespace quickdesk {

SharedMemoryManager::SharedMemoryManager(QObject* parent)
    : QObject(parent)
{
}

SharedMemoryManager::~SharedMemoryManager()
{
    detachAll();
}

bool SharedMemoryManager::attach(const QString& connectionId, 
                                  const QString& sharedMemoryName)
{
    std::string key = connectionId.toStdString();
    
    // Already attached?
    auto it = m_handles.find(key);
    if (it != m_handles.end()) {
        auto& existing = it->second;
        if (existing->sharedMemoryName == sharedMemoryName && 
            existing->sharedMemory && existing->sharedMemory->isAttached()) {
            return true;  // Already attached to same memory
        }
        // Different memory name - detach first
        detach(connectionId);
    }

    // Create new QSharedMemory instance
    auto shm = std::make_unique<QSharedMemory>();
    
    // Use setNativeKey() to directly use the platform-specific name
    // This allows interoperability with C++ client using native APIs
    shm->setNativeKey(sharedMemoryName);

    // Attach to existing shared memory (created by C++ client)
    if (!shm->attach(QSharedMemory::ReadOnly)) {
        LOG_WARN("Failed to attach to shared memory '{}' for connection {}: {}",
                 sharedMemoryName.toStdString(), connectionId.toStdString(),
                 shm->errorString().toStdString());
        emit frameReadError(connectionId, 
                            QString("Failed to attach: %1").arg(shm->errorString()));
        return false;
    }

    // Create handle entry
    auto handle = std::make_unique<SharedMemoryHandle>();
    handle->connectionId = connectionId;
    handle->sharedMemoryName = sharedMemoryName;
    handle->lastFrameIndex = 0;
    handle->sharedMemory = std::move(shm);

    m_handles[key] = std::move(handle);

    LOG_INFO("Attached to shared memory '{}' for connection {}",
             sharedMemoryName.toStdString(), connectionId.toStdString());
    emit attachmentChanged(connectionId, true);
    return true;
}

void SharedMemoryManager::detach(const QString& connectionId)
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return;
    }

    closeHandle(*it->second);
    m_handles.erase(it);

    LOG_INFO("Detached from shared memory for connection {}", 
             connectionId.toStdString());
    emit attachmentChanged(connectionId, false);
}

void SharedMemoryManager::detachAll()
{
    for (auto& pair : m_handles) {
        closeHandle(*pair.second);
        emit attachmentChanged(pair.second->connectionId, false);
    }
    m_handles.clear();
    LOG_INFO("Detached from all shared memory regions");
}

bool SharedMemoryManager::isAttached(const QString& connectionId) const
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return false;
    }
    return it->second->sharedMemory && it->second->sharedMemory->isAttached();
}

QImage SharedMemoryManager::readFrame(const QString& connectionId)
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        LOG_WARN("No shared memory handle for connection {}", 
                 connectionId.toStdString());
        return QImage();
    }

    auto& handle = it->second;
    if (!handle->sharedMemory || !handle->sharedMemory->isAttached()) {
        return QImage();
    }

    // Lock shared memory for reading
    if (!handle->sharedMemory->lock()) {
        LOG_WARN("Failed to lock shared memory for connection {}: {}",
                 connectionId.toStdString(), 
                 handle->sharedMemory->errorString().toStdString());
        return QImage();
    }

    QImage result;
    const void* data = handle->sharedMemory->constData();
    
    if (data) {
        // Read header
        const SharedFrameHeader* header = 
            static_cast<const SharedFrameHeader*>(data);

        // Validate magic number
        if (header->magic != kSharedFrameMagic) {
            LOG_WARN("Invalid magic number in shared memory for connection {}: 0x{:08X}",
                     connectionId.toStdString(), header->magic);
            emit frameReadError(connectionId, "Invalid magic number in shared memory");
        } else if (header->version != kSharedFrameVersion) {
            // Validate version
            LOG_WARN("Unsupported version in shared memory for connection {}: {}",
                     connectionId.toStdString(), header->version);
            emit frameReadError(connectionId, 
                                QString("Unsupported version: %1").arg(header->version));
        } else {
            // Update last frame index
            handle->lastFrameIndex = header->frame_index;

            quint32 width = header->width;
            quint32 height = header->height;
            quint32 dataSize = header->data_size;

            // Validate dimensions
            if (width == 0 || height == 0 || width > 8192 || height > 8192) {
                LOG_WARN("Invalid frame dimensions for connection {}: {}x{}",
                         connectionId.toStdString(), width, height);
            } else {
                // Validate data size
                quint32 expectedSize = width * height * 4;
                if (dataSize != expectedSize) {
                    LOG_WARN("Data size mismatch for connection {}: {} vs expected {}",
                             connectionId.toStdString(), dataSize, expectedSize);
                } else {
                    // Get pointer to frame data
                    const uchar* frameData = static_cast<const uchar*>(data) + 
                                             sizeof(SharedFrameHeader);

                    // Create QImage from data
                    // Format is BGRA, which maps to QImage::Format_ARGB32 on little-endian
                    QImage image(frameData, static_cast<int>(width), static_cast<int>(height),
                                 static_cast<int>(width * 4), QImage::Format_ARGB32);
                    
                    // Deep copy the image so it doesn't depend on shared memory
                    result = image.copy();
                }
            }
        }
    }

    // Unlock shared memory
    handle->sharedMemory->unlock();

    return result;
}

QVideoFrame SharedMemoryManager::readVideoFrame(const QString& connectionId)
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return QVideoFrame();
    }

    auto& handle = it->second;
    if (!handle->sharedMemory || !handle->sharedMemory->isAttached()) {
        return QVideoFrame();
    }

    // Lock shared memory for reading
    if (!handle->sharedMemory->lock()) {
        LOG_WARN("Failed to lock shared memory for connection {}: {}",
                 connectionId.toStdString(), 
                 handle->sharedMemory->errorString().toStdString());
        return QVideoFrame();
    }

    QVideoFrame result;
    const void* data = handle->sharedMemory->constData();
    
    if (data) {
        const SharedFrameHeader* header = 
            static_cast<const SharedFrameHeader*>(data);

        // Validate header
        if (header->magic == kSharedFrameMagic && 
            header->version == kSharedFrameVersion) {
            
            handle->lastFrameIndex = header->frame_index;

            quint32 width = header->width;
            quint32 height = header->height;
            quint32 dataSize = header->data_size;
            quint32 expectedSize = width * height * 4;

            if (width > 0 && height > 0 && width <= 8192 && height <= 8192 &&
                dataSize == expectedSize) {
                
                const uchar* frameData = static_cast<const uchar*>(data) + 
                                         sizeof(SharedFrameHeader);

                // Create QVideoFrame with BGRA format for GPU rendering
                QVideoFrameFormat format(QSize(static_cast<int>(width), 
                                               static_cast<int>(height)),
                                        QVideoFrameFormat::Format_BGRA8888);
                result = QVideoFrame(format);
                
                if (result.map(QVideoFrame::WriteOnly)) {
                    uchar* destData = result.bits(0);
                    int destStride = result.bytesPerLine(0);
                    int srcStride = static_cast<int>(width * 4);
                    
                    // Copy line by line (handles stride differences)
                    if (destStride == srcStride) {
                        memcpy(destData, frameData, dataSize);
                    } else {
                        for (quint32 y = 0; y < height; ++y) {
                            memcpy(destData + y * destStride,
                                   frameData + y * srcStride,
                                   srcStride);
                        }
                    }
                    result.unmap();
                } else {
                    result = QVideoFrame();  // Failed to map
                }
            }
        }
    }

    handle->sharedMemory->unlock();
    return result;
}

FrameData SharedMemoryManager::lockFrame(const QString& connectionId)
{
    FrameData frameData;
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return frameData;
    }

    auto& handle = it->second;
    if (!handle->sharedMemory || !handle->sharedMemory->isAttached()) {
        return frameData;
    }

    // Already locked?
    if (handle->isLocked) {
        LOG_WARN("Shared memory already locked for connection {}", 
                 connectionId.toStdString());
        return frameData;
    }

    if (!handle->sharedMemory->lock()) {
        LOG_WARN("Failed to lock shared memory for connection {}: {}",
                 connectionId.toStdString(), 
                 handle->sharedMemory->errorString().toStdString());
        return frameData;
    }

    handle->isLocked = true;

    const void* data = handle->sharedMemory->constData();
    if (!data) {
        handle->sharedMemory->unlock();
        handle->isLocked = false;
        return frameData;
    }

    const SharedFrameHeader* header = 
        static_cast<const SharedFrameHeader*>(data);

    // Validate header
    if (header->magic != kSharedFrameMagic || 
        header->version != kSharedFrameVersion) {
        handle->sharedMemory->unlock();
        handle->isLocked = false;
        return frameData;
    }

    quint32 width = header->width;
    quint32 height = header->height;
    quint32 dataSize = header->data_size;
    quint32 expectedSize = width * height * 4;

    if (width == 0 || height == 0 || width > 8192 || height > 8192 ||
        dataSize != expectedSize) {
        handle->sharedMemory->unlock();
        handle->isLocked = false;
        return frameData;
    }

    // Update last frame index
    handle->lastFrameIndex = header->frame_index;

    // Fill frame data
    frameData.valid = true;
    frameData.width = width;
    frameData.height = height;
    frameData.frameIndex = header->frame_index;
    frameData.timestampUs = header->timestamp_us;
    frameData.format = static_cast<SharedFrameFormat>(header->format);
    frameData.data = static_cast<const uchar*>(data) + sizeof(SharedFrameHeader);
    frameData.dataSize = dataSize;
    frameData.stride = width * 4;

    return frameData;
}

void SharedMemoryManager::unlockFrame(const QString& connectionId)
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return;
    }

    auto& handle = it->second;
    if (handle->isLocked && handle->sharedMemory) {
        handle->sharedMemory->unlock();
        handle->isLocked = false;
    }
}

QSize SharedMemoryManager::frameSize(const QString& connectionId) const
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end() || !it->second->sharedMemory || 
        !it->second->sharedMemory->isAttached()) {
        return QSize();
    }

    // We need to read the header to get dimensions
    // This requires a const_cast since we need to lock
    auto& handle = *it->second;
    if (!const_cast<QSharedMemory*>(handle.sharedMemory.get())->lock()) {
        return QSize();
    }

    QSize result;
    const void* data = handle.sharedMemory->constData();
    if (data) {
        const SharedFrameHeader* header = 
            static_cast<const SharedFrameHeader*>(data);
        if (header->magic == kSharedFrameMagic) {
            result = QSize(static_cast<int>(header->width), 
                          static_cast<int>(header->height));
        }
    }

    const_cast<QSharedMemory*>(handle.sharedMemory.get())->unlock();
    return result;
}

quint32 SharedMemoryManager::lastFrameIndex(const QString& connectionId) const
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return 0;
    }
    return it->second->lastFrameIndex;
}

bool SharedMemoryManager::isNewFrame(const QString& connectionId, 
                                      quint32 currentFrameIndex) const
{
    std::string key = connectionId.toStdString();
    auto it = m_handles.find(key);
    if (it == m_handles.end()) {
        return true;  // Not tracking, assume new
    }
    return currentFrameIndex > it->second->lastFrameIndex;
}

void SharedMemoryManager::closeHandle(SharedMemoryHandle& handle)
{
    if (handle.sharedMemory) {
        if (handle.sharedMemory->isAttached()) {
            handle.sharedMemory->detach();
        }
        handle.sharedMemory.reset();
    }
    handle.lastFrameIndex = 0;
}

} // namespace quickdesk
