// Kernel log (messages from all plugins).
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MBStudio.Backend
import ".."
import "../components"

Rectangle {
    id: root
    color: Theme.surface
    border.color: Theme.border
    readonly property var log: App.log

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 14
            Layout.rightMargin: 6
            Layout.preferredHeight: 36
            Icon { path: Icons.terminal; color: Theme.accent; size: 18 }
            Text { text: qsTr("Log"); color: Theme.text; font.pixelSize: Theme.fontBody; font.weight: Font.DemiBold; Layout.fillWidth: true }
            IconButton { iconPath: Icons.copy; tip: qsTr("Copy log"); onClicked: App.copyToClipboard(log.text()) }
            IconButton { iconPath: Icons.sweep; tip: qsTr("Clear log"); onClicked: log.clear() }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: log
            onCountChanged: positionViewAtEnd()
            ScrollBar.vertical: ScrollBar {}
            delegate: RowLayout {
                width: ListView.view.width
                height: 22
                spacing: 10
                Text { text: time; color: Theme.textFaint; font.family: Theme.monoFont; font.pixelSize: Theme.fontSmall; Layout.leftMargin: 14 }
                Rectangle { width: 7; height: 7; radius: 4; color: Theme.levelColor(level) }
                Text { text: source; color: Theme.textMuted; font.pixelSize: Theme.fontSmall; Layout.preferredWidth: 70; elide: Text.ElideRight }
                Text { text: message; color: Theme.text; font.pixelSize: Theme.fontSmall + 1; Layout.fillWidth: true; elide: Text.ElideRight }
            }
        }
    }
}
