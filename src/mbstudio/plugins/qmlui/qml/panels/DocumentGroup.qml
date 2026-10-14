// One group of document tabs (the left or right half of a split view):
// its tab bar and the area where its documents are shown. The documents
// themselves are created once by DocumentArea and placed in their group.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Item {
    id: root
    property int group: 0
    readonly property alias host: hostArea
    readonly property var workspaces: App.workspaces
    // the shown tab of this group
    readonly property int shownIndex: group === 0 ? workspaces.primaryIndex : workspaces.secondaryIndex
    readonly property bool activeGroup: shownIndex >= 0 && shownIndex === workspaces.currentIndex

    function activate() {
        if (shownIndex >= 0 && shownIndex !== workspaces.currentIndex)
            workspaces.currentIndex = shownIndex
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.panelHeader
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
            RowLayout {
                anchors.fill: parent
                spacing: 0
                ListView {
                    id: tabs
                    objectName: root.group === 0 ? "documentTabs" : "documentTabsRight"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    clip: true
                    model: root.workspaces
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        id: tab
                        required property int index
                        required property string title
                        required property string icon
                        required property bool closable
                        required property int group
                        required property bool shown
                        required property string workspace
                        readonly property bool mine: group === root.group
                        readonly property bool current: index === root.workspaces.currentIndex
                        visible: mine
                        width: mine ? tabRow.implicitWidth + 20 : 0
                        height: ListView.view.height
                        color: shown ? Theme.tabActive : tabMouse.containsMouse ? Theme.surfaceHover : "transparent"
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 2; color: Theme.accent; visible: tab.current }
                        Rectangle { anchors.top: parent.top; width: parent.width; height: 2; color: Theme.border; visible: tab.shown && !tab.current }
                        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.border }
                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                            onClicked: (mouse) => {
                                if (mouse.button === Qt.MiddleButton) {
                                    root.workspaces.close(tab.index)
                                } else if (mouse.button === Qt.RightButton) {
                                    tabMenu.index = tab.index
                                    tabMenu.workspace = tab.workspace
                                    tabMenu.closable = tab.closable
                                    tabMenu.popup()
                                } else {
                                    root.workspaces.currentIndex = tab.index
                                }
                            }
                        }
                        RowLayout {
                            id: tabRow
                            anchors.centerIn: parent
                            spacing: 5
                            Icon { path: Icons.byName(tab.icon); size: 13; color: tab.current ? Theme.accent : Theme.textMuted }
                            Text {
                                text: tab.title
                                color: tab.shown ? Theme.text : Theme.textMuted
                                font.pixelSize: Theme.fontSmall + 1
                                elide: Text.ElideMiddle
                                Layout.maximumWidth: 200
                            }
                            IconButton {
                                visible: tab.closable
                                implicitWidth: 18
                                implicitHeight: 18
                                padding: 2
                                iconPath: Icons.close
                                tip: qsTr("Close")
                                onClicked: root.workspaces.close(tab.index)
                            }
                        }
                    }
                }
                IconButton {
                    objectName: "splitButton"
                    iconPath: Icons.split
                    tip: root.group === 0 ? qsTr("Show the document on the right (split view)") : qsTr("Move the document back to the left")
                    enabled: root.shownIndex >= 0
                    onClicked: root.workspaces.moveToGroup(root.shownIndex, root.group === 0 ? 1 : 0)
                }
                IconButton {
                    visible: root.group === 1
                    iconPath: Icons.close
                    tip: qsTr("Join the split view")
                    onClicked: root.workspaces.unsplit()
                }
            }
        }
        Item {
            id: hostArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            // a press anywhere in the group makes its document the active one
            Item {
                anchors.fill: parent
                z: 100
                TapHandler {
                    acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                    gesturePolicy: TapHandler.DragThreshold
                    onPressedChanged: if (pressed) root.activate()
                }
            }
            // the active group of a split view is outlined
            Rectangle {
                anchors.fill: parent
                z: 101
                color: "transparent"
                border.color: Theme.accent
                border.width: 1
                visible: root.workspaces.split && root.activeGroup
                opacity: 0.6
            }
        }
    }

    Menu {
        id: tabMenu
        property int index: -1
        property string workspace
        property bool closable: false
        MenuItem {
            text: root.group === 0 ? qsTr("Show on the right") : qsTr("Move to the left")
            onTriggered: root.workspaces.moveToGroup(tabMenu.index, root.group === 0 ? 1 : 0)
        }
        MenuItem {
            text: qsTr("Open in 3D")
            visible: tabMenu.workspace === "grid"
            height: visible ? implicitHeight : 0
            onTriggered: App.openTerrain(root.workspaces.argument(tabMenu.index), root.group === 0 ? 1 : 0)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Close")
            enabled: tabMenu.closable
            onTriggered: root.workspaces.close(tabMenu.index)
        }
    }
}
