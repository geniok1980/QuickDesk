// Copyright 2026 QuickDesk Authors
// Custom video buffer for YUV planar data with stride support (Qt 6)

#pragma once

#include <QAbstractVideoBuffer>
#include <QVideoFrameFormat>
#include <QByteArray>

namespace quickdesk {

/**
 * @brief Custom video buffer for YUV420P data with stride support (Qt 6 API)
 * 
 * This allows us to control the stride (bytes per line) for each plane,
 * enabling single-copy optimization when receiving video frames.
 */
class YUVPlanarVideoBuffer : public QAbstractVideoBuffer {
public:
    YUVPlanarVideoBuffer(const QVideoFrameFormat& fmt)
        : m_format(fmt)
    {
    }

    // Qt 6.8 requires format() implementation
    QVideoFrameFormat format() const override { 
        return m_format; 
    }

    MapData map(QVideoFrame::MapMode mode) override
    {
        Q_UNUSED(mode);
        
        MapData mapData;
        mapData.planeCount = m_planeCount;
        for (int i = 0; i < m_planeCount; ++i) {
            mapData.data[i] = reinterpret_cast<uchar*>(m_data[i].data());
            mapData.bytesPerLine[i] = m_bytesPerLine[i];
            mapData.dataSize[i] = m_data[i].size();
        }
        
        return mapData;
    }
    
    void unmap() override { 
        // No-op for CPU memory buffer
    }

    // Plane data
    QByteArray m_data[4];          // Plane data buffers (using QByteArray for ownership)
    int m_bytesPerLine[4] = {0};   // Stride for each plane
    int m_planeCount = 0;          // Number of planes
    
private:
    QVideoFrameFormat m_format;    // Video frame format
};

} // namespace quickdesk
