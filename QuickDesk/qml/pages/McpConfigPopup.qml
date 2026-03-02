import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../component"

Popup {
    id: popup
    
    property var mainController
    signal showToast(string message, int toastType)
    
    width: 380
    height: contentColumn.implicitHeight + padding * 2
    padding: Theme.spacingLarge
    
    x: (parent.width - width) / 2
    y: parent.height - height - 48
    
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    background: Rectangle {
        color: Theme.surface
        radius: Theme.radiusMedium
        border.width: Theme.borderWidthThin
        border.color: Theme.border
        
        QDShadow { anchors.fill: parent; radius: parent.radius }
    }
    
    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 150 }
        NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 150; easing.type: Easing.OutCubic }
    }
    
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
    }
    
    ColumnLayout {
        id: contentColumn
        width: parent.width
        spacing: Theme.spacingMedium
        
        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            
            Text {
                text: FluentIconGlyph.robotGlyph
                font.family: "Segoe Fluent Icons"
                font.pixelSize: 20
                color: Theme.primary
            }
            
            Text {
                text: qsTr("AI Integration (MCP)")
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.DemiBold
                color: Theme.text
                Layout.fillWidth: true
            }
        }
        
        // Service toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMedium
            
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                
                Text {
                    text: qsTr("MCP Service")
                    font.pixelSize: Theme.fontSizeMedium
                    color: Theme.text
                }
                
                Text {
                    text: {
                        if (!mainController) return ""
                        if (!mainController.mcpServiceRunning) return qsTr("Service is off. AI agents cannot connect.")
                        var port = mainController.mcpPort
                        var clients = mainController.mcpConnectedClients
                        if (clients > 0)
                            return qsTr("Port %1 · %2 agent(s) connected").arg(port).arg(clients)
                        return qsTr("Port %1 · Waiting for AI agent").arg(port)
                    }
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.textSecondary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
            
            QDSwitch {
                checked: mainController ? mainController.mcpServiceRunning : false
                onToggled: {
                    if (checked) {
                        mainController.startMcpService()
                    } else {
                        mainController.stopMcpService()
                    }
                }
            }
        }
        
        QDDivider { Layout.fillWidth: true }
        
        // Configure AI Client section
        Text {
            text: qsTr("Configure AI Client")
            font.pixelSize: Theme.fontSizeMedium
            font.weight: Font.DemiBold
            color: Theme.text
        }
        
        Text {
            text: qsTr("Generate MCP config for your AI client and paste it into the config file.")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        
        // Client config buttons - 2 column grid
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.spacingSmall
            rowSpacing: Theme.spacingSmall
            
            Repeater {
                model: [
                    { name: "Cursor", type: "cursor", icon: FluentIconGlyph.codeGlyph },
                    { name: "Claude Desktop", type: "claude", icon: FluentIconGlyph.chatBubblesGlyph },
                    { name: "Windsurf", type: "windsurf", icon: FluentIconGlyph.codeGlyph },
                    { name: "VS Code", type: "vscode", icon: FluentIconGlyph.codeGlyph }
                ]
                
                delegate: Rectangle {
                    id: configBtn
                    required property var modelData
                    required property int index
                    
                    Layout.fillWidth: true
                    height: 44
                    radius: Theme.radiusSmall
                    color: configBtnMouse.containsMouse
                           ? Theme.surfaceHover : Theme.surfaceVariant
                    border.width: Theme.borderWidthThin
                    border.color: Theme.border
                    
                    Behavior on color {
                        ColorAnimation { duration: Theme.animationDurationFast }
                    }
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        spacing: Theme.spacingSmall
                        
                        Text {
                            text: configBtn.modelData.icon
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 14
                            color: Theme.primary
                        }
                        
                        Text {
                            text: configBtn.modelData.name
                            font.pixelSize: Theme.fontSizeSmall
                            font.weight: Font.DemiBold
                            color: Theme.text
                            Layout.fillWidth: true
                        }
                        
                        Text {
                            text: FluentIconGlyph.copyGlyph
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 12
                            color: Theme.textSecondary
                        }
                    }
                    
                    MouseArea {
                        id: configBtnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            mainController.copyMcpConfig(configBtn.modelData.type)
                            popup.close()
                            popup.showToast(
                                qsTr("%1 config copied to clipboard").arg(configBtn.modelData.name),
                                0) // Success
                        }
                    }
                    
                    QDToolTip {
                        visible: configBtnMouse.containsMouse
                        text: qsTr("Copy %1 MCP config to clipboard").arg(configBtn.modelData.name)
                    }
                }
            }
        }
        
        // Claude Desktop: write directly
        QDButton {
            Layout.fillWidth: true
            text: qsTr("Auto-configure Claude Desktop")
            buttonType: QDButton.Type.Secondary
            iconText: FluentIconGlyph.settingsGlyph
            visible: Qt.platform.os === "windows" || Qt.platform.os === "osx"
            onClicked: {
                var result = mainController.writeMcpConfig("claude")
                popup.close()
                if (result === 0) {
                    popup.showToast(
                        qsTr("Claude Desktop configured! Restart Claude to apply."),
                        0) // Success
                } else if (result === 1) {
                    popup.showToast(
                        qsTr("Claude Desktop not found. Please install it first."),
                        2) // Error
                } else {
                    popup.showToast(
                        qsTr("Failed to write config. Copy and paste manually."),
                        2) // Error
                }
            }
        }
        
        QDDivider { Layout.fillWidth: true }
        
        // MCP binary path
        Text {
            text: qsTr("MCP Binary Path")
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.textSecondary
        }
        
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            
            Rectangle {
                Layout.fillWidth: true
                height: 32
                radius: Theme.radiusSmall
                color: Theme.surfaceVariant
                border.width: Theme.borderWidthThin
                border.color: Theme.border
                clip: true
                
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingSmall
                    anchors.rightMargin: Theme.spacingSmall
                    text: mainController ? mainController.getMcpBinaryPath() : ""
                    font.pixelSize: Theme.fontSizeSmall - 1
                    font.family: "Consolas"
                    color: Theme.text
                    elide: Text.ElideMiddle
                    verticalAlignment: Text.AlignVCenter
                }
            }
            
            QDIconButton {
                iconSource: FluentIconGlyph.copyGlyph
                buttonSize: QDIconButton.Size.Small
                onClicked: {
                    if (mainController) {
                        mainController.copyToClipboard(mainController.getMcpBinaryPath())
                        popup.close()
                        popup.showToast(qsTr("Path copied"), 0)
                    }
                }
                
                QDToolTip {
                    visible: parent.hovered
                    text: qsTr("Copy path")
                }
            }
        }
    }
}
