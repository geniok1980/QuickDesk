import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QuickDesk 1.0

ApplicationWindow {
    id: root
    width: 900
    height: 600
    visible: true
    title: "QuickDesk - 远程桌面"

    // Main controller
    MainController {
        id: mainController
        
        onInitializationFailed: (error) => {
            console.error("Initialization failed:", error)
            statusText.text = "初始化失败: " + error
        }
        
        onDeviceIdChanged: {
            console.log("Device ID changed:", mainController.deviceId)
        }
        
        onAccessCodeChanged: {
            console.log("Access Code changed:", mainController.accessCode)
        }
    }

    Component.onCompleted: {
        console.log("Main.qml loaded, initializing...")
        mainController.initialize()
    }

    Component.onDestruction: {
        console.log("Main.qml unloading, shutting down...")
        mainController.shutdown()
    }

    // Main layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Header
        Text {
            text: "QuickDesk"
            font.pixelSize: 28
            font.bold: true
            color: "#2196F3"
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: mainController.isInitialized ? "#E8F5E9" : "#FFF3E0"
            radius: 5
            
            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                
                Text {
                    id: statusText
                    text: mainController.initStatus
                    color: mainController.isInitialized ? "#2E7D32" : "#E65100"
                }
                
                Item { Layout.fillWidth: true }
                
                Text {
                    text: mainController.serverManager ? 
                          "服务器: " + mainController.serverManager.serverUrl : ""
                    color: "#666"
                    font.pixelSize: 12
                }
            }
        }

        // Main content
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 20

            // Left panel - Host info
            Rectangle {
                Layout.preferredWidth: 300
                Layout.fillHeight: true
                color: "#F5F5F5"
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Text {
                        text: "我的设备"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#333"
                    }

                    // Device ID card
                    Rectangle {
                        Layout.fillWidth: true
                        height: 100
                        color: "white"
                        radius: 8
                        border.color: "#E0E0E0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "设备 ID"
                                font.pixelSize: 12
                                color: "#666"
                            }

                            Text {
                                text: mainController.deviceId || "获取中..."
                                font.pixelSize: 24
                                font.bold: true
                                font.family: "Consolas"
                                color: mainController.deviceId ? "#1976D2" : "#999"
                            }
                        }
                    }

                    // Access code card
                    Rectangle {
                        Layout.fillWidth: true
                        height: 100
                        color: "white"
                        radius: 8
                        border.color: "#E0E0E0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Text {
                                text: "临时访问码"
                                font.pixelSize: 12
                                color: "#666"
                            }

                            RowLayout {
                                Text {
                                    text: mainController.accessCode || "获取中..."
                                    font.pixelSize: 24
                                    font.bold: true
                                    font.family: "Consolas"
                                    color: mainController.accessCode ? "#4CAF50" : "#999"
                                }
                                
                                Item { Layout.fillWidth: true }
                                
                                // Refresh button (placeholder)
                                Rectangle {
                                    width: 30
                                    height: 30
                                    radius: 15
                                    color: refreshArea.containsMouse ? "#E3F2FD" : "transparent"
                                    
                                    Text {
                                        anchors.centerIn: parent
                                        text: "🔄"
                                        font.pixelSize: 16
                                    }
                                    
                                    MouseArea {
                                        id: refreshArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            console.log("Refresh access code clicked")
                                            // TODO: Implement refresh
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Connection status
                    Rectangle {
                        Layout.fillWidth: true
                        height: 50
                        color: mainController.isHostConnected ? "#E8F5E9" : "#FFEBEE"
                        radius: 8

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: mainController.isHostConnected ? "#4CAF50" : "#F44336"
                            }

                            Text {
                                text: mainController.isHostConnected ? "已就绪，可接受连接" : "未连接"
                                color: mainController.isHostConnected ? "#2E7D32" : "#C62828"
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }

                    // Connected clients section
                    Text {
                        text: "已连接的客户端"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#333"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 100
                        color: "white"
                        radius: 8
                        border.color: "#E0E0E0"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: mainController.hostManager ? 
                                  (mainController.hostManager.clientCount > 0 ? 
                                   mainController.hostManager.clientCount + " 个客户端" : 
                                   "暂无连接") : 
                                  "加载中..."
                            color: "#999"
                        }
                    }
                }
            }

            // Right panel - Client controls
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#FAFAFA"
                radius: 10

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 15
                    spacing: 15

                    Text {
                        text: "连接远程主机"
                        font.pixelSize: 18
                        font.bold: true
                        color: "#333"
                    }

                    // Connection input
                    Rectangle {
                        Layout.fillWidth: true
                        height: 180
                        color: "white"
                        radius: 8
                        border.color: "#E0E0E0"
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 15
                            spacing: 12

                            // Device ID input
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    text: "设备 ID"
                                    font.pixelSize: 12
                                    color: "#666"
                                }

                                TextField {
                                    id: remoteDeviceId
                                    Layout.fillWidth: true
                                    placeholderText: "输入远程设备 ID"
                                    font.pixelSize: 16
                                }
                            }

                            // Access code input
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 4

                                Text {
                                    text: "访问码"
                                    font.pixelSize: 12
                                    color: "#666"
                                }

                                TextField {
                                    id: remoteAccessCode
                                    Layout.fillWidth: true
                                    placeholderText: "输入访问码"
                                    font.pixelSize: 16
                                    echoMode: TextInput.Password
                                }
                            }

                            // Connect button
                            Button {
                                Layout.fillWidth: true
                                text: "连接"
                                enabled: remoteDeviceId.text.length > 0 && 
                                         remoteAccessCode.text.length > 0 &&
                                         mainController.isInitialized
                                
                                onClicked: {
                                    console.log("Connecting to:", remoteDeviceId.text)
                                    var connId = mainController.connectToRemoteHost(
                                        remoteDeviceId.text,
                                        remoteAccessCode.text
                                    )
                                    console.log("Connection ID:", connId)
                                }
                            }
                        }
                    }

                    // Active connections
                    Text {
                        text: "我的远程连接"
                        font.pixelSize: 14
                        font.bold: true
                        color: "#333"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "white"
                        radius: 8
                        border.color: "#E0E0E0"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: mainController.clientManager ?
                                  (mainController.clientManager.connectionCount > 0 ?
                                   mainController.clientManager.connectionCount + " 个连接" :
                                   "暂无远程连接") :
                                  "加载中..."
                            color: "#999"
                        }
                    }
                }
            }
        }
    }
}
