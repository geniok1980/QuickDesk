import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../component"
import "../quickdeskcomponent"

Item {
    id: root

    required property var mainController

    signal connectToDevice(string deviceId, string accessCode)
    signal showToast(string message, int toastType)

    property bool isLoggedIn: mainController && mainController.authManager ? mainController.authManager.isLoggedIn : false

    // Not logged in prompt
    Item {
        anchors.fill: parent
        visible: !root.isLoggedIn

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Theme.spacingLarge

            Text {
                text: FluentIconGlyph.contactInfoGlyph
                font.family: "Segoe Fluent Icons"
                font.pixelSize: 48
                color: Theme.textDisabled
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("Please login to view device list")
                font.pixelSize: Theme.fontSizeMedium
                color: Theme.textSecondary
                Layout.alignment: Qt.AlignHCenter
            }

            QDButton {
                text: qsTr("Login")
                highlighted: true
                Layout.alignment: Qt.AlignHCenter
                onClicked: loginDialog.open()
            }
        }
    }

    // Logged in content
    ScrollView {
        anchors.fill: parent
        anchors.margins: Theme.spacingLarge
        visible: root.isLoggedIn
        clip: true

        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: QDScrollBar {}

        ColumnLayout {
            width: root.width - Theme.spacingLarge * 2
            spacing: Theme.spacingLarge

            // ---- My Devices Section ----
            Text {
                text: qsTr("My Devices")
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.Bold
                color: Theme.text
            }

            Text {
                visible: deviceRepeater.count === 0
                text: qsTr("No devices bound yet")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
            }

            Repeater {
                id: deviceRepeater
                model: root.mainController && root.mainController.cloudDeviceManager
                       ? root.mainController.cloudDeviceManager.myDevices : []

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    height: 56
                    radius: Theme.radiusSmall
                    color: deviceMouseArea.containsMouse ? Theme.surfaceHover : Theme.surfaceVariant

                    Behavior on color {
                        ColorAnimation { duration: Theme.animationDurationFast }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        spacing: Theme.spacingSmall

                        // Online indicator
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: modelData.online ? Theme.success : Theme.textDisabled
                        }

                        // Device name + ID
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: modelData.remark || modelData.device_name || qsTr("Device")
                                font.pixelSize: Theme.fontSizeMedium
                                color: Theme.text
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.device_id || ""
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                            }
                        }

                        // Connect button (only for online devices)
                        QDIconButton {
                            visible: modelData.online === true
                            icon: FluentIconGlyph.remoteGlyph
                            toolTipText: qsTr("Connect")

                            onClicked: {
                                var accessCode = root.mainController.cloudDeviceManager.getDeviceAccessCode(modelData.device_id)
                                if (accessCode) {
                                    root.connectToDevice(modelData.device_id, accessCode)
                                } else {
                                    root.showToast(qsTr("Access code not available"), 2)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: deviceMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.RightButton

                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                deviceContextMenu.deviceId = modelData.device_id
                                deviceContextMenu.popup()
                            }
                        }
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // ---- My Favorites Section ----
            Text {
                text: qsTr("My Favorites")
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.Bold
                color: Theme.text
            }

            Text {
                visible: favoriteRepeater.count === 0
                text: qsTr("No favorites yet. Star a device from Remote Control page.")
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.textSecondary
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Repeater {
                id: favoriteRepeater
                model: root.mainController && root.mainController.cloudDeviceManager
                       ? root.mainController.cloudDeviceManager.myFavorites : []

                delegate: Rectangle {
                    required property var modelData
                    required property int index

                    Layout.fillWidth: true
                    height: 56
                    radius: Theme.radiusSmall
                    color: favMouseArea.containsMouse ? Theme.surfaceHover : Theme.surfaceVariant

                    Behavior on color {
                        ColorAnimation { duration: Theme.animationDurationFast }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingMedium
                        anchors.rightMargin: Theme.spacingMedium
                        spacing: Theme.spacingSmall

                        // Star icon
                        Text {
                            text: FluentIconGlyph.favoriteStarFillGlyph
                            font.family: "Segoe Fluent Icons"
                            font.pixelSize: 16
                            color: Theme.warning
                        }

                        // Name + ID
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: modelData.device_name || modelData.device_id || qsTr("Device")
                                font.pixelSize: Theme.fontSizeMedium
                                color: Theme.text
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.device_id || ""
                                font.pixelSize: Theme.fontSizeSmall
                                color: Theme.textSecondary
                            }
                        }

                        // Connect button
                        QDIconButton {
                            icon: FluentIconGlyph.remoteGlyph
                            toolTipText: qsTr("Connect")

                            onClicked: {
                                var password = modelData.access_password || ""
                                if (password) {
                                    root.connectToDevice(modelData.device_id, password)
                                } else {
                                    root.showToast(qsTr("No password saved for this device"), 2)
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: favMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.RightButton

                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                favContextMenu.deviceId = modelData.device_id
                                favContextMenu.deviceName = modelData.device_name || ""
                                favContextMenu.popup()
                            }
                        }
                    }
                }
            }

            // Bottom spacer
            Item { Layout.preferredHeight: Theme.spacingLarge }
        }
    }

    // Context menu for My Devices
    Menu {
        id: deviceContextMenu
        property string deviceId: ""

        MenuItem {
            text: qsTr("Set Remark")
            onTriggered: {
                remarkDialog.deviceId = deviceContextMenu.deviceId
                remarkDialog.isFavorite = false
                remarkDialog.open()
            }
        }
        MenuItem {
            text: qsTr("Remove")
            onTriggered: {
                root.mainController.cloudDeviceManager.unbindDevice(deviceContextMenu.deviceId)
            }
        }
    }

    // Context menu for Favorites
    Menu {
        id: favContextMenu
        property string deviceId: ""
        property string deviceName: ""

        MenuItem {
            text: qsTr("Edit Remark")
            onTriggered: {
                remarkDialog.deviceId = favContextMenu.deviceId
                remarkDialog.isFavorite = true
                remarkDialog.open()
            }
        }
        MenuItem {
            text: qsTr("Remove Favorite")
            onTriggered: {
                root.mainController.cloudDeviceManager.removeFavorite(favContextMenu.deviceId)
            }
        }
    }

    // Remark edit dialog
    Popup {
        id: remarkDialog
        modal: true
        anchors.centerIn: parent
        width: 300
        padding: Theme.spacingLarge

        property string deviceId: ""
        property bool isFavorite: false

        background: Rectangle {
            color: Theme.surface
            radius: Theme.radiusLarge
            border.width: Theme.borderWidthThin
            border.color: Theme.border
        }

        onOpened: remarkField.text = ""

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacingMedium

            Text {
                text: qsTr("Set Remark")
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.Bold
                color: Theme.text
            }

            QDTextField {
                id: remarkField
                Layout.fillWidth: true
                placeholderText: qsTr("Enter remark name")
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingSmall

                QDButton {
                    text: qsTr("Cancel")
                    onClicked: remarkDialog.close()
                }

                QDButton {
                    text: qsTr("Save")
                    highlighted: true
                    enabled: remarkField.text.length > 0
                    onClicked: {
                        if (remarkDialog.isFavorite) {
                            root.mainController.cloudDeviceManager.updateFavorite(
                                remarkDialog.deviceId, remarkField.text, "")
                        } else {
                            root.mainController.cloudDeviceManager.setDeviceRemark(
                                remarkDialog.deviceId, remarkField.text)
                        }
                        remarkDialog.close()
                    }
                }
            }
        }
    }

    // Login dialog
    LoginDialog {
        id: loginDialog
        mainController: root.mainController
    }
}
