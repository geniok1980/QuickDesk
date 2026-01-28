// Floating Tool Button - Draggable button overlay on remote desktop
import QtQuick
import QtQuick.Controls
import "../component"

Item {
    id: root
    
    // Properties
    property string connectionId: ""
    property var clientManager: null
    property var desktopView: null
    property var videoInfo: null  // Video info including original resolution
    
    // Signals
    signal disconnectRequested(string connectionId)
    
    // Size - include extra space for shadow (shadow size is 12px on each side)
    width: 80
    height: 80
    clip: false  // Don't clip shadow
    
    // Draggable behavior
    MouseArea {
        id: dragArea
        anchors.fill: parent
        drag.target: root
        drag.axis: Drag.XAndYAxis
        drag.minimumX: 0
        drag.maximumX: root.parent ? root.parent.width - root.width : 0
        drag.minimumY: 0
        drag.maximumY: root.parent ? root.parent.height - root.height : 0
        
        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
        hoverEnabled: true
        
        // Track if this is a drag or click
        property bool isDragging: false
        property point pressPos: Qt.point(0, 0)
        
        onPressed: function(mouse) {
            isDragging = false
            pressPos = Qt.point(mouse.x, mouse.y)
        }
        
        onPositionChanged: function(mouse) {
            // If moved more than 5 pixels, consider it a drag
            var dx = Math.abs(mouse.x - pressPos.x)
            var dy = Math.abs(mouse.y - pressPos.y)
            if (dx > 5 || dy > 5) {
                isDragging = true
            }
        }
        
        onClicked: {
            if (!isDragging) {
                // Calculate smart menu position
                var menuX, menuY
                var spaceRight = root.parent.width - (root.x + root.width)
                var spaceBottom = root.parent.height - (root.y + root.height)
                
                // Prefer right side, but use left if not enough space
                if (spaceRight >= floatingMenu.width + Theme.spacingMedium) {
                    menuX = root.x + root.width + Theme.spacingMedium
                } else {
                    menuX = root.x - floatingMenu.width - Theme.spacingMedium
                }
                
                // Prefer bottom, but use top if not enough space
                if (spaceBottom >= floatingMenu.height + Theme.spacingMedium) {
                    menuY = root.y
                } else {
                    menuY = root.y + root.height - floatingMenu.height
                }
                
                // Make sure menu is within bounds
                menuX = Math.max(Theme.spacingSmall, Math.min(menuX, root.parent.width - floatingMenu.width - Theme.spacingSmall))
                menuY = Math.max(Theme.spacingSmall, Math.min(menuY, root.parent.height - floatingMenu.height - Theme.spacingSmall))
                
                floatingMenu.x = menuX
                floatingMenu.y = menuY
                floatingMenu.open()
            }
        }
    }
    
    // Circular button background
    Rectangle {
        id: buttonBackground
        anchors.centerIn: parent
        width: 50
        height: 50
        radius: 25
        color: dragArea.containsMouse ? Theme.primaryHover : Theme.primary
        opacity: 0.9
        
        Behavior on color {
            ColorAnimation { duration: Theme.animationDurationFast }
        }
        
        // Icon
        Text {
            id: iconText
            anchors.centerIn: parent
            text: floatingMenu.opened ? FluentIconGlyph.cancelGlyph : FluentIconGlyph.moreGlyph
            font.family: "Segoe Fluent Icons"
            font.pixelSize: 20
            color: Theme.textOnPrimary
            
            // Smooth transition when icon changes
            Behavior on text {
                SequentialAnimation {
                    NumberAnimation {
                        target: iconText
                        property: "opacity"
                        to: 0
                        duration: Theme.animationDurationFast
                    }
                    PropertyAction {
                        target: iconText
                        property: "text"
                    }
                    NumberAnimation {
                        target: iconText
                        property: "opacity"
                        to: 1
                        duration: Theme.animationDurationFast
                    }
                }
            }
        }
    }

    // Shadow effect (outside of button, with margins for shadow space)
    QDShadow {
        anchors.fill: root
        target: root
        shadowSize: 12
        shadowColor: Qt.rgba(0, 0, 0, 0.4)
        z: -1
    }
    
    // Floating Menu using QDMenu
    QDMenu {
        id: floatingMenu
        parent: root.parent
        width: 220
        
        QDMenuItem {
            text: qsTr("Send Ctrl+Alt+Del")
            iconText: FluentIconGlyph.keyboardFullGlyph
            onTriggered: {
                console.log("Send Ctrl+Alt+Del to:", root.connectionId)
            }
        }
        
        QDMenuSeparator { }
        
        QDMenuItem {
            text: qsTr("Performance")
            iconText: FluentIconGlyph.areaChartGlyph
            onTriggered: {
                console.log("Show performance for:", root.connectionId)
            }
        }
        
        QDMenuItem {
            id: resolutionMenuItem
            text: qsTr("Resolution")
            iconText: FluentIconGlyph.resizeMouseMediumGlyph
            hasSubmenu: true
            onTriggered: {
                // Calculate smart submenu position
                var parentMenu = floatingMenu
                var submenu = resolutionMenu
                var windowWidth = root.parent ? root.parent.width : 1920
                var windowHeight = root.parent ? root.parent.height : 1080
                
                // Estimate submenu height (8 items + 1 separator + padding)
                var itemHeight = Theme.buttonHeightMedium
                var separatorHeight = 1 + Theme.spacingXSmall * 2
                var menuPadding = Theme.spacingSmall
                var estimatedSubmenuHeight = (8 * itemHeight) + separatorHeight + (menuPadding * 2) + (Theme.spacingXSmall * 8)
                
                // Calculate vertical position
                // Resolution is the 3rd menu item (after Performance and a separator)
                // Item 0: Ctrl+Alt+Del
                // Item 1: Separator (small height)
                // Item 2: Performance
                // Item 3: Resolution (this item)
                var itemOffsetInMenu = menuPadding + itemHeight + separatorHeight + itemHeight
                var menuY = parentMenu.y + itemOffsetInMenu
                
                // Check if submenu would go off bottom
                var spaceBottom = windowHeight - menuY
                if (spaceBottom < estimatedSubmenuHeight) {
                    // Adjust upward to fit, but align with parent menu if possible
                    menuY = Math.max(Theme.spacingSmall, Math.min(menuY, windowHeight - estimatedSubmenuHeight - Theme.spacingSmall))
                }
                
                // Calculate horizontal position
                var rightX = parentMenu.x + parentMenu.width + Theme.spacingSmall
                var spaceRight = windowWidth - rightX
                
                var menuX
                if (spaceRight >= submenu.width + Theme.spacingSmall) {
                    // Enough space on right - show on right side
                    menuX = rightX
                } else {
                    // Not enough space on right - show on left side
                    menuX = parentMenu.x - submenu.width - Theme.spacingSmall
                    // Make sure it doesn't go off left edge
                    if (menuX < Theme.spacingSmall) {
                        menuX = Theme.spacingSmall
                    }
                }
                
                resolutionMenu.x = menuX
                resolutionMenu.y = menuY
                resolutionMenu.open()
            }
        }
        
        QDMenuItem {
            text: qsTr("Quality")
            iconText: FluentIconGlyph.colorGlyph
            onTriggered: {
                console.log("Show quality for:", root.connectionId)
            }
        }
        
        QDMenuSeparator { }
        
        QDMenuItem {
            text: qsTr("Clipboard")
            iconText: FluentIconGlyph.clipboardListGlyph
            onTriggered: {
                console.log("Show clipboard for:", root.connectionId)
            }
        }
        
        QDMenuItem {
            text: qsTr("File Transfer")
            iconText: FluentIconGlyph.folderGlyph
            onTriggered: {
                console.log("Open file transfer for:", root.connectionId)
            }
        }
        
        QDMenuSeparator { }
        
        QDMenuItem {
            text: qsTr("Disconnect")
            iconText: FluentIconGlyph.cancelGlyph
            isDestructive: true
            onTriggered: {
                console.log("Disconnect connection:", root.connectionId)
                // Emit signal to let RemoteWindow handle both disconnect and tab removal
                root.disconnectRequested(root.connectionId)
            }
        }
    }
    
    // Resolution submenu
    QDMenu {
        id: resolutionMenu
        parent: root.parent
        width: 190
        
        // Close both menus when submenu closes
        onClosed: {
            if (floatingMenu.opened) {
                floatingMenu.close()
            }
        }
        
        QDMenuItem {
            text: {
                if (root.videoInfo && root.videoInfo.originalWidth > 0 && root.videoInfo.originalHeight > 0) {
                    return qsTr("Original") + " (" + root.videoInfo.originalWidth + "x" + root.videoInfo.originalHeight + ")"
                }
                return qsTr("Original")
            }
            enabled: root.videoInfo && root.videoInfo.originalWidth > 0 && root.videoInfo.originalHeight > 0
            onTriggered: {
                // Restore to original resolution (first frame resolution)
                if (root.videoInfo && root.videoInfo.originalWidth > 0 && root.videoInfo.originalHeight > 0 && root.clientManager) {
                    console.log("Restore to original resolution:", root.videoInfo.originalWidth + "x" + root.videoInfo.originalHeight)
                    root.clientManager.setResolution(
                        root.connectionId, 
                        root.videoInfo.originalWidth, 
                        root.videoInfo.originalHeight, 
                        96
                    )
                } else {
                    console.log("Cannot restore to original: invalid resolution data. Width:", root.videoInfo ? root.videoInfo.originalWidth : "null", "Height:", root.videoInfo ? root.videoInfo.originalHeight : "null")
                }
            }
        }
        
        QDMenuSeparator { }
        
        QDMenuItem {
            text: "3840 x 2160 (4K)"
            onTriggered: {
                console.log("Set resolution 3840x2160 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 3840, 2160, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "2560 x 1440 (2K)"
            onTriggered: {
                console.log("Set resolution 2560x1440 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 2560, 1440, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "1920 x 1080 (FHD)"
            onTriggered: {
                console.log("Set resolution 1920x1080 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 1920, 1080, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "1600 x 900"
            onTriggered: {
                console.log("Set resolution 1600x900 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 1600, 900, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "1366 x 768"
            onTriggered: {
                console.log("Set resolution 1366x768 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 1366, 768, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "1280 x 720"
            onTriggered: {
                console.log("Set resolution 1280x720 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 1280, 720, 96)
                }
            }
        }
        
        QDMenuItem {
            text: "1024 x 768"
            onTriggered: {
                console.log("Set resolution 1024x768 for:", root.connectionId)
                if (root.clientManager) {
                    root.clientManager.setResolution(root.connectionId, 1024, 768, 96)
                }
            }
        }
    }
}
