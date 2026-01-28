// Remote Desktop Window - Independent window for remote desktop connections
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QuickDesk 1.0

import "../component"
import "../quickdeskcomponent"

Window {
    id: remoteWindow
    width: 1280
    height: 720
    visible: true
    title: qsTr("QuickDesk - Remote Desktop")
    
    // Properties
    property var clientManager: null
    property var connections: [] // Array of connection objects: [{id, deviceId, name, ping, state}]
    property int currentTabIndex: 0
    
    // Add connection to this window
    function addConnection(connectionId, deviceId) {
        // Check if connection already exists
        for (var i = 0; i < connections.length; i++) {
            if (connections[i].id === connectionId) {
                console.log("Connection already exists in window:", connectionId)
                currentTabIndex = i
                return
            }
        }
        
        var conn = {
            id: connectionId,
            deviceId: deviceId,
            name: deviceId,
            ping: 0,
            state: "connecting"
        }
        
        // Create new array to trigger property binding update
        var newConnections = connections.slice()
        newConnections.push(conn)
        connections = newConnections
        
        currentTabIndex = connections.length - 1
        console.log("Added connection to remote window:", connectionId, "Total tabs:", connections.length)
    }
    
    // Close connection and remove tab (unified function for both scenarios)
    function closeConnection(index) {
        if (index < 0 || index >= connections.length) {
            console.warn("closeConnection: invalid index", index)
            return
        }
        
        var connId = connections[index].id
        console.log("Closing connection:", connId, "at index:", index)
        
        // 1. Disconnect from host
        if (clientManager) {
            clientManager.disconnectFromHost(connId)
        }
        
        // 2. Remove the tab
        removeConnection(index)
    }
    
    // Remove connection from this window (internal helper)
    function removeConnection(index) {
        if (index < 0 || index >= connections.length) return
        
        var connId = connections[index].id
        
        // Create new array to trigger property binding update
        var newConnections = connections.slice()
        newConnections.splice(index, 1)
        connections = newConnections
        
        // Update current tab index
        if (currentTabIndex >= connections.length) {
            currentTabIndex = Math.max(0, connections.length - 1)
        }
        
        // Close window if no connections left
        if (connections.length === 0) {
            remoteWindow.close()
        }
        
        console.log("Removed connection from remote window:", connId, "Remaining tabs:", connections.length)
    }
    
    // Clean up all connections when window closes
    onClosing: function(close) {
        console.log("RemoteWindow closing, disconnecting all connections")
        for (var i = 0; i < connections.length; i++) {
            if (clientManager) {
                console.log("Disconnecting:", connections[i].id)
                clientManager.disconnectFromHost(connections[i].id)
            }
        }
        connections = []
    }
    
    // Update connection state
    function updateConnectionState(connectionId, state, ping) {
        var updated = false
        for (var i = 0; i < connections.length; i++) {
            if (connections[i].id === connectionId) {
                // Create new array to trigger property binding update
                var newConnections = connections.slice()
                newConnections[i].state = state
                newConnections[i].ping = ping || 0
                connections = newConnections
                updated = true
                break
            }
        }
        if (updated) {
            console.log("Updated connection state:", connectionId, "->", state)
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        // Tab Bar
        RemoteTabBar {
            id: tabBar
            Layout.fillWidth: true
            connections: remoteWindow.connections
            currentIndex: remoteWindow.currentTabIndex
            
            onTabClicked: function(index) {
                remoteWindow.currentTabIndex = index
            }
            
            onTabCloseRequested: function(index) {
                remoteWindow.closeConnection(index)
            }
            
            onNewTabRequested: {
                // TODO: Show quick connect dialog
                console.log("New tab requested")
            }
        }
        
        // Remote Desktop View Stack
        StackLayout {
            id: desktopStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: remoteWindow.currentTabIndex
            
            Repeater {
                model: remoteWindow.connections
                
                Item {
                    required property int index
                    required property var modelData
                    
                    // Remote desktop video view (ONLY video, no overlay UI)
                    RemoteDesktopView {
                        id: desktopView
                        anchors.fill: parent
                        connectionId: modelData.id
                        clientManager: remoteWindow.clientManager
                        active: index === remoteWindow.currentTabIndex
                    }
                }
            }
        }
    }
        
    Item {
        anchors.fill: parent
        anchors.topMargin: tabBar.height  // Offset by tab bar height        
        
        // Single floating button bound to current active connection
        FloatingToolButton {
            x: parent.width - width - Theme.spacingXLarge
            y: Theme.spacingXLarge
            z: 1000
            visible: remoteWindow.connections.length > 0
            
            connectionId: remoteWindow.currentTabIndex >= 0 && remoteWindow.currentTabIndex < remoteWindow.connections.length 
                ? remoteWindow.connections[remoteWindow.currentTabIndex].id 
                : ""
            clientManager: remoteWindow.clientManager
            desktopView: {
                // Find the current desktop view
                if (remoteWindow.currentTabIndex >= 0) {
                    var stackItem = desktopStack.children[remoteWindow.currentTabIndex]
                    return stackItem ? stackItem.children[0] : null
                }
                return null
            }
            
            onDisconnectRequested: function(connectionId) {
                console.log("FloatingToolButton disconnect requested for:", connectionId)
                
                // Find the connection index and close it
                for (var i = 0; i < remoteWindow.connections.length; i++) {
                    if (remoteWindow.connections[i].id === connectionId) {
                        remoteWindow.closeConnection(i)
                        break
                    }
                }
            }
        }
    }
    
    // Monitor connection state changes
    Connections {
        target: remoteWindow.clientManager
        
        function onConnectionStateChanged(connectionId, state, hostInfo) {
            console.log("Remote window: connection state changed:", connectionId, state)
            remoteWindow.updateConnectionState(connectionId, state, 0)
        }
    }
}
