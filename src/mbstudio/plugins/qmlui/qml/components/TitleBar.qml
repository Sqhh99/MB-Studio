// The title bar drawn by MB-Studio: application and project identity, the
// menus (default content), tool buttons and the window buttons.
//
// With native frame integration (Windows) the window system moves the
// window from the empty parts of the bar (Snap, double click, system menu);
// elsewhere the bar asks the window system to move the window. Controls in
// the bar take the mouse themselves (see WindowChrome::onControl).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."

Rectangle {
    id: root
    property var chrome: null
    property string title: ""
    property string subtitle: ""
    property bool unsaved: false
    // menus and other controls on the left
    default property alias content: slot.data
    // buttons before the window buttons
    property alias tools: toolSlot.data
    readonly property bool windowActive: chrome === null || chrome.windowActive
    implicitHeight: Theme.titleBarHeight
    color: windowActive ? Theme.titleBar : Theme.titleBarInactive

    // moving and maximising where the window system does not hit test
    DragHandler {
        target: null
        enabled: root.chrome !== null && root.chrome.active && !root.chrome.nativeFrame
        onActiveChanged: if (active) root.chrome.startMove()
    }
    TapHandler {
        enabled: root.chrome !== null && root.chrome.active && !root.chrome.nativeFrame
        onDoubleTapped: root.chrome.toggleMaximize()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        spacing: 2

        Image {
            source: "qrc:/mbstudio/images/mbstudio.png"
            sourceSize: Qt.size(48, 48)
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            Layout.rightMargin: 4
            smooth: true
            opacity: root.windowActive ? 1.0 : 0.6
        }
        RowLayout {
            id: slot
            spacing: 0
            Layout.fillHeight: true
        }
        // identity: project, unsaved edits
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 40
            RowLayout {
                anchors.centerIn: parent
                width: Math.min(implicitWidth, parent.width - 8)
                spacing: 6
                Text {
                    objectName: "windowTitle"
                    text: root.title
                    color: root.windowActive ? Theme.text : Theme.textMuted
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                    Layout.maximumWidth: implicitWidth
                }
                Text {
                    visible: root.subtitle !== ""
                    text: root.subtitle
                    color: Theme.textFaint
                    font.pixelSize: Theme.fontSmall
                    elide: Text.ElideRight
                }
                Rectangle {
                    visible: root.unsaved
                    Layout.preferredWidth: 7
                    Layout.preferredHeight: 7
                    radius: 3.5
                    color: Theme.warning
                    ToolTip.visible: unsavedMouse.containsMouse
                    ToolTip.text: qsTr("Sounding edits that are not saved")
                    MouseArea { id: unsavedMouse; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }
            }
        }
        RowLayout {
            id: toolSlot
            spacing: 0
        }
        // window buttons
        Row {
            visible: root.chrome !== null && root.chrome.active
            Layout.fillHeight: true
            Layout.leftMargin: 4
            WindowButton {
                objectName: "minimizeButton"
                height: root.height
                iconPath: Icons.windowMinimize
                accessibleName: qsTr("Minimise")
                onClicked: root.chrome.minimize()
            }
            WindowButton {
                objectName: "maximizeButton"
                height: root.height
                iconPath: root.chrome !== null && root.chrome.maximized ? Icons.windowRestore : Icons.windowMaximize
                accessibleName: root.chrome !== null && root.chrome.maximized ? qsTr("Restore") : qsTr("Maximise")
                onClicked: root.chrome.toggleMaximize()
            }
            WindowButton {
                objectName: "closeButton"
                height: root.height
                iconPath: Icons.close
                accessibleName: qsTr("Close")
                danger: true
                onClicked: root.chrome.close()
            }
        }
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.border }
}
