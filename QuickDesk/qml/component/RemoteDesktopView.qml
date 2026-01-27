// Copyright 2026 QuickDesk Authors
// Remote desktop video display component

import QtQuick
import QtMultimedia
import QuickDesk 1.0

/**
 * RemoteDesktopView - Displays remote desktop video stream
 * 
 * Usage:
 *   RemoteDesktopView {
 *       connectionId: "conn_1"
 *       clientManager: mainController.clientManager
 *       active: visible  // Only render when visible
 *   }
 */
Rectangle {
    id: root
    
    // Required properties
    required property string connectionId
    required property ClientManager clientManager
    
    // Optional properties
    property bool active: true
    property alias fillMode: videoOutput.fillMode
    
    // Read-only properties
    readonly property int frameWidth: frameProvider.frameSize.width
    readonly property int frameHeight: frameProvider.frameSize.height
    readonly property int frameRate: frameProvider.frameRate
    readonly property bool hasVideo: frameWidth > 0 && frameHeight > 0
    
    color: "#1a1a1a"  // Dark background
    
    // Video output for GPU rendering
    VideoOutput {
        id: videoOutput
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }
    
    // Frame provider connects shared memory to video sink
    VideoFrameProvider {
        id: frameProvider
        videoSink: videoOutput.videoSink
        connectionId: root.connectionId
        sharedMemoryManager: root.clientManager ? root.clientManager.sharedMemoryManager : null
        active: root.active && root.visible
    }
    
    // Connect to videoFrameReady signal from ClientManager
    Connections {
        target: root.clientManager
        
        function onVideoFrameReady(connId, frameIndex) {
            if (connId === root.connectionId) {
                frameProvider.onVideoFrameReady(frameIndex)
            }
        }
    }
    
    // Loading indicator when no video
    Column {
        anchors.centerIn: parent
        spacing: 16
        visible: !root.hasVideo && root.active
        
        QDSpinner {
            anchors.horizontalCenter: parent.horizontalCenter
            size: 48
            running: visible
        }
        
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Waiting for video...")
            color: "#888888"
            font.pixelSize: 14
        }
    }
    
    // Frame rate overlay (optional, for debugging)
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 8
        width: fpsText.width + 12
        height: fpsText.height + 8
        radius: 4
        color: "#80000000"
        visible: root.hasVideo && false  // Set to true to show FPS
        
        Text {
            id: fpsText
            anchors.centerIn: parent
            text: root.frameRate + " FPS"
            color: root.frameRate >= 30 ? "#00ff00" : 
                   root.frameRate >= 15 ? "#ffff00" : "#ff0000"
            font.pixelSize: 12
            font.family: "Consolas"
        }
    }
}
