import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../component"

Popup {
    id: loginDialog

    required property var mainController

    modal: true
    anchors.centerIn: parent
    width: 380
    height: contentColumn.implicitHeight + 60
    padding: Theme.spacingXLarge

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radiusLarge
        border.width: Theme.borderWidthThin
        border.color: Theme.border

        // Shadow
        layer.enabled: true
        layer.effect: null
    }

    property string mode: "login"  // "login" or "register"
    property string errorMessage: ""
    property bool isLoading: false

    onOpened: {
        errorMessage = ""
        usernameField.text = ""
        passwordField.text = ""
        phoneField.text = ""
        emailField.text = ""
        usernameField.forceActiveFocus()
    }

    // Connect to AuthManager signals
    Connections {
        target: loginDialog.mainController ? loginDialog.mainController.authManager : null

        function onLoginSuccess() {
            loginDialog.isLoading = false
            loginDialog.close()
        }

        function onLoginFailed(errorMsg) {
            loginDialog.isLoading = false
            loginDialog.errorMessage = errorMsg
        }

        function onRegisterSuccess() {
            loginDialog.isLoading = false
            loginDialog.mode = "login"
            loginDialog.errorMessage = qsTr("Registration successful! Please login.")
        }

        function onRegisterFailed(errorMsg) {
            loginDialog.isLoading = false
            loginDialog.errorMessage = errorMsg
        }
    }

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        spacing: Theme.spacingMedium

        // Title
        Text {
            text: loginDialog.mode === "login" ? qsTr("Login") : qsTr("Register")
            font.pixelSize: Theme.fontSizeXLarge
            font.weight: Font.Bold
            color: Theme.text
            Layout.alignment: Qt.AlignHCenter
        }

        // Username
        QDTextField {
            id: usernameField
            Layout.fillWidth: true
            placeholderText: qsTr("Username")
            enabled: !loginDialog.isLoading
        }

        // Password
        QDTextField {
            id: passwordField
            Layout.fillWidth: true
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
            enabled: !loginDialog.isLoading

            Keys.onReturnPressed: {
                if (loginDialog.mode === "login") {
                    confirmAction()
                }
            }
        }

        // Phone (register only)
        QDTextField {
            id: phoneField
            Layout.fillWidth: true
            placeholderText: qsTr("Phone (optional)")
            visible: loginDialog.mode === "register"
            enabled: !loginDialog.isLoading
        }

        // Email (register only)
        QDTextField {
            id: emailField
            Layout.fillWidth: true
            placeholderText: qsTr("Email (optional)")
            visible: loginDialog.mode === "register"
            enabled: !loginDialog.isLoading
        }

        // Error message
        Text {
            visible: loginDialog.errorMessage !== ""
            text: loginDialog.errorMessage
            color: loginDialog.errorMessage.indexOf(qsTr("successful")) >= 0 ? Theme.success : Theme.error
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
        }

        // Confirm button
        QDButton {
            Layout.fillWidth: true
            text: loginDialog.isLoading
                  ? qsTr("Please wait...")
                  : (loginDialog.mode === "login" ? qsTr("Login") : qsTr("Register"))
            highlighted: true
            enabled: !loginDialog.isLoading && usernameField.text.length > 0 && passwordField.text.length > 0

            onClicked: confirmAction()
        }

        // Switch mode link
        Text {
            text: loginDialog.mode === "login"
                  ? qsTr("Don't have an account? Register")
                  : qsTr("Already have an account? Login")
            color: Theme.primary
            font.pixelSize: Theme.fontSizeSmall
            Layout.alignment: Qt.AlignHCenter

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    loginDialog.errorMessage = ""
                    loginDialog.mode = loginDialog.mode === "login" ? "register" : "login"
                }
            }
        }
    }

    function confirmAction() {
        if (loginDialog.isLoading) return
        loginDialog.errorMessage = ""
        loginDialog.isLoading = true

        if (loginDialog.mode === "login") {
            loginDialog.mainController.authManager.login(usernameField.text, passwordField.text)
        } else {
            loginDialog.mainController.authManager.registerUser(
                usernameField.text, passwordField.text,
                phoneField.text, emailField.text
            )
        }
    }
}
