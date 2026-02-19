// Fluent Design Accordion Component
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: control
    
    // ============ Custom Properties ============
    
    property string title: ""
    property string iconSource: ""
    property bool expanded: false
    property bool showBorder: true
    
    default property alias content: contentContainer.data
    
    // ============ Size & Style ============
    
    implicitWidth: parent ? parent.width : 400
    implicitHeight: header.height + contentWrapper.height
    
    // ============ Content ============
    
    Column {
        width: parent.width
        spacing: 0
        
        // Header
        Rectangle {
            id: header
            width: parent.width
            height: Theme.buttonHeightLarge
            color: headerArea.containsMouse ? Theme.surfaceHover : Theme.surface
            border.width: control.showBorder ? Theme.borderWidthThin : 0
            border.color: Theme.border
            radius: Theme.radiusMedium
            
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingMedium
                anchors.rightMargin: Theme.spacingMedium
                spacing: Theme.spacingMedium
                
                // Icon
                Text {
                    visible: control.iconSource !== ""
                    text: control.iconSource
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 20
                    color: Theme.primary
                }
                
                // Title
                Text {
                    text: control.title
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    font.weight: Font.DemiBold
                    color: Theme.text
                    Layout.fillWidth: true
                }
                
                // Expand Icon
                Text {
                    text: control.expanded ? FluentIconGlyph.chevronUpGlyph : FluentIconGlyph.chevronDownGlyph
                    font.family: "Segoe Fluent Icons"
                    font.pixelSize: 14
                    color: Theme.textSecondary
                    
                    Behavior on rotation {
                        NumberAnimation {
                            duration: Theme.animationDurationMedium
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
            
            MouseArea {
                id: headerArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                
                onClicked: {
                    control.expanded = !control.expanded
                    control.expandedChanged()
                }
            }
            
            Behavior on color {
                ColorAnimation { duration: Theme.animationDurationFast }
            }
        }
        
        // Content
        Rectangle {
            id: contentWrapper
            width: parent.width
            height: control.expanded ? contentContainer.childrenRect.height + Theme.spacingMedium * 2 : 0
            visible: height > 0
            color: Theme.surface
            border.width: control.showBorder ? Theme.borderWidthThin : 0
            border.color: Theme.border
            radius: Theme.radiusMedium
            clip: true
            
            // Top border overlaps with header
            anchors.topMargin: -1
            
            Item {
                id: contentContainer
                width: parent.width - Theme.spacingMedium * 2
                height: childrenRect.height
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Theme.spacingMedium
                anchors.topMargin: Theme.spacingMedium + 1
            }
            
            Behavior on height {
                NumberAnimation {
                    duration: Theme.animationDurationMedium
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
}
